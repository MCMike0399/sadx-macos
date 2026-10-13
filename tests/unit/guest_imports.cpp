// M1 step 4a: import resolution. sonic.exe's import table is parsed by ExecutableImage::imports(), every
// import is bound to a trap in the reserved page, registered HLE functions run (and are logged) when the
// guest calls them, and an unregistered import stops the guest with its name.
//   guest-imports-test SONIC.EXE
#include "hle/hle_registry.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {using namespace sadx;void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}}
int main(int argc,char** argv){try{
    require(argc==2,"usage: guest-imports-test SONIC.EXE");
    std::ifstream f(argv[1],std::ios::binary);require(bool(f),"cannot read sonic.exe");
    std::string error;auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    // The table as tools/lib/pe_image.py reads it: 9 DLLs, 159 imports, DSOUND by ordinal 11.
    const auto modules=image->imports(&error);require(bool(modules),error);
    std::size_t count=0;for(const auto& m:*modules)count+=m.symbols.size();
    require(modules->size()==9&&count==159,"9 DLLs, 159 imports");
    require((*modules)[0].dll=="KERNEL32.dll"&&(*modules)[0].symbols[0].name==std::string("CompareStringW")&&(*modules)[0].symbols[0].iat_va==0x7db02c,"KERNEL32 first thunk");
    require((*modules)[1].dll=="USER32.dll"&&(*modules)[1].symbols[0].name==std::string("ShowCursor")&&(*modules)[1].symbols[0].iat_va==0x7db1dc,"USER32 ShowCursor slot 7db1dc");
    require((*modules)[5].dll=="DSOUND.dll"&&!(*modules)[5].symbols[0].name&&(*modules)[5].symbols[0].ordinal==11,"DSOUND ordinal 11");

    auto guest=X86Guest::create(&error);require(guest!=nullptr,error);require(guest->map_image(*image,&error),error);
    HleRegistry hle;int cursor=2;
    hle.add("user32.dll","ShowCursor",{4,1,[&](HleCall& c){cursor+=c.argument(0).value_or(0)?1:-1;return static_cast<std::uint32_t>(cursor);}});
    const auto bound=bind_imports(*guest,*image,hle);require(bool(bound),bound.error);
    require(bound.bindings.size()==159&&bound.bound==1&&bound.unbound==158,"every import has a trap; one is registered");
    for(const auto& b:bound.bindings)require(guest->u32(b.iat_va)==b.trap&&b.trap>=hle_trap_base&&b.trap<hle_trap_base+159*16,"IAT slot holds its trap: "+b.dll+"!"+b.symbol);

    // WinMain 402c9b..402ca9: mov esi,[7db1dc]; push 0; call esi; test eax,eax; jg 402ca1 (ShowCursor(FALSE) until <= 0).
    const auto r=guest->run(0x402c9b,0x402ca9);require(bool(r),"ShowCursor loop: "+r.error);
    require(hle.log().size()==2&&cursor==0,"two ShowCursor(FALSE) calls: 1, then 0 ends the loop");
    for(const auto& rec:hle.log())require(rec.dll=="USER32.dll"&&rec.symbol=="ShowCursor"&&rec.arguments==std::vector<std::uint32_t>{0}&&rec.return_address==0x402ca5,"logged call record");
    require(hle.log()[0].result==1&&hle.log()[1].result==0,"logged results");

    // An unregistered import is an explicit failure naming it: call [7db01c] (DSOUND #11) from a scratch page.
    require(guest->map(0x10000000,0x1000,true,&error),error);
    const std::uint8_t code[]={0xff,0x15,0x1c,0xb0,0x7d,0x00,0xc3};guest->write(0x10000000,code,sizeof code);
    const auto u=guest->call(0x10000000,{});
    require(!u&&u.error.find("unbound import DSOUND.dll!#11")!=std::string::npos&&u.error.find("10000006")!=std::string::npos,"unbound import stops with its name: "+u.error);
    std::cout<<"Guest imports: 159 imports of 9 DLLs bound to traps; WinMain's ShowCursor loop ran through HLE (2 logged calls); an unbound import stops with its name.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
