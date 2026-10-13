#include "guest/x86_guest.hpp"
#include <unicorn/unicorn.h>
#include <cstring>
namespace sadx {
namespace {
uc_engine* engine(void* p){return static_cast<uc_engine*>(p);}
bool fail(std::string* error,const std::string& text){if(error)*error=text;return false;}
}
std::unique_ptr<X86Guest> X86Guest::create(std::string* error){
    std::unique_ptr<X86Guest> g(new X86Guest);uc_engine* uc=nullptr;
    const auto rc=uc_open(UC_ARCH_X86,UC_MODE_32,&uc);
    if(rc!=UC_ERR_OK){fail(error,std::string("uc_open: ")+uc_strerror(rc));return nullptr;}
    g->uc_=uc;
    std::string e;
    if(!g->map(g->stack_base_,g->stack_size_,false,&e)||!g->map(g->stop_address_&~0xfffU,0x1000,true,&e)){fail(error,e);return nullptr;}
    const std::uint8_t halt[]={0xf4};g->write(g->stop_address_,halt,1); // never executed: emulation stops before it
    return g;
}
X86Guest::~X86Guest(){if(uc_)uc_close(engine(uc_));}
bool X86Guest::map(std::uint32_t base,std::uint32_t size,bool executable,std::string* error){
    const auto aligned=(size+0xfffU)&~0xfffU;
    const auto rc=uc_mem_map(engine(uc_),base,aligned,UC_PROT_READ|UC_PROT_WRITE|(executable?UC_PROT_EXEC:0));
    if(rc!=UC_ERR_OK)return fail(error,"uc_mem_map "+std::to_string(base)+": "+uc_strerror(rc));
    return true;
}
bool X86Guest::map_image(const ExecutableImage& image,std::string* error){
    // The image as the loader lays it out: SizeOfImage bytes from the base, file bytes where the
    // image has them and zeros elsewhere (section tails, .bss). One RWX mapping; per-section
    // protection is not modelled yet.
    const auto base=image.image_base(),size=image.size_of_image();
    if(!map(base,size,true,error))return false;
    std::vector<std::uint8_t> bytes(size);
    for(std::uint32_t i=0;i<size;++i)bytes[i]=image.byte(base+i).value_or(0);
    if(!write(base,bytes.data(),bytes.size()))return fail(error,"image write");
    return true;
}
bool X86Guest::write(std::uint32_t address,const void* bytes,std::size_t size){return uc_mem_write(engine(uc_),address,bytes,size)==UC_ERR_OK;}
bool X86Guest::read(std::uint32_t address,void* bytes,std::size_t size)const{return uc_mem_read(engine(uc_),address,bytes,size)==UC_ERR_OK;}
std::optional<std::uint32_t> X86Guest::u32(std::uint32_t address)const{std::uint32_t v;if(!read(address,&v,4))return std::nullopt;return v;}
bool X86Guest::set_u32(std::uint32_t address,std::uint32_t value){return write(address,&value,4);}
std::uint32_t X86Guest::reg(int r)const{std::uint32_t v=0;uc_reg_read(engine(uc_),r,&v);return v;}
void X86Guest::set_reg(int r,std::uint32_t v){uc_reg_write(engine(uc_),r,&v);}
void X86Guest::code_hook(void*,std::uint64_t address,std::uint32_t,void* user){
    auto* self=static_cast<X86Guest*>(user);
    const auto found=self->overrides_.find(static_cast<std::uint32_t>(address));
    if(found==self->overrides_.end())return;
    const auto pops=found->second(*self);
    if(self->stopped_)return; // the override stopped the guest: leave the state at the stop
    // Emulate `ret N`: the override replaced the whole callee.
    auto esp=self->reg(UC_X86_REG_ESP);const auto back=self->u32(esp).value_or(self->stop_address_);
    self->set_reg(UC_X86_REG_ESP,esp+4+pops);self->set_reg(UC_X86_REG_EIP,back);
}
void X86Guest::stop(std::string error){stopped_=std::move(error);uc_emu_stop(engine(uc_));}
std::uint32_t X86Guest::esp()const{return reg(UC_X86_REG_ESP);}
std::uint32_t X86Guest::eip()const{return reg(UC_X86_REG_EIP);}
void X86Guest::set_eax(std::uint32_t v){set_reg(UC_X86_REG_EAX,v);}
void X86Guest::set_edx(std::uint32_t v){set_reg(UC_X86_REG_EDX,v);}
bool X86Guest::add_override(std::uint32_t address,Override override,std::string* error){
    if(overrides_.count(address))return fail(error,"override already bound");
    uc_hook handle=0;
    const auto rc=uc_hook_add(engine(uc_),&handle,UC_HOOK_CODE,reinterpret_cast<void*>(&X86Guest::code_hook),this,address,address);
    if(rc!=UC_ERR_OK)return fail(error,std::string("uc_hook_add: ")+uc_strerror(rc));
    overrides_[address]=std::move(override);handles_.push_back(static_cast<std::uint32_t>(handle));
    uc_ctl_flush_tb(engine(uc_)); // blocks translated before the hook existed would bypass it
    return true;
}
GuestCallResult X86Guest::run(std::uint32_t from,std::uint32_t until,std::uint16_t control_word,std::uint64_t limit){
    GuestCallResult r;
    const std::uint32_t esp=stack_base_+stack_size_-0x1000-4;write(esp,&stop_address_,4);
    set_reg(UC_X86_REG_ESP,esp);set_reg(UC_X86_REG_EBP,esp);
    std::uint32_t cw=control_word,sw=0,tag=0xffff;uc_reg_write(engine(uc_),UC_X86_REG_FPCW,&cw);uc_reg_write(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_write(engine(uc_),UC_X86_REG_FPTAG,&tag);
    stopped_.reset();
    const auto rc=uc_emu_start(engine(uc_),from,until,0,limit);
    r.eax=reg(UC_X86_REG_EAX);r.eip=reg(UC_X86_REG_EIP);r.esp=reg(UC_X86_REG_ESP);
    if(stopped_){r.error=*stopped_;stopped_.reset();return r;}
    if(rc!=UC_ERR_OK){r.error=uc_strerror(rc);return r;}
    r.completed=r.eip==until;
    if(!r.completed)r.error="stopped at instruction limit";
    return r;
}
GuestCallResult X86Guest::call(std::uint32_t address,std::initializer_list<std::uint32_t> arguments,std::optional<std::uint32_t> ecx,std::optional<std::uint32_t> edx,std::uint16_t control_word,std::uint64_t limit){
    GuestCallResult r;
    // Fresh stack: arguments right-to-left, then the stop return address.
    std::vector<std::uint32_t> frame(arguments.begin(),arguments.end());
    std::uint32_t esp=stack_base_+stack_size_-0x1000;
    esp-=static_cast<std::uint32_t>(4*frame.size()+4);
    std::vector<std::uint32_t> words;words.push_back(stop_address_);words.insert(words.end(),frame.begin(),frame.end());
    write(esp,words.data(),words.size()*4);
    set_reg(UC_X86_REG_ESP,esp);set_reg(UC_X86_REG_EBP,esp);
    if(ecx)set_reg(UC_X86_REG_ECX,*ecx);if(edx)set_reg(UC_X86_REG_EDX,*edx);
    std::uint32_t cw=control_word,sw=0,tag=0xffff;uc_reg_write(engine(uc_),UC_X86_REG_FPCW,&cw);uc_reg_write(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_write(engine(uc_),UC_X86_REG_FPTAG,&tag);
    std::uint32_t mxcsr=0x1f80;uc_reg_write(engine(uc_),UC_X86_REG_MXCSR,&mxcsr);
    stopped_.reset();
    const auto rc=uc_emu_start(engine(uc_),address,stop_address_,0,limit);
    r.eax=reg(UC_X86_REG_EAX);r.eip=reg(UC_X86_REG_EIP);r.esp=reg(UC_X86_REG_ESP);
    if(stopped_){r.error=*stopped_;stopped_.reset();return r;}
    if(rc!=UC_ERR_OK){r.error=uc_strerror(rc);return r;}
    r.completed=r.eip==stop_address_;
    if(!r.completed)r.error="stopped at instruction limit";
    return r;
}
} // namespace sadx
