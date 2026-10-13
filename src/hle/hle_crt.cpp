// The Universal CRT (api-ms-win-crt-*) and VCRUNTIME140 for the guest's MSVC 2015+ DLLs: startup
// (_initterm, onexit tables, argv/environment), the heap, memory/string primitives and errno. C++
// exceptions, SEH unwinding and process exit are not modelled: they stop the guest with their name.
#include "hle/hle_process.hpp"
#include "hle/hle_printf.hpp"
#include "hle/hle_text.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
namespace sadx {
namespace {
constexpr const char* runtime="api-ms-win-crt-runtime-l1-1-0.dll";
constexpr const char* heap="api-ms-win-crt-heap-l1-1-0.dll";
constexpr const char* strings="api-ms-win-crt-string-l1-1-0.dll";
constexpr const char* vcruntime="VCRUNTIME140.dll";
// Calls each non-null function pointer in [begin, end) in the guest (cdecl, no arguments); with
// `stop_on_nonzero` (_initterm_e) the first nonzero return ends the walk and is the result.
void initterm(HleCall& c,std::uint32_t begin,std::uint32_t end,bool stop_on_nonzero){
    auto targets=std::make_shared<std::vector<std::uint32_t>>();
    for(std::uint32_t at=begin;at<end;at+=4){const auto f=c.guest.u32(at).value_or(0);if(f)targets->push_back(f);}
    auto next=std::make_shared<std::size_t>(0);
    auto step=std::make_shared<std::function<void(X86Guest&)>>();
    *step=[targets,next,step,stop_on_nonzero](X86Guest& g){
        if(*next>=targets->size()){g.set_eax(0);return;}
        const auto f=(*targets)[(*next)++];
        g.redirect(f,{},0,[step,stop_on_nonzero](X86Guest& g2){
            if(stop_on_nonzero&&g2.reg(19)){return;} // EAX (UC_X86_REG_EAX) nonzero: _initterm_e returns it
            (*step)(g2);});};
    if(targets->empty()){c.guest.set_eax(0);return;}
    (*step)(c.guest);
}
}
// One byte through Windows text-mode translation (CRLF -> LF, Ctrl-Z ends the file); binary mode raw.
int stream_getc(HleFile& f){
    if(!f.pushback.empty()){const auto c=f.pushback.back();f.pushback.pop_back();return c;}
    if(!f.host||!f.readable||f.eof){f.eof=true;return EOF;}
    int c=std::fgetc(f.host);
    if(c==EOF){f.eof=true;return EOF;}
    if(f.text){
        if(c==0x1a){f.eof=true;return EOF;}
        if(c=='\r'){const int n=std::fgetc(f.host);if(n=='\n')return '\n';if(n!=EOF)std::ungetc(n,f.host);}
    }
    return c;
}
bool stream_putc(HleFile& f,int c){
    if(!f.host||!f.writable)return false;
    if(f.text&&c=='\n'&&std::fputc('\r',f.host)==EOF)return false;
    return std::fputc(c,f.host)!=EOF;
}
void register_hle_stdio(HleProcess& p);
void register_hle_crt_locale(HleProcess& p);
void register_hle_crt(HleProcess& p){
    register_hle_stdio(p);
    auto& r=p.hle();auto* P=&p;
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    const auto C=[&](const char* dll,const char* name,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add(dll,name,{0,logged,std::move(body)});}; // cdecl
    // --- startup
    C(runtime,"_initterm",2,[arg](HleCall& c)->std::uint32_t{initterm(c,arg(c,0),arg(c,1),false);return c.guest.reg(19);});
    C(runtime,"_initterm_e",2,[arg](HleCall& c)->std::uint32_t{initterm(c,arg(c,0),arg(c,1),true);return c.guest.reg(19);});
    C(runtime,"_initialize_onexit_table",1,[P,arg](HleCall& c)->std::uint32_t{
        const std::uint32_t z[3]={0,0,0};c.guest.write(arg(c,0),z,12);P->onexit_tables[arg(c,0)].clear();return 0;});
    C(runtime,"_register_onexit_function",2,[P,arg](HleCall& c)->std::uint32_t{P->onexit_tables[arg(c,0)].push_back(arg(c,1));return 0;});
    C(runtime,"_crt_atexit",1,[P,arg](HleCall& c)->std::uint32_t{P->onexit_tables[0].push_back(arg(c,0));return 0;});
    C(runtime,"_execute_onexit_table",1,[](HleCall& c)->std::uint32_t{c.guest.stop("_execute_onexit_table: process/DLL teardown is not modelled");return 0;});
    C(runtime,"_cexit",0,[](HleCall& c)->std::uint32_t{c.guest.stop("_cexit: process teardown is not modelled");return 0;});
    C(runtime,"exit",1,[arg](HleCall& c)->std::uint32_t{c.guest.stop("exit("+std::to_string(arg(c,0))+")");return 0;});
    C(runtime,"terminate",0,[](HleCall& c)->std::uint32_t{c.guest.stop("terminate()");return 0;});
    C(runtime,"_invalid_parameter_noinfo",0,[](HleCall& c)->std::uint32_t{c.guest.stop("_invalid_parameter_noinfo: a CRT function rejected its arguments");return 0;});
    C(runtime,"_invalid_parameter_noinfo_noreturn",0,[](HleCall& c)->std::uint32_t{c.guest.stop("_invalid_parameter_noinfo_noreturn: a CRT function rejected its arguments");return 0;});
    C(runtime,"_seh_filter_dll",2,[](HleCall& c)->std::uint32_t{c.guest.stop("_seh_filter_dll: a structured exception reached a DLL entry point");return 0;});
    C(runtime,"_configure_narrow_argv",1,[P](HleCall&)->std::uint32_t{
        if(!P->narrow_argv){const auto exe=P->allocate_string(P->modules().windows_dir()+"\\sonic.exe").value_or(0);P->narrow_argv=P->memory().allocate(8).value_or(0);
            const std::uint32_t v[2]={exe,0};P->guest().write(P->narrow_argv,v,8);}
        return 0;});
    C(runtime,"_initialize_narrow_environment",0,[P](HleCall&)->std::uint32_t{
        P->assume("the process environment block is empty (the reference environment was not captured)");
        if(!P->environment){P->environment=P->memory().allocate(4).value_or(0);}
        return 0;});
    C(runtime,"_errno",0,[P](HleCall&)->std::uint32_t{if(!P->errno_address)P->errno_address=P->memory().allocate(4).value_or(0);return P->errno_address;});
    // --- heap
    C(heap,"malloc",1,[P,arg](HleCall& c)->std::uint32_t{return P->memory().allocate(arg(c,0)).value_or(0);});
    C(heap,"calloc",2,[P,arg](HleCall& c)->std::uint32_t{
        const std::uint64_t n=std::uint64_t(arg(c,0))*arg(c,1);if(n>0xffffffffULL)return 0;return P->memory().allocate(static_cast<std::uint32_t>(n)).value_or(0);});
    C(heap,"free",1,[P,arg](HleCall& c)->std::uint32_t{if(arg(c,0)&&!P->memory().free(arg(c,0)))c.guest.stop("free() of a block the heap did not hand out");return 0;});
    C(heap,"_callnewh",1,[](HleCall&)->std::uint32_t{return 0;}); // no new handler installed: operator new then throws bad_alloc
    // --- VCRUNTIME140 memory/string primitives (on guest memory)
    C(vcruntime,"memcpy",3,[arg](HleCall& c)->std::uint32_t{std::vector<std::uint8_t> b(arg(c,2));c.guest.read(arg(c,1),b.data(),b.size());c.guest.write(arg(c,0),b.data(),b.size());return arg(c,0);});
    C(vcruntime,"memmove",3,[arg](HleCall& c)->std::uint32_t{std::vector<std::uint8_t> b(arg(c,2));c.guest.read(arg(c,1),b.data(),b.size());c.guest.write(arg(c,0),b.data(),b.size());return arg(c,0);});
    C(vcruntime,"memset",3,[arg](HleCall& c)->std::uint32_t{std::vector<std::uint8_t> b(arg(c,2),static_cast<std::uint8_t>(arg(c,1)));c.guest.write(arg(c,0),b.data(),b.size());return arg(c,0);});
    C(vcruntime,"memchr",3,[arg](HleCall& c)->std::uint32_t{
        std::vector<std::uint8_t> b(arg(c,2));c.guest.read(arg(c,0),b.data(),b.size());
        const auto i=std::find(b.begin(),b.end(),static_cast<std::uint8_t>(arg(c,1)));return i==b.end()?0:arg(c,0)+static_cast<std::uint32_t>(i-b.begin());});
    C(vcruntime,"strrchr",2,[P,arg](HleCall& c)->std::uint32_t{
        const auto s=P->string_at(arg(c,0));if(!s)return 0;const char ch=static_cast<char>(arg(c,1));
        if(!ch)return arg(c,0)+static_cast<std::uint32_t>(s->size());const auto at=s->rfind(ch);return at==std::string::npos?0:arg(c,0)+static_cast<std::uint32_t>(at);});
    C(vcruntime,"strstr",2,[P,arg](HleCall& c)->std::uint32_t{
        const auto s=P->string_at(arg(c,0)),t=P->string_at(arg(c,1));if(!s||!t)return 0;const auto at=s->find(*t);return at==std::string::npos?0:arg(c,0)+static_cast<std::uint32_t>(at);});
    C(vcruntime,"__std_type_info_destroy_list",1,[](HleCall&)->std::uint32_t{return 0;});
    for(const char* name:{"_CxxThrowException","__CxxFrameHandler","__CxxFrameHandler3","_except_handler4_common","__std_terminate","longjmp"})
        C(vcruntime,name,0,[name](HleCall& c)->std::uint32_t{c.guest.stop(std::string(name)+": C++ exceptions / SEH / longjmp are not modelled in the guest");return 0;});
    // --- string.h / ctype (api-ms-win-crt-string)
    C(strings,"strncmp",3,[P,arg](HleCall& c)->std::uint32_t{
        const auto a=P->string_at(arg(c,0)).value_or(""),b=P->string_at(arg(c,1)).value_or("");return static_cast<std::uint32_t>(std::strncmp(a.c_str(),b.c_str(),arg(c,2)));});
    C(strings,"_stricmp",2,[P,arg](HleCall& c)->std::uint32_t{
        auto a=P->string_at(arg(c,0)).value_or(""),b=P->string_at(arg(c,1)).value_or("");
        for(auto* s:{&a,&b})std::transform(s->begin(),s->end(),s->begin(),[](unsigned char ch){return static_cast<char>(std::tolower(ch));});
        return static_cast<std::uint32_t>(a<b?-1:a>b?1:0);});
    C(strings,"strnlen",2,[arg](HleCall& c)->std::uint32_t{std::uint32_t n=0;for(;n<arg(c,1);++n){char ch;if(!c.guest.read(arg(c,0)+n,&ch,1)||!ch)break;}return n;});
    C(strings,"_strdup",1,[P,arg](HleCall& c)->std::uint32_t{if(!arg(c,0))return 0;const auto s=P->string_at(arg(c,0));return s?P->allocate_string(*s).value_or(0):0;});
    C(strings,"tolower",1,[arg](HleCall& c)->std::uint32_t{const auto ch=arg(c,0);return ch<128?static_cast<std::uint32_t>(std::tolower(static_cast<int>(ch))):ch;});
    C(strings,"towlower",1,[arg](HleCall& c)->std::uint32_t{const auto ch=arg(c,0)&0xffff;return ch<128?static_cast<std::uint32_t>(std::tolower(static_cast<int>(ch))):ch;});
    C(strings,"isdigit",1,[arg](HleCall& c)->std::uint32_t{const auto ch=arg(c,0);return ch<128&&std::isdigit(static_cast<int>(ch))?4:0;}); // _DIGIT
    C(strings,"isspace",1,[arg](HleCall& c)->std::uint32_t{const auto ch=arg(c,0);return ch<128&&std::isspace(static_cast<int>(ch))?8:0;}); // _SPACE
    C(strings,"strcpy_s",3,[P,arg](HleCall& c)->std::uint32_t{
        const auto s=P->string_at(arg(c,2));if(!s||!arg(c,0)||s->size()+1>arg(c,1)){c.guest.stop("strcpy_s: invalid parameter");return 22;}
        P->put_string(arg(c,0),*s);return 0;});
    C(strings,"strcat_s",3,[P,arg](HleCall& c)->std::uint32_t{
        const auto d=P->string_at(arg(c,0)),s=P->string_at(arg(c,2));if(!d||!s||d->size()+s->size()+1>arg(c,1)){c.guest.stop("strcat_s: invalid parameter");return 22;}
        P->put_string(arg(c,0),*d+*s);return 0;});
    C(strings,"strncpy",3,[P,arg](HleCall& c)->std::uint32_t{
        const auto s=P->string_at(arg(c,1)).value_or("");std::vector<char> b(arg(c,2),0);std::copy_n(s.begin(),std::min<std::size_t>(s.size(),b.size()),b.begin());
        c.guest.write(arg(c,0),b.data(),b.size());return arg(c,0);});
    register_hle_crt_locale(p); // last: it aliases msvcrt.dll to bodies registered above
}
} // namespace sadx

