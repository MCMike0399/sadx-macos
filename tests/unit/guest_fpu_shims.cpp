// M1 4e: the FPU shims. Every confirmed FPATAN/FSINCOS/FPTAN/FSIN/FCOS site of sonic.exe
// (reference/inventory/fpu-sites.json) is executed in the guest — a stub loads ST(1)/ST(0) with random canonical
// ext80 operands (game-range angles, large and tiny values, ±0, ±inf, NaN) under a random control word and jumps
// to the site; the guest stops right after the instruction — and the registers, TOP and C2 must equal the native
// Bochs model computed directly. Then 7716df (D3DXMatrixPerspectiveFovLH) runs with the shims on and must equal
// the native port bit for bit, including fovy = ±inf, where Unicorn's own FSINCOS disagreed (d3dx_perspective
// oracle, primary profile). F2XM1/FYL2X/RCP/RSQRT sites are hooked as counted, unshimmed.
//   guest-fpu-shims-test SONIC.EXE FPU_SITES_JSON
#include "guest/fpu_shims.hpp"
#include "cpu/x87_transcendental.hpp"
#include "math/d3dx_math.hpp"
#include <unicorn/unicorn.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
struct Ext {std::uint64_t sig;std::uint16_t se;};
Ext ext_of(double d){ // exact FLD m64
    std::uint64_t b;std::memcpy(&b,&d,8);const std::uint16_t s=(b>>63)?0x8000:0;const int e=static_cast<int>((b>>52)&0x7ff);std::uint64_t m=b&0xfffffffffffffULL;
    if(e==0x7ff)return {(m<<11)|(1ULL<<63),static_cast<std::uint16_t>(s|0x7fff)};
    if(!e){if(!m)return {0,s};int k=0;while(!(m&(1ULL<<52))){m<<=1;++k;}return {m<<11,static_cast<std::uint16_t>(s|(15361-k))};}
    return {(m|(1ULL<<52))<<11,static_cast<std::uint16_t>(s|(e+15360))};}
}
int main(int argc,char** argv){try{
    require(argc==3,"usage: guest-fpu-shims-test SONIC.EXE FPU_SITES_JSON");
    std::ifstream f(argv[1],std::ios::binary);std::string error;
    const auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&error);require(bool(image),error);
    const auto sites=load_fpu_sites(argv[2]);std::map<std::string,std::size_t> kinds;for(const auto& s:sites)++kinds[s.instruction];
    require(kinds["FPATAN"]==542&&kinds["FSINCOS"]==10&&kinds["FPTAN"]==3&&kinds["FSIN"]==5&&kinds["FCOS"]==1,"the confirmed site list (fpu_sites.py)");
    auto g=X86Guest::create(&error);require(g!=nullptr,error);require(g->map_image(*image,&error),error);
    FpuShimStats stats;require(install_fpu_shims(*g,sites,stats,&error),error);
    // stub at 0x10000000: fld tbyte [0x10000800]; fld tbyte [0x10000810]; jmp site   (ST1 = first, ST0 = second)
    require(g->map(0x10000000,0x1000,true,&error),error);
    std::mt19937_64 rng(0xf5);std::uniform_real_distribution<double> angle(-7,7),big(-1e18,1e18),tiny(-1e-30,1e-30);
    const auto operand=[&]()->double{switch(rng()%12){case 0:return big(rng);case 1:return tiny(rng);case 2:return 0.0;case 3:return -0.0;
        case 4:return std::numeric_limits<double>::infinity();case 5:return -std::numeric_limits<double>::infinity();case 6:return std::nan("");default:return angle(rng);}};
    std::size_t checked=0;
    for(const auto& site:sites){
        if(site.instruction!="FPATAN"&&site.instruction!="FSINCOS"&&site.instruction!="FPTAN"&&site.instruction!="FSIN"&&site.instruction!="FCOS")continue;
        for(int n=0;n<4;++n){
            const auto cw=static_cast<std::uint16_t>(std::initializer_list<int>{0x7f,0x27f,0x37f,0xb7f,0x77f}.begin()[rng()%5]);
            const Ext a=ext_of(operand()),b=ext_of(operand());
            std::uint8_t stub[17]={0xdb,0x2d,0x00,0x08,0x00,0x10,0xdb,0x2d,0x10,0x08,0x00,0x10,0xe9};
            const auto rel=site.address-(0x10000000+17);std::memcpy(stub+13,&rel,4);g->write(0x10000000,stub,17);
            g->write(0x10000800,&a.sig,8);g->write(0x10000808,&a.se,2);g->write(0x10000810,&b.sig,8);g->write(0x10000818,&b.se,2);
            const auto r=g->run(0x10000000,site.address+2,cw);if(!r)std::cerr<<"n "<<n<<" eip "<<std::hex<<r.eip<<" a "<<a.se<<":"<<a.sig<<" b "<<b.se<<":"<<b.sig<<" cw "<<cw<<std::dec<<"\n";require(bool(r),site.instruction+" at "+std::to_string(site.address)+": "+r.error);
            // the native model on the same operands
            X87Context fp(cw);const auto x=fp.from_canonical_ext80(b.se,b.sig);
            const auto top=static_cast<unsigned>((g->fpu_status()>>11)&7);const bool c2=(g->fpu_status()&0x400)!=0;
            if(site.instruction=="FPATAN"){const auto y=fp.from_canonical_ext80(a.se,a.sig);const auto v=X87Transcendental::fpatan_bochs_p5_p6(fp,y,x);
                require(top==7&&g->st(0)==std::make_pair(v.significand(),v.sign_exponent()),"FPATAN at "+std::to_string(site.address));}
            else if(site.instruction=="FSINCOS"){const auto v=X87Transcendental::fsincos_bochs(fp,x);
                if(v)require(!c2&&top==5&&g->st(0)==std::make_pair(v->cosine.significand(),v->cosine.sign_exponent())&&g->st(1)==std::make_pair(v->sine.significand(),v->sine.sign_exponent()),"FSINCOS at "+std::to_string(site.address));
                else require(c2&&top==6,"FSINCOS C2 path at "+std::to_string(site.address));}
            else if(site.instruction=="FPTAN"){const auto v=X87Transcendental::fptan_bochs(fp,x);
                if(v)require(!c2&&top==5&&g->st(0)==std::make_pair(std::uint64_t(1)<<63,std::uint16_t(0x3fff))&&g->st(1)==std::make_pair(v->significand(),v->sign_exponent()),"FPTAN at "+std::to_string(site.address));
                else require(c2&&top==6,"FPTAN C2 path at "+std::to_string(site.address));}
            else{const auto v=site.instruction=="FSIN"?X87Transcendental::fsin_bochs(fp,x):X87Transcendental::fcos_bochs(fp,x);
                if(v)require(!c2&&top==6&&g->st(0)==std::make_pair(v->significand(),v->sign_exponent()),site.instruction+" at "+std::to_string(site.address));
                else require(c2&&top==6,site.instruction+" C2 path at "+std::to_string(site.address));}
            ++checked;}
    }
    // 7716df end to end with the shims (its FSINCOS at 771706 is a confirmed site).
    std::size_t perspective=0;
    for(int n=0;n<3000;++n){
        const auto cw=static_cast<std::uint16_t>(std::initializer_list<int>{0x7f,0x27f,0x37f,0xc7f}.begin()[rng()%4]);
        float v[4]={static_cast<float>(angle(rng)),std::uniform_real_distribution<float>(0.2F,3)(rng),std::uniform_real_distribution<float>(0.01F,10)(rng),std::uniform_real_distribution<float>(10,1e5F)(rng)};
        if(n%50==0)v[0]=n%100?std::numeric_limits<float>::infinity():-std::numeric_limits<float>::infinity();
        std::uint32_t w[4];std::memcpy(w,v,16);
        const auto r=g->call(0x7716df,{0x10000900,w[0],w[1],w[2],w[3]},std::nullopt,std::nullopt,cw);require(bool(r),"7716df: "+r.error);
        X87Context fp(cw);const auto native=d3dx_perspective_7716df(v[0],v[1],v[2],v[3],fp);require(bool(native),"the native port");
        std::uint32_t out[16];g->read(0x10000900,out,64);
        for(int k=0;k<16;++k)require(out[k]==(*native)[k],"7716df with the shims differs from the native port at element "+std::to_string(k));
        ++perspective;}
    std::cout<<"Guest FPU shims: "<<sites.size()<<" confirmed sites hooked; "<<checked<<" executions at every FPATAN/FSINCOS/FPTAN/FSIN/FCOS site equal the Bochs model; "
             <<"7716df equals the native port on "<<perspective<<" cases including fovy = +-inf (shimmed FSINCOS "<<stats.shimmed["FSINCOS"]<<"x)\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
