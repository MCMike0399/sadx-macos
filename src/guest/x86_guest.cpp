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
    g->install_continuations();
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
// Host writes (HLE, loader patches through WriteProcessMemory, tests) invalidate translated code in the range:
// Unicorn's self-modifying-code detection covers guest stores only.
bool X86Guest::write(std::uint32_t address,const void* bytes,std::size_t size){
    if(uc_mem_write(engine(uc_),address,bytes,size)!=UC_ERR_OK)return false;
    if(size)uc_ctl_remove_cache(engine(uc_),address,static_cast<std::uint64_t>(address)+size);
    return true;}
bool X86Guest::read(std::uint32_t address,void* bytes,std::size_t size)const{return uc_mem_read(engine(uc_),address,bytes,size)==UC_ERR_OK;}
std::optional<std::uint32_t> X86Guest::u32(std::uint32_t address)const{std::uint32_t v;if(!read(address,&v,4))return std::nullopt;return v;}
bool X86Guest::set_u32(std::uint32_t address,std::uint32_t value){return write(address,&value,4);}
std::uint32_t X86Guest::reg(int r)const{std::uint32_t v=0;uc_reg_read(engine(uc_),r,&v);return v;}
void X86Guest::set_reg(int r,std::uint32_t v){uc_reg_write(engine(uc_),r,&v);}
void X86Guest::code_hook(void*,std::uint64_t address,std::uint32_t,void* user){
    auto* self=static_cast<X86Guest*>(user);
    const auto found=self->overrides_.find(static_cast<std::uint32_t>(address));
    if(found==self->overrides_.end())return;
    self->redirected_=false;
    const auto pops=found->second(*self);
    if(self->stopped_)return; // the override stopped the guest: leave the state at the stop
    if(self->redirected_){self->redirected_=false;return;} // the override called into the guest; its continuation returns
    // Emulate `ret N`: the override replaced the whole callee.
    auto esp=self->reg(UC_X86_REG_ESP);const auto back=self->u32(esp).value_or(self->stop_address_);
    self->set_reg(UC_X86_REG_ESP,esp+4+pops);self->set_reg(UC_X86_REG_EIP,back);
}
void X86Guest::install_continuations(){
    std::string e;map(continuation_base_,0x10000,true,&e);
    const std::vector<std::uint8_t> halt(0x10000,0xf4);write(continuation_base_,halt.data(),halt.size());
    uc_hook handle=0;
    uc_hook_add(engine(uc_),&handle,UC_HOOK_CODE,reinterpret_cast<void*>(&X86Guest::continuation_hook),this,continuation_base_,continuation_base_+0xffff);
}
void X86Guest::redirect(std::uint32_t target,const std::vector<std::uint32_t>& arguments,std::uint32_t pops,Continuation then){
    // Called from an override: [ESP] is the override caller's return address, then its arguments.
    const auto esp=reg(UC_X86_REG_ESP);const auto back=u32(esp).value_or(stop_address_);
    std::uint32_t trap;
    if(!free_continuations_.empty()){trap=free_continuations_.back();free_continuations_.pop_back();}
    else{trap=next_continuation_;next_continuation_+=4;}
    pending_[trap]=Pending{esp+4+pops,back,std::move(then)};
    std::vector<std::uint32_t> frame;frame.push_back(trap);frame.insert(frame.end(),arguments.begin(),arguments.end());
    const auto top=esp+4+pops-static_cast<std::uint32_t>(4*frame.size());
    write(top,frame.data(),frame.size()*4);
    set_reg(UC_X86_REG_ESP,top);set_reg(UC_X86_REG_EIP,target);redirected_=true;
}
void X86Guest::continuation_hook(void*,std::uint64_t address,std::uint32_t,void* user){
    auto* self=static_cast<X86Guest*>(user);
    const auto found=self->pending_.find(static_cast<std::uint32_t>(address));
    if(found==self->pending_.end()){self->stop("return to an unknown continuation "+std::to_string(address));return;}
    auto pending=std::move(found->second);self->pending_.erase(found);self->free_continuations_.push_back(static_cast<std::uint32_t>(address));
    // The callee returned: restore the caller's stack, then let the continuation set results or call again.
    self->set_reg(UC_X86_REG_ESP,pending.resume_esp-4);self->write(pending.resume_esp-4,&pending.return_address,4);
    self->redirected_=false;
    if(pending.then)pending.then(*self);
    if(self->stopped_)return;
    if(self->redirected_){self->redirected_=false;return;}
    self->set_reg(UC_X86_REG_ESP,pending.resume_esp);self->set_reg(UC_X86_REG_EIP,pending.return_address);
}
bool X86Guest::set_fs_base(std::uint32_t teb,std::uint32_t gdt,std::string* error){
    if(!map(gdt,0x1000,false,error))return false;
    const auto descriptor=[](std::uint32_t base,std::uint32_t limit,std::uint8_t access)->std::uint64_t{
        // limit in 4 KiB units (G=1), 32-bit (D=1)
        std::uint64_t d=0;d|=limit&0xffffU;d|=std::uint64_t(base&0xffffffU)<<16;d|=std::uint64_t(access)<<40;
        d|=std::uint64_t((limit>>16)&0xfU)<<48;d|=std::uint64_t(0xcU)<<52;d|=std::uint64_t(base>>24)<<56;return d;};
    std::uint64_t table[8]={};
    table[1]=descriptor(0,0xfffff,0x9b);  // code, ring 0 (Unicorn runs the guest at CPL 0)
    table[2]=descriptor(0,0xfffff,0x93);  // data / stack
    table[3]=descriptor(teb,0xfffff,0x93); // FS: the TEB (limit covers the page and beyond, as Windows' does)
    if(!write(gdt,table,sizeof table))return fail(error,"GDT write");
    uc_x86_mmr gdtr{};gdtr.base=gdt;gdtr.limit=sizeof table-1;
    if(uc_reg_write(engine(uc_),UC_X86_REG_GDTR,&gdtr)!=UC_ERR_OK)return fail(error,"GDTR");
    const std::uint32_t cs=1<<3,ds=2<<3,fs=3<<3;
    if(uc_reg_write(engine(uc_),UC_X86_REG_CS,&cs)!=UC_ERR_OK||uc_reg_write(engine(uc_),UC_X86_REG_DS,&ds)!=UC_ERR_OK||
       uc_reg_write(engine(uc_),UC_X86_REG_ES,&ds)!=UC_ERR_OK||uc_reg_write(engine(uc_),UC_X86_REG_SS,&ds)!=UC_ERR_OK||
       uc_reg_write(engine(uc_),UC_X86_REG_FS,&fs)!=UC_ERR_OK)return fail(error,"segment registers");
    return true;
}
void X86Guest::return_x87(std::uint64_t significand,std::uint16_t sign_exponent){
    std::uint32_t sw=0,tag=0;uc_reg_read(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_read(engine(uc_),UC_X86_REG_FPTAG,&tag);
    const std::uint32_t top=((sw>>11)&7U)-1U&7U;sw=(sw&~0x3800U)|(top<<11);
    struct {std::uint64_t mantissa;std::uint16_t exponent;} value{significand,sign_exponent}; // Unicorn's 10-byte FPn layout
    uc_reg_write(engine(uc_),UC_X86_REG_FP0+static_cast<int>(top),&value);
    tag&=~(3U<<(2*top)); // 00 = valid (the full tag word)
    uc_reg_write(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_write(engine(uc_),UC_X86_REG_FPTAG,&tag);
}
std::pair<std::uint64_t,std::uint16_t> X86Guest::st(unsigned i)const{
    std::uint32_t sw=0;uc_reg_read(engine(uc_),UC_X86_REG_FPSW,&sw);const auto top=(sw>>11)&7U;
    struct {std::uint64_t mantissa;std::uint16_t exponent;} value{};uc_reg_read(engine(uc_),UC_X86_REG_FP0+static_cast<int>((top+i)&7U),&value);return {value.mantissa,value.exponent};
}
std::uint64_t X86Guest::xmm0_low()const{std::uint64_t v[2]={};uc_reg_read(engine(uc_),UC_X86_REG_XMM0,v);return v[0];}
void X86Guest::set_xmm0_low(std::uint64_t low){std::uint64_t v[2]={};uc_reg_read(engine(uc_),UC_X86_REG_XMM0,v);v[0]=low;uc_reg_write(engine(uc_),UC_X86_REG_XMM0,v);}
void X86Guest::set_st(unsigned i,std::uint64_t significand,std::uint16_t sign_exponent){
    std::uint32_t sw=0,tag=0;uc_reg_read(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_read(engine(uc_),UC_X86_REG_FPTAG,&tag);
    const auto physical=(((sw>>11)&7U)+i)&7U;struct {std::uint64_t mantissa;std::uint16_t exponent;} value{significand,sign_exponent};
    uc_reg_write(engine(uc_),UC_X86_REG_FP0+static_cast<int>(physical),&value);tag&=~(3U<<(2*physical));uc_reg_write(engine(uc_),UC_X86_REG_FPTAG,&tag);
}
std::uint16_t X86Guest::fpu_status()const{std::uint32_t sw=0;uc_reg_read(engine(uc_),UC_X86_REG_FPSW,&sw);return static_cast<std::uint16_t>(sw);}
void X86Guest::set_fpu_status(std::uint16_t v){const std::uint32_t sw=v;uc_reg_write(engine(uc_),UC_X86_REG_FPSW,&sw);}
void X86Guest::instruction_hook(void*,std::uint64_t address,std::uint32_t size,void* user){
    auto* self=static_cast<X86Guest*>(user);const auto i=self->instruction_hooks_.find(static_cast<std::uint32_t>(address));
    if(i==self->instruction_hooks_.end())return;
    if(i->second(*self,static_cast<std::uint32_t>(address))&&!self->stopped_)self->set_reg(UC_X86_REG_EIP,static_cast<std::uint32_t>(address)+size);
}
bool X86Guest::add_instruction_hook(std::uint32_t address,InstructionHook hook,std::string* error){
    if(instruction_hooks_.count(address))return fail(error,"instruction hook already bound");
    uc_hook handle=0;
    const auto rc=uc_hook_add(engine(uc_),&handle,UC_HOOK_CODE,reinterpret_cast<void*>(&X86Guest::instruction_hook),this,address,address);
    if(rc!=UC_ERR_OK)return fail(error,std::string("uc_hook_add: ")+uc_strerror(rc));
    instruction_hooks_[address]=std::move(hook);uc_ctl_flush_tb(engine(uc_));return true;
}
void X86Guest::pop_x87(){
    std::uint32_t sw=0,tag=0;uc_reg_read(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_read(engine(uc_),UC_X86_REG_FPTAG,&tag);
    const auto top=(sw>>11)&7U;tag|=3U<<(2*top);sw=(sw&~0x3800U)|(((top+1)&7U)<<11);
    uc_reg_write(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_write(engine(uc_),UC_X86_REG_FPTAG,&tag);
}
void X86Guest::write_hook(void* uc,int,std::uint64_t address,int size,std::int64_t value,void* user){
    std::uint32_t pc=0;uc_reg_read(static_cast<uc_engine*>(uc),UC_X86_REG_EIP,&pc);
    (*static_cast<WriteWatch*>(user))(pc,static_cast<std::uint32_t>(address),static_cast<std::uint32_t>(size),static_cast<std::uint64_t>(value));
}
bool X86Guest::add_write_watch(std::uint32_t begin,std::uint32_t end,WriteWatch seen,std::string* error){
    watches_.push_back(std::make_unique<WriteWatch>(std::move(seen)));uc_hook handle=0;
    const auto rc=uc_hook_add(engine(uc_),&handle,UC_HOOK_MEM_WRITE,reinterpret_cast<void*>(&X86Guest::write_hook),watches_.back().get(),begin,end-1);
    if(rc!=UC_ERR_OK)return fail(error,std::string("uc_hook_add: ")+uc_strerror(rc));
    return true;
}
void X86Guest::stop(std::string error){stopped_=std::move(error);uc_emu_stop(engine(uc_));}
std::uint32_t X86Guest::esp()const{return reg(UC_X86_REG_ESP);}
std::uint32_t X86Guest::eip()const{return reg(UC_X86_REG_EIP);}
void X86Guest::set_eax(std::uint32_t v){set_reg(UC_X86_REG_EAX,v);}
void X86Guest::set_edx(std::uint32_t v){set_reg(UC_X86_REG_EDX,v);}
bool X86Guest::reserve_hook_region(std::uint32_t begin,std::uint32_t size,std::string* error){
    uc_hook handle=0;
    const auto rc=uc_hook_add(engine(uc_),&handle,UC_HOOK_CODE,reinterpret_cast<void*>(&X86Guest::code_hook),this,begin,static_cast<std::uint64_t>(begin)+size-1);
    if(rc!=UC_ERR_OK)return fail(error,std::string("uc_hook_add: ")+uc_strerror(rc));
    hook_regions_.push_back({begin,size});uc_ctl_flush_tb(engine(uc_));return true;
}
bool X86Guest::add_override(std::uint32_t address,Override override,std::string* error){
    if(overrides_.count(address))return fail(error,"override already bound");
    for(const auto& [begin,size]:hook_regions_)if(address>=begin&&address-begin<size){overrides_[address]=std::move(override);return true;} // the region hook serves it
    uc_hook handle=0;
    const auto rc=uc_hook_add(engine(uc_),&handle,UC_HOOK_CODE,reinterpret_cast<void*>(&X86Guest::code_hook),this,address,address);
    if(rc!=UC_ERR_OK)return fail(error,std::string("uc_hook_add: ")+uc_strerror(rc));
    overrides_[address]=std::move(override);handles_.push_back(static_cast<std::uint32_t>(handle));
    uc_ctl_flush_tb(engine(uc_)); // blocks translated before the hook existed would bypass it
    return true;
}
GuestCallResult X86Guest::run(std::uint32_t from,std::uint32_t until,std::uint16_t control_word,std::uint64_t timeout){
    GuestCallResult r;
    const std::uint32_t esp=stack_base_+stack_size_-0x1000-4;write(esp,&stop_address_,4);
    set_reg(UC_X86_REG_ESP,esp);set_reg(UC_X86_REG_EBP,esp);
    std::uint32_t cw=control_word,sw=0,tag=0xffff;uc_reg_write(engine(uc_),UC_X86_REG_FPCW,&cw);uc_reg_write(engine(uc_),UC_X86_REG_FPSW,&sw);uc_reg_write(engine(uc_),UC_X86_REG_FPTAG,&tag);
    stopped_.reset();
    const auto rc=uc_emu_start(engine(uc_),from,until,timeout,0);
    r.eax=reg(UC_X86_REG_EAX);r.eip=reg(UC_X86_REG_EIP);r.esp=reg(UC_X86_REG_ESP);
    if(stopped_){r.error=*stopped_;stopped_.reset();return r;}
    if(rc!=UC_ERR_OK){r.error=uc_strerror(rc);return r;}
    r.completed=r.eip==until;
    if(!r.completed)r.error="stopped at the time limit";
    return r;
}
GuestCallResult X86Guest::call(std::uint32_t address,std::initializer_list<std::uint32_t> arguments,std::optional<std::uint32_t> ecx,std::optional<std::uint32_t> edx,std::uint16_t control_word,std::uint64_t timeout){
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
    const auto rc=uc_emu_start(engine(uc_),address,stop_address_,timeout,0);
    r.eax=reg(UC_X86_REG_EAX);r.eip=reg(UC_X86_REG_EIP);r.esp=reg(UC_X86_REG_ESP);
    if(stopped_){r.error=*stopped_;stopped_.reset();return r;}
    if(rc!=UC_ERR_OK){r.error=uc_strerror(rc);return r;}
    r.completed=r.eip==stop_address_;
    if(!r.completed)r.error="stopped at the time limit";
    return r;
}
} // namespace sadx