namespace sadx {
void register_hle_stdio(HleProcess& p){
    auto& r=p.hle();auto* P=&p;
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    constexpr const char* stdio="api-ms-win-crt-stdio-l1-1-0.dll";
    const auto C=[&](const char* name,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add(stdio,name,{0,logged,std::move(body)});};
    const auto new_stream=[P](HleFile f)->std::uint32_t{
        const auto at=P->memory().allocate(0x38).value_or(0);if(!at)return 0; // UCRT __crt_stdio_stream_data; _ptr/_base/_cnt stay 0 (no exposed buffer)
        const std::uint32_t fd=static_cast<std::uint32_t>(P->files.size()+3);P->guest().write(at+0x10,&fd,4);P->files[at]=std::move(f);return at;};
    C("__acrt_iob_func",1,[P,arg,new_stream](HleCall& c)->std::uint32_t{
        if(!P->iob_address){P->assume("stdin/stdout/stderr are unattached (a GUI-subsystem process without a console): reads see EOF, writes fail");
            P->iob_address=new_stream({});new_stream({});new_stream({});}
        const auto i=arg(c,0);if(i>2){c.guest.stop("__acrt_iob_func("+std::to_string(i)+")");return 0;}
        // The three standard streams were created consecutively: find the i-th.
        std::uint32_t n=0;for(const auto& [at,f]:P->files){(void)f;if(at>=P->iob_address&&n++==i)return at;}return 0;});
    const auto open=[P,new_stream](const std::string& name,const std::string& mode)->std::uint32_t{
        P->trace_file("fopen["+mode+"]",name,P->host_path(name)?P->host_path(name)->string():"not found");
        if(mode.empty()){return 0;}
        const bool plus=mode.find('+')!=std::string::npos,binary=mode.find('b')!=std::string::npos;
        if(mode.find("ccs=")!=std::string::npos)return 0xffffffffU; // encoded streams: caller stops
        HleFile f;f.windows_path=name;f.text=!binary;
        if(mode[0]=='r'){const auto host=P->host_path(name);if(!host||std::filesystem::is_directory(*host))return 0;
            f.host=std::fopen(host->string().c_str(),plus?"r+b":"rb");f.readable=true;f.writable=plus;}
        else if(mode[0]=='w'||mode[0]=='a'){const auto host=P->write_path(name);if(!host)return 0;
            f.host=std::fopen(host->string().c_str(),mode[0]=='w'?(plus?"w+b":"wb"):(plus?"a+b":"ab"));f.writable=true;f.readable=plus;}
        else return 0;
        if(!f.host)return 0;
        return new_stream(std::move(f));};
    const auto set_errno=[P](std::uint32_t e){if(!P->errno_address)P->errno_address=P->memory().allocate(4).value_or(0);P->guest().write(P->errno_address,&e,4);};
    C("fopen",2,[P,arg,open,set_errno](HleCall& c)->std::uint32_t{
        const auto name=P->string_at(arg(c,0)),mode=P->string_at(arg(c,1));if(!name||!mode){c.guest.stop("fopen: unreadable arguments");return 0;}
        const auto f=open(*name,*mode);if(f==0xffffffffU){c.guest.stop("fopen with ccs= encoding is not implemented");return 0;}if(!f)set_errno(2);return f;});
    C("_wfopen",2,[P,arg,open,set_errno](HleCall& c)->std::uint32_t{
        const auto name=P->wide_at(arg(c,0)),mode=P->wide_at(arg(c,1));if(!name||!mode){c.guest.stop("_wfopen: unreadable or non-ASCII arguments");return 0;}
        const auto f=open(*name,*mode);if(f==0xffffffffU){c.guest.stop("_wfopen with ccs= encoding is not implemented");return 0;}if(!f)set_errno(2);return f;});
    // _fsopen/_wfsopen: fopen with a share flag (one process: sharing never conflicts).
    C("_fsopen",3,[P,arg,open,set_errno](HleCall& c)->std::uint32_t{
        const auto name=P->string_at(arg(c,0)),mode=P->string_at(arg(c,1));if(!name||!mode){c.guest.stop("_fsopen: unreadable arguments");return 0;}
        const auto f=open(*name,*mode);if(f==0xffffffffU){c.guest.stop("_fsopen with ccs= encoding is not implemented");return 0;}if(!f)set_errno(2);return f;});
    C("_wfsopen",3,[P,arg,open,set_errno](HleCall& c)->std::uint32_t{
        const auto name=P->wide_at(arg(c,0)),mode=P->wide_at(arg(c,1));if(!name||!mode){c.guest.stop("_wfsopen: unreadable or non-ASCII arguments");return 0;}
        const auto f=open(*name,*mode);if(f==0xffffffffU){c.guest.stop("_wfsopen with ccs= encoding is not implemented");return 0;}if(!f)set_errno(2);return f;});
    C("fopen_s",3,[P,arg,open](HleCall& c)->std::uint32_t{
        const auto name=P->string_at(arg(c,1)),mode=P->string_at(arg(c,2));if(!arg(c,0)||!name||!mode){c.guest.stop("fopen_s: invalid parameter");return 22;}
        const auto f=open(*name,*mode);if(f==0xffffffffU){c.guest.stop("fopen_s with ccs= encoding is not implemented");return 22;}
        c.guest.set_u32(arg(c,0),f);return f?0:2;});
    const auto stream=[P](HleCall& c,std::uint32_t at)->HleFile*{const auto i=P->files.find(at);if(i==P->files.end()){c.guest.stop("stdio on an unknown FILE*");return nullptr;}return &i->second;};
    C("fclose",1,[P,arg](HleCall& c)->std::uint32_t{
        const auto i=P->files.find(arg(c,0));if(i==P->files.end()){c.guest.stop("fclose of an unknown FILE*");return EOF;}
        if(i->second.host)std::fclose(i->second.host);P->memory().free(arg(c,0));P->files.erase(i);return 0;});
    const auto read=[](HleCall& c,HleFile& f,std::uint32_t buffer,std::uint64_t size,std::uint64_t count)->std::uint32_t{
        if(!size||!count)return 0;std::uint64_t done=0;
        for(;done<count;++done){std::vector<std::uint8_t> item(static_cast<std::size_t>(size));std::size_t k=0;
            for(;k<item.size();++k){const int ch=stream_getc(f);if(ch==EOF)break;item[k]=static_cast<std::uint8_t>(ch);}
            c.guest.write(buffer+static_cast<std::uint32_t>(done*size),item.data(),k);if(k<item.size())break;}
        return static_cast<std::uint32_t>(done);};
    C("fread",4,[arg,stream,read](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,3));return f?read(c,*f,arg(c,0),arg(c,1),arg(c,2)):0;});
    C("fread_s",5,[arg,stream,read](HleCall& c)->std::uint32_t{
        auto* f=stream(c,arg(c,4));if(!f)return 0;if(std::uint64_t(arg(c,2))*arg(c,3)>arg(c,1)){c.guest.stop("fread_s: buffer too small");return 0;}return read(c,*f,arg(c,0),arg(c,2),arg(c,3));});
    C("fwrite",4,[arg,stream](HleCall& c)->std::uint32_t{
        auto* f=stream(c,arg(c,3));if(!f)return 0;const std::uint64_t size=arg(c,1),count=arg(c,2);if(!size||!count)return 0;
        std::vector<std::uint8_t> b(static_cast<std::size_t>(size*count));c.guest.read(arg(c,0),b.data(),b.size());
        std::size_t k=0;for(;k<b.size();++k)if(!stream_putc(*f,b[k])){f->error=true;break;}return static_cast<std::uint32_t>(k/size);});
    const auto seek=[](HleFile& f,std::int64_t offset,std::uint32_t origin)->std::uint32_t{
        if(!f.host)return static_cast<std::uint32_t>(-1);f.pushback.clear();f.eof=false;
        return std::fseek(f.host,static_cast<long>(offset),origin==0?SEEK_SET:origin==1?SEEK_CUR:SEEK_END)?static_cast<std::uint32_t>(-1):0;};
    C("fseek",3,[arg,stream,seek](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));return f?seek(*f,static_cast<std::int32_t>(arg(c,1)),arg(c,2)):static_cast<std::uint32_t>(-1);});
    C("_fseeki64",4,[arg,stream,seek](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));return f?seek(*f,static_cast<std::int64_t>((std::uint64_t(arg(c,2))<<32)|arg(c,1)),arg(c,3)):static_cast<std::uint32_t>(-1);});
    C("ftell",1,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));if(!f||!f->host)return static_cast<std::uint32_t>(-1);return static_cast<std::uint32_t>(std::ftell(f->host)-static_cast<long>(f->pushback.size()));});
    C("fgetpos",2,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));if(!f||!f->host)return static_cast<std::uint32_t>(-1);
        const std::int64_t at=std::ftell(f->host)-static_cast<long>(f->pushback.size());c.guest.write(arg(c,1),&at,8);return 0;});
    C("fsetpos",2,[arg,stream,seek](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));if(!f)return static_cast<std::uint32_t>(-1);std::int64_t at=0;c.guest.read(arg(c,1),&at,8);return seek(*f,at,0);});
    C("feof",1,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));return f&&f->eof&&f->pushback.empty()?0x10:0;});
    C("fflush",1,[P,arg](HleCall& c)->std::uint32_t{if(!arg(c,0)){for(auto& [at,f]:P->files){(void)at;if(f.host)std::fflush(f.host);}return 0;}
        const auto i=P->files.find(arg(c,0));if(i==P->files.end()){c.guest.stop("fflush of an unknown FILE*");return EOF;}if(i->second.host)std::fflush(i->second.host);return 0;});
    C("fgetc",1,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));return f?static_cast<std::uint32_t>(stream_getc(*f)):static_cast<std::uint32_t>(EOF);});
    C("ungetc",2,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,1));const auto ch=static_cast<std::int32_t>(arg(c,0));if(!f||ch==EOF)return static_cast<std::uint32_t>(EOF);
        f->pushback.push_back(ch&0xff);f->eof=false;return static_cast<std::uint32_t>(ch&0xff);});
    C("fgets",3,[arg,stream](HleCall& c)->std::uint32_t{
        auto* f=stream(c,arg(c,2));const auto n=static_cast<std::int32_t>(arg(c,1));if(!f||n<=0)return 0;
        std::string s;while(static_cast<std::int32_t>(s.size())<n-1){const int ch=stream_getc(*f);if(ch==EOF)break;s.push_back(static_cast<char>(ch));if(ch=='\n')break;}
        if(s.empty())return 0;c.guest.write(arg(c,0),s.c_str(),s.size()+1);return arg(c,0);});
    C("fputc",2,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,1));if(!f||!stream_putc(*f,static_cast<int>(arg(c,0)&0xff)))return static_cast<std::uint32_t>(EOF);return arg(c,0)&0xff;});
    C("fputs",2,[P,arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,1));const auto s=P->string_at(arg(c,0));if(!f||!s)return static_cast<std::uint32_t>(EOF);
        for(const char ch:*s)if(!stream_putc(*f,static_cast<unsigned char>(ch)))return static_cast<std::uint32_t>(EOF);return 0;});
    // Wide characters on a byte stream without ccs=: two bytes per wchar_t, as the UCRT's binary/ANSI streams do.
    C("fgetwc",1,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,0));if(!f)return 0xffff;
        const int lo=stream_getc(*f);if(lo==EOF)return 0xffff;const int hi=stream_getc(*f);if(hi==EOF)return 0xffff;return static_cast<std::uint32_t>(lo|(hi<<8));});
    C("fputwc",2,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,1));const auto ch=arg(c,0)&0xffff;
        if(!f||!stream_putc(*f,static_cast<int>(ch&0xff))||!stream_putc(*f,static_cast<int>(ch>>8)))return 0xffff;return ch;});
    C("ungetwc",2,[arg,stream](HleCall& c)->std::uint32_t{auto* f=stream(c,arg(c,1));const auto ch=arg(c,0)&0xffff;if(!f||ch==0xffff)return 0xffff;
        f->pushback.push_back(static_cast<int>(ch>>8));f->pushback.push_back(static_cast<int>(ch&0xff));f->eof=false;return ch;});
    // The current directory is the install (where the game is started from).
    const auto getcwd=[P,arg](bool wide){return [P,arg,wide](HleCall& c)->std::uint32_t{
        const auto dir=P->modules().windows_dir();auto buffer=arg(c,0);const auto size=arg(c,1);
        if(!buffer){buffer=P->memory().allocate(static_cast<std::uint32_t>(std::max<std::size_t>(size,dir.size()+1)*(wide?2:1))).value_or(0);}
        else if(dir.size()+1>size)return 0; // ERANGE
        if(wide)P->put_wide(buffer,dir);else P->put_string(buffer,dir);return buffer;};};
    C("_getcwd",2,getcwd(false));r.add("api-ms-win-crt-environment-l1-1-0.dll","_wgetcwd",{0,2,getcwd(true)});
    // msvcrt.dll (SDL2 links the old system CRT): the same bodies, its own FILE array _iob (32-byte
    // MSVCRT FILEs; stdin/stdout/stderr unattached), vfprintf through the same formatter.
    r.add_data("msvcrt.dll","_iob",[P]()->std::optional<std::uint32_t>{
        const auto at=P->memory().allocate(3*32);if(!at)return std::nullopt;
        for(std::uint32_t i=0;i<3;++i){const std::uint32_t fd=i;P->guest().write(*at+32*i+16,&fd,4);P->files[*at+32*i]={};}return at;});
    C("vfprintf",3,[P,arg,stream](HleCall& c)->std::uint32_t{
        auto* f=stream(c,arg(c,0));if(!f)return static_cast<std::uint32_t>(-1);
        std::vector<std::uint16_t> fmt;for(std::uint32_t i=0;;++i){std::uint8_t ch=0;if(!c.guest.read(arg(c,1)+i,&ch,1)){c.guest.stop("vfprintf: unreadable format");return 0;}if(!ch)break;fmt.push_back(cp1252_to_utf16(ch));}
        std::string error;const auto text=format_guest(c.guest,fmt,arg(c,2),false,&error);if(!text){c.guest.stop("vfprintf: "+error);return 0;}
        for(const auto u:*text)if(!stream_putc(*f,utf16_to_cp1252(u).value_or('?')))return static_cast<std::uint32_t>(-1);
        (void)P;return static_cast<std::uint32_t>(text->size());});
    C("setvbuf",4,[](HleCall&)->std::uint32_t{return 0;});
    C("_lock_file",1,[](HleCall&)->std::uint32_t{return 0;});C("_unlock_file",1,[](HleCall&)->std::uint32_t{return 0;});
    // UCRT's own lock entry points live in the filesystem set as well.
    r.add("api-ms-win-crt-filesystem-l1-1-0.dll","_lock_file",{0,1,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("api-ms-win-crt-filesystem-l1-1-0.dll","_unlock_file",{0,1,[](HleCall&)->std::uint32_t{return 0;}});
    C("_get_stream_buffer_pointers",4,[arg](HleCall& c)->std::uint32_t{
        const auto f=arg(c,0);if(arg(c,1))c.guest.set_u32(arg(c,1),f+4);if(arg(c,2))c.guest.set_u32(arg(c,2),f);if(arg(c,3))c.guest.set_u32(arg(c,3),f+8);return 0;});
    // __stdio_common_vsprintf(options lo, hi, buffer, count, format, locale, va_list) and the _s / wide forms.
    const auto vsprintf=[arg](bool wide,bool secure){return [arg,wide,secure](HleCall& c)->std::uint32_t{
        const auto options=arg(c,0),buffer=arg(c,2),count=arg(c,3),format=arg(c,4),va=arg(c,6);
        std::vector<std::uint16_t> f;const auto unit=wide?2U:1U;
        for(std::uint32_t i=0;;++i){std::uint16_t ch=0;if(!c.guest.read(format+unit*i,&ch,unit)){c.guest.stop("vsprintf: unreadable format");return 0;}if(!ch)break;f.push_back(wide?ch:cp1252_to_utf16(static_cast<std::uint8_t>(ch)));}
        std::string error;const auto text=format_guest(c.guest,f,va,wide,&error);if(!text){c.guest.stop("vsprintf: "+error);return 0;}
        const auto n=static_cast<std::uint32_t>(text->size());
        const auto put=[&](std::uint32_t k){for(std::uint32_t i=0;i<k;++i){if(wide)c.guest.write(buffer+2*i,&(*text)[i],2);else{const auto b=utf16_to_cp1252((*text)[i]).value_or(static_cast<std::uint8_t>('?'));c.guest.write(buffer+i,&b,1);}}};
        const auto nul=[&](std::uint32_t at){const std::uint16_t z=0;c.guest.write(buffer+unit*at,&z,unit);};
        if(!buffer&&!count)return n;
        if(secure){if(n+1>count){c.guest.stop("vsprintf_s: buffer too small (the UCRT calls the invalid-parameter handler)");return static_cast<std::uint32_t>(-1);}put(n);nul(n);return n;}
        if(options&2){if(count){const auto k=std::min(n,count-1);put(k);nul(k);}return n;} // standard snprintf behaviour
        if(n<count){put(n);nul(n);return n;}
        put(count);if(options&1&&count)nul(count-1);return n==count?n:static_cast<std::uint32_t>(-1);};};
    C("__stdio_common_vsprintf",7,vsprintf(false,false));C("__stdio_common_vsprintf_s",7,vsprintf(false,true));
    C("__stdio_common_vswprintf",7,vsprintf(true,false));C("__stdio_common_vswprintf_s",7,vsprintf(true,true));
}
} // namespace sadx
namespace sadx {
namespace {
std::uint64_t double_bits(double d){std::uint64_t b;std::memcpy(&b,&d,8);return b;}
// A double as the x87 holds it after FLD m64 (exact): ext80 fields.
std::pair<std::uint64_t,std::uint16_t> to_ext80(double d){
    const auto b=double_bits(d);const std::uint16_t sign=(b>>63)?0x8000:0;const auto e=static_cast<int>((b>>52)&0x7ff);std::uint64_t m=b&0xfffffffffffffULL;
    if(e==0x7ff)return {(m<<11)|(1ULL<<63),static_cast<std::uint16_t>(sign|0x7fff)};
    if(e==0){if(!m)return {0,sign};int shift=0;while(!(m&(1ULL<<52))){m<<=1;++shift;}return {m<<11,static_cast<std::uint16_t>(sign|(15361-shift+1-1))};}
    return {(m|(1ULL<<52))<<11,static_cast<std::uint16_t>(sign|(e-1023+16383))};}
}
void register_hle_crt_locale(HleProcess& p){
    auto& r=p.hle();auto* P=&p;
    const auto arg=[](HleCall& c,unsigned i){return c.argument(i).value_or(0);};
    const auto C=[&](const char* dll,const char* name,unsigned logged,std::function<std::uint32_t(HleCall&)> body){r.add(dll,name,{0,logged,std::move(body)});};
    constexpr const char* locale="api-ms-win-crt-locale-l1-1-0.dll";constexpr const char* strings="api-ms-win-crt-string-l1-1-0.dll";
    constexpr const char* convert="api-ms-win-crt-convert-l1-1-0.dll";constexpr const char* math="api-ms-win-crt-math-l1-1-0.dll";
    constexpr const char* runtime="api-ms-win-crt-runtime-l1-1-0.dll";constexpr const char* time="api-ms-win-crt-time-l1-1-0.dll";
    // The "C" locale the UCRT starts in (nothing in the guest calls setlocale with a name yet).
    auto data=std::make_shared<std::map<std::string,std::uint32_t>>();
    const auto block=[P,data](const std::string& key,const std::vector<std::uint8_t>& bytes)->std::uint32_t{
        auto& at=(*data)[key];if(!at){at=P->memory().allocate(static_cast<std::uint32_t>(bytes.size())).value_or(0);P->guest().write(at,bytes.data(),bytes.size());}return at;};
    const auto string_block=[block](const std::string& s){return block("s:"+s,std::vector<std::uint8_t>(s.c_str(),s.c_str()+s.size()+1));};
    const auto wide_block=[block](const std::string& s){std::vector<std::uint8_t> b;for(const char ch:s){b.push_back(static_cast<std::uint8_t>(ch));b.push_back(0);}b.push_back(0);b.push_back(0);return block("w:"+s,b);};
    C(locale,"_lock_locales",0,[](HleCall&)->std::uint32_t{return 0;});C(locale,"_unlock_locales",0,[](HleCall&)->std::uint32_t{return 0;});
    C(locale,"___lc_codepage_func",0,[P](HleCall&)->std::uint32_t{P->assume("the CRT runs in the \"C\" locale (lc_codepage 0, mb_cur_max 1, the ASCII ctype table)");return 0;});
    C(locale,"___lc_collate_cp_func",0,[](HleCall&)->std::uint32_t{return 0;});
    C(locale,"___mb_cur_max_func",0,[](HleCall&)->std::uint32_t{return 1;});
    C(locale,"___lc_locale_name_func",0,[block](HleCall&)->std::uint32_t{return block("lc_names",std::vector<std::uint8_t>(24,0));});
    C(locale,"__pctype_func",0,[block](HleCall&)->std::uint32_t{
        std::vector<std::uint8_t> t(2*257,0);
        for(unsigned ch=0;ch<128;++ch){const auto v=static_cast<std::uint16_t>(ctype1(static_cast<std::uint16_t>(ch))&0x1ff);std::memcpy(&t[2*(ch+1)],&v,2);}
        return block("pctype",t)+2;}); // entry -1 (EOF) precedes the returned pointer
    C(locale,"setlocale",2,[P,arg,string_block](HleCall& c)->std::uint32_t{
        if(arg(c,1)){const auto s=P->string_at(arg(c,1)).value_or("?");if(s!="C"){c.guest.stop("setlocale(\""+s+"\") is not implemented");return 0;}}
        return string_block("C");});
    C(locale,"_wsetlocale",2,[P,arg,wide_block](HleCall& c)->std::uint32_t{
        if(arg(c,1)){const auto s=P->wide_at(arg(c,1)).value_or("?");if(s!="C"){c.guest.stop("_wsetlocale(L\""+s+"\") is not implemented");return 0;}}
        return wide_block("C");});
    C(locale,"localeconv",0,[block,string_block,wide_block](HleCall&)->std::uint32_t{
        std::vector<std::uint8_t> b(0x50,0);
        const auto put=[&](unsigned off,std::uint32_t v){std::memcpy(&b[off],&v,4);};
        put(0,string_block("."));for(unsigned k=1;k<10;++k)put(4*k,string_block(""));
        for(unsigned k=0;k<8;++k)b[40+k]=127; // CHAR_MAX
        put(48,wide_block("."));for(unsigned k=1;k<8;++k)put(48+4*k,wide_block(""));
        return block("lconv",b);});
    const auto names=[block](const std::string& key,const std::string& s){return [block,key,s](HleCall& c)->std::uint32_t{(void)c;return block(key,std::vector<std::uint8_t>(s.c_str(),s.c_str()+s.size()+1));};};
    C(time,"_Getdays",0,names("days",":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday"));
    C(time,"_Getmonths",0,names("months",":Jan:January:Feb:February:Mar:March:Apr:April:May:May:Jun:June:Jul:July:Aug:August:Sep:September:Oct:October:Nov:November:Dec:December"));
    for(const char* n:{"_Gettnames","_Strftime","_W_Getdays","_W_Getmonths","_W_Gettnames","_Wcsftime"})
        C(time,n,0,[n](HleCall& c)->std::uint32_t{c.guest.stop(std::string(n)+": the time-facet locale data is not implemented");return 0;});
    // string.h
    C(strings,"__strncnt",2,[arg](HleCall& c)->std::uint32_t{std::uint32_t n=0;for(;n<arg(c,1);++n){char ch;if(!c.guest.read(arg(c,0)+n,&ch,1)||!ch)break;}return n;});
    C(strings,"wcsnlen",2,[arg](HleCall& c)->std::uint32_t{std::uint32_t n=0;for(;n<arg(c,1);++n){std::uint16_t ch;if(!c.guest.read(arg(c,0)+2*n,&ch,2)||!ch)break;}return n;});
    C(strings,"islower",1,[arg](HleCall& c)->std::uint32_t{const auto ch=arg(c,0);return ch<128&&ch>='a'&&ch<='z'?2:0;});
    C(strings,"isupper",1,[arg](HleCall& c)->std::uint32_t{const auto ch=arg(c,0);return ch<128&&ch>='A'&&ch<='Z'?1:0;});
    C(strings,"strcspn",2,[P,arg](HleCall& c)->std::uint32_t{const auto s=P->string_at(arg(c,0)).value_or(""),set=P->string_at(arg(c,1)).value_or("");const auto at=s.find_first_of(set);return static_cast<std::uint32_t>(at==std::string::npos?s.size():at);});
    C(strings,"_wcsdup",1,[P,arg](HleCall& c)->std::uint32_t{
        std::vector<std::uint16_t> w;for(std::uint32_t i=0;;++i){std::uint16_t ch=0;if(!c.guest.read(arg(c,0)+2*i,&ch,2))return 0;w.push_back(ch);if(!ch)break;}
        const auto at=P->memory().allocate(static_cast<std::uint32_t>(w.size()*2)).value_or(0);if(at)c.guest.write(at,w.data(),w.size()*2);return at;});
    C(strings,"wcscpy_s",3,[arg](HleCall& c)->std::uint32_t{
        std::vector<std::uint16_t> w;for(std::uint32_t i=0;;++i){std::uint16_t ch=0;if(!c.guest.read(arg(c,2)+2*i,&ch,2))return 22;w.push_back(ch);if(!ch)break;}
        if(w.size()>arg(c,1)){c.guest.stop("wcscpy_s: buffer too small");return 34;}c.guest.write(arg(c,0),w.data(),w.size()*2);return 0;});
    // stdlib conversions in the "C" locale, through the host's (IEEE double, correctly rounded) parser.
    const auto parse_double=[P](HleCall& c,std::uint32_t at,std::uint32_t end_out,bool wide)->double{
        std::string s;for(std::uint32_t i=0;i<256;++i){std::uint16_t ch=0;if(!c.guest.read(at+(wide?2*i:i),&ch,wide?2:1)||!ch)break;s.push_back(ch<128?static_cast<char>(ch):'\x01');}
        char* end=nullptr;const double d=std::strtod(s.c_str(),&end);
        if(end_out){const auto used=static_cast<std::uint32_t>(end-s.c_str());c.guest.set_u32(end_out,at+(wide?2*used:used));}(void)P;return d;};
    C(convert,"strtod",2,[arg,parse_double](HleCall& c)->std::uint32_t{const auto e=to_ext80(parse_double(c,arg(c,0),arg(c,1),false));c.guest.return_x87(e.first,e.second);return 0;});
    C(convert,"wcstod",2,[arg,parse_double](HleCall& c)->std::uint32_t{const auto e=to_ext80(parse_double(c,arg(c,0),arg(c,1),true));c.guest.return_x87(e.first,e.second);return 0;});
    C(convert,"strtof",2,[arg](HleCall& c)->std::uint32_t{
        std::string s;for(std::uint32_t i=0;i<256;++i){char ch=0;if(!c.guest.read(arg(c,0)+i,&ch,1)||!ch)break;s.push_back(ch);}
        char* end=nullptr;const float f=std::strtof(s.c_str(),&end);if(arg(c,1))c.guest.set_u32(arg(c,1),arg(c,0)+static_cast<std::uint32_t>(end-s.c_str()));
        const auto e=to_ext80(f);c.guest.return_x87(e.first,e.second);return 0;});
    C(convert,"wcstof",2,[arg](HleCall& c)->std::uint32_t{
        std::string s;for(std::uint32_t i=0;i<256;++i){std::uint16_t ch=0;if(!c.guest.read(arg(c,0)+2*i,&ch,2)||!ch)break;s.push_back(ch<128?static_cast<char>(ch):'\x01');}
        char* end=nullptr;const float f=std::strtof(s.c_str(),&end);if(arg(c,1))c.guest.set_u32(arg(c,1),arg(c,0)+2*static_cast<std::uint32_t>(end-s.c_str()));
        const auto e=to_ext80(f);c.guest.return_x87(e.first,e.second);return 0;});
    const auto integer=[P,arg](bool wide,bool is_unsigned,bool sixty_four){return [P,arg,wide,is_unsigned,sixty_four](HleCall& c)->std::uint32_t{
        std::string s;for(std::uint32_t i=0;i<256;++i){std::uint16_t ch=0;if(!c.guest.read(arg(c,0)+(wide?2*i:i),&ch,wide?2:1)||!ch)break;s.push_back(ch<128?static_cast<char>(ch):'\x01');}
        char* end=nullptr;const int base=static_cast<int>(arg(c,2));errno=0;
        std::uint64_t v;if(is_unsigned)v=std::strtoull(s.c_str(),&end,base);else v=static_cast<std::uint64_t>(std::strtoll(s.c_str(),&end,base));
        if(!sixty_four){ // clamp like strtol/strtoul (32-bit long on Windows)
            if(is_unsigned){if(v>0xffffffffULL&&static_cast<std::int64_t>(v)>=0)v=0xffffffffULL;}
            else{const auto sv=static_cast<std::int64_t>(v);if(sv>0x7fffffffLL)v=0x7fffffffULL;else if(sv<-0x80000000LL)v=static_cast<std::uint64_t>(-0x80000000LL);}}
        if(arg(c,1))c.guest.set_u32(arg(c,1),arg(c,0)+(wide?2U:1U)*static_cast<std::uint32_t>(end-s.c_str()));
        if(sixty_four)c.guest.set_edx(static_cast<std::uint32_t>(v>>32));(void)P;return static_cast<std::uint32_t>(v);};};
    C(convert,"strtol",3,integer(false,false,false));C(convert,"strtoul",3,integer(false,true,false));C(convert,"wcstol",3,integer(true,false,false));
    C(convert,"strtoll",3,integer(false,false,true));C(convert,"strtoull",3,integer(false,true,true));
    C(convert,"atoi",1,[P,arg](HleCall& c)->std::uint32_t{return static_cast<std::uint32_t>(std::atoi(P->string_at(arg(c,0)).value_or("").c_str()));});
    C(convert,"atol",1,[P,arg](HleCall& c)->std::uint32_t{return static_cast<std::uint32_t>(std::atoi(P->string_at(arg(c,0)).value_or("").c_str()));});
    C(convert,"_wtoi",1,[P,arg](HleCall& c)->std::uint32_t{return static_cast<std::uint32_t>(std::atoi(P->wide_at(arg(c,0)).value_or("").c_str()));});
    // math.h pieces MSVCP140 imports (pure classification/sign functions on stack doubles).
    const auto stack_double=[arg](HleCall& c,unsigned i){std::uint64_t b=(std::uint64_t(arg(c,i+1))<<32)|arg(c,i);double d;std::memcpy(&d,&b,8);return d;};
    C(math,"_dclass",2,[stack_double](HleCall& c)->std::uint32_t{const auto d=stack_double(c,0);return std::isnan(d)?2:std::isinf(d)?1:d==0?0:std::fpclassify(d)==FP_SUBNORMAL?-2:-1;});
    C(math,"_dsign",2,[arg](HleCall& c)->std::uint32_t{return arg(c,1)&0x80000000U?0x8000:0;});
    C(math,"_fdclass",1,[arg](HleCall& c)->std::uint32_t{float f;const auto b=arg(c,0);std::memcpy(&f,&b,4);return std::isnan(f)?2:std::isinf(f)?1:f==0?0:std::fpclassify(f)==FP_SUBNORMAL?-2:-1;});
    C(math,"_fdsign",1,[arg](HleCall& c)->std::uint32_t{return arg(c,0)&0x80000000U?0x8000:0;});
    // libm. Exact operations (ceil, floor, copysign, sqrt, _ftol, classification) are bit-identical in any
    // correct implementation; the transcendental ones use the host libm (an assumption, reported).
    const auto ext80_to_double=[](std::pair<std::uint64_t,std::uint16_t> v)->double{
        const auto e=static_cast<int>(v.second&0x7fff);const bool neg=v.second&0x8000;
        if(e==0x7fff){return (v.first<<1)?std::numeric_limits<double>::quiet_NaN():(neg?-1.0:1.0)*std::numeric_limits<double>::infinity();}
        const double d=std::ldexp(static_cast<double>(v.first),e-16383-63);return neg?-d:d;}; // rounds the 64-bit significand to double (RN)
    const auto host_libm=[P](){P->assume("UCRT transcendental functions (pow, atan2, acos, asin, sin, cos) take their results from the host libm; the UCRT's own last-bit results were not captured");};
    const auto x87_unary=[ext80_to_double](std::function<double(double)> f){return [ext80_to_double,f](HleCall& c)->std::uint32_t{
        const double x=ext80_to_double(c.guest.st(0));c.guest.pop_x87();const auto e=to_ext80(f(x));c.guest.return_x87(e.first,e.second);return 0;};};
    const auto x87_binary=[ext80_to_double](std::function<double(double,double)> f){return [ext80_to_double,f](HleCall& c)->std::uint32_t{
        const double y=ext80_to_double(c.guest.st(0)),x=ext80_to_double(c.guest.st(1));c.guest.pop_x87();c.guest.pop_x87();
        const auto e=to_ext80(f(x,y));c.guest.return_x87(e.first,e.second);return 0;};}; // _CIpow(x=ST1, y=ST0), _CIatan2(y=ST1, x=ST0) pass (ST1, ST0)
    const auto stack_unary=[stack_double](std::function<double(double)> f){return [stack_double,f](HleCall& c)->std::uint32_t{const auto e=to_ext80(f(stack_double(c,0)));c.guest.return_x87(e.first,e.second);return 0;};};
    C(math,"ceil",2,stack_unary([](double x){return std::ceil(x);}));C(math,"floor",2,stack_unary([](double x){return std::floor(x);}));
    C(math,"copysign",4,[stack_double](HleCall& c)->std::uint32_t{const auto e=to_ext80(std::copysign(stack_double(c,0),stack_double(c,2)));c.guest.return_x87(e.first,e.second);return 0;});
    C(math,"_finite",2,[stack_double](HleCall& c)->std::uint32_t{return std::isfinite(stack_double(c,0))?1:0;});
    C(math,"_dtest",1,[arg](HleCall& c)->std::uint32_t{std::uint64_t b=0;c.guest.read(arg(c,0),&b,8);double d;std::memcpy(&d,&b,8);
        return std::isnan(d)?2:std::isinf(d)?1:d==0?0:std::fpclassify(d)==FP_SUBNORMAL?static_cast<std::uint32_t>(-2):static_cast<std::uint32_t>(-1);});
    C(math,"_ftol",0,[ext80_to_double](HleCall& c)->std::uint32_t{ // truncates ST0 toward zero into EDX:EAX, pops
        const auto v=c.guest.st(0);c.guest.pop_x87();const double d=ext80_to_double(v);
        const auto t=std::isfinite(d)&&std::fabs(d)<9.2e18?static_cast<std::int64_t>(d):std::int64_t(0x8000000000000000ULL);
        c.guest.set_edx(static_cast<std::uint32_t>(static_cast<std::uint64_t>(t)>>32));return static_cast<std::uint32_t>(t);});
    C(math,"_CIpow",0,[host_libm,x87_binary](HleCall& c){host_libm();return x87_binary([](double x,double y){return std::pow(x,y);})(c);});
    C(math,"_CIatan2",0,[host_libm,x87_binary](HleCall& c){host_libm();return x87_binary([](double y,double x){return std::atan2(y,x);})(c);});
    C(math,"_CIacos",0,[host_libm,x87_unary](HleCall& c){host_libm();return x87_unary([](double x){return std::acos(x);})(c);});
    const auto sse2=[](std::function<double(double)> f){return [f](HleCall& c)->std::uint32_t{double x;const auto b=c.guest.xmm0_low();std::memcpy(&x,&b,8);
        const double y=f(x);std::uint64_t o;std::memcpy(&o,&y,8);c.guest.set_xmm0_low(o);return 0;};};
    C(math,"_libm_sse2_sqrt_precise",0,sse2([](double x){return std::sqrt(x);}));
    C(math,"_libm_sse2_sin_precise",0,[host_libm,sse2](HleCall& c){host_libm();return sse2([](double x){return std::sin(x);})(c);});
    C(math,"_libm_sse2_cos_precise",0,[host_libm,sse2](HleCall& c){host_libm();return sse2([](double x){return std::cos(x);})(c);});
    C(math,"_libm_sse2_asin_precise",0,[host_libm,sse2](HleCall& c){host_libm();return sse2([](double x){return std::asin(x);})(c);});
    C(math,"_except1",0,[](HleCall& c)->std::uint32_t{c.guest.stop("_except1: a libm floating-point exception report");return 0;});
    // runtime odds and ends
    C(runtime,"_set_new_handler",1,[](HleCall&)->std::uint32_t{return 0;});
    C(runtime,"abort",0,[](HleCall& c)->std::uint32_t{c.guest.stop("abort()");return 0;});
    C(runtime,"_beginthreadex",6,[](HleCall& c)->std::uint32_t{c.guest.stop("_beginthreadex: guest threads are not modelled");return 0;});
    C("api-ms-win-crt-heap-l1-1-0.dll","realloc",2,[P,arg](HleCall& c)->std::uint32_t{
        if(!arg(c,0))return P->memory().allocate(arg(c,1)).value_or(0);if(!arg(c,1)){P->memory().free(arg(c,0));return 0;}return P->memory().reallocate(arg(c,0),arg(c,1)).value_or(0);});
    C("api-ms-win-crt-utility-l1-1-0.dll","rand_s",1,[P,arg](HleCall& c)->std::uint32_t{
        P->assume("rand_s: a fixed LCG sequence (the reference's RtlGenRandom output is random by design)");
        static std::uint32_t state=0x12345678U;state=state*1103515245U+12345U;c.guest.set_u32(arg(c,0),state);return 0;});
    (void)double_bits;
    C(strings,"strlen",1,[P,arg](HleCall& c)->std::uint32_t{const auto s=P->string_at(arg(c,0),1u<<20);if(!s){c.guest.stop("strlen: unterminated or unreadable string");return 0;}return static_cast<std::uint32_t>(s->size());});
    // msvcrt.dll aliases
    const std::pair<const char*,const char*> same[]={{runtime,"_initterm"},{runtime,"abort"},{"api-ms-win-crt-heap-l1-1-0.dll","calloc"},{"api-ms-win-crt-heap-l1-1-0.dll","free"},
        {"VCRUNTIME140.dll","memcpy"},{"api-ms-win-crt-heap-l1-1-0.dll","realloc"},{strings,"strlen"},{strings,"strncmp"},{"api-ms-win-crt-stdio-l1-1-0.dll","fputs"},
        {"api-ms-win-crt-stdio-l1-1-0.dll","vfprintf"},{"api-ms-win-crt-stdio-l1-1-0.dll","_lock_file"}};
    for(const auto& [dll,name]:same)r.alias("msvcrt.dll",name,dll,name);
    r.add("msvcrt.dll","_lock",{0,1,[](HleCall&)->std::uint32_t{return 0;}});r.add("msvcrt.dll","_unlock",{0,1,[](HleCall&)->std::uint32_t{return 0;}});
    r.add("msvcrt.dll","_amsg_exit",{0,1,[](HleCall& c)->std::uint32_t{c.guest.stop("_amsg_exit("+std::to_string(c.argument(0).value_or(0))+"): a CRT runtime error");return 0;}});
}
} // namespace sadx
