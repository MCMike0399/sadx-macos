#include "hle/hle_printf.hpp"
#include "hle/hle_text.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
namespace sadx {
namespace {
struct Args {
    const X86Guest& g;std::uint32_t at;
    std::optional<std::uint32_t> u32(){const auto v=g.u32(at);at+=4;return v;}
    std::optional<std::uint64_t> u64(){const auto lo=g.u32(at),hi=g.u32(at+4);at+=8;if(!lo||!hi)return std::nullopt;return (std::uint64_t(*hi)<<32)|*lo;}
};
std::vector<std::uint16_t> widen(const std::string& s){std::vector<std::uint16_t> w;for(const unsigned char c:s)w.push_back(c);return w;}
}
std::optional<std::vector<std::uint16_t>> format_guest(const X86Guest& g,const std::vector<std::uint16_t>& f,std::uint32_t va,bool wide,std::string* error){
    std::vector<std::uint16_t> out;Args a{g,va};
    const auto bad=[&](const std::string& why)->std::optional<std::vector<std::uint16_t>>{if(error)*error=why;return std::nullopt;};
    for(std::size_t i=0;i<f.size();++i){
        if(f[i]!='%'){out.push_back(f[i]);continue;}
        if(++i>=f.size())break;
        if(f[i]=='%'){out.push_back('%');continue;}
        std::string flags;while(i<f.size()&&std::strchr("-+ #0",static_cast<char>(f[i]))&&f[i]<128){flags.push_back(static_cast<char>(f[i]));++i;}
        std::optional<int> width,precision;
        if(i<f.size()&&f[i]=='*'){const auto w=a.u32();if(!w)return bad("unreadable * width");auto v=static_cast<std::int32_t>(*w);if(v<0){flags.push_back('-');v=-v;}width=v;++i;}
        else{int v=0;bool any=false;while(i<f.size()&&f[i]>='0'&&f[i]<='9'){v=v*10+(f[i]-'0');++i;any=true;}if(any)width=v;}
        if(i<f.size()&&f[i]=='.'){++i;if(i<f.size()&&f[i]=='*'){const auto p=a.u32();if(!p)return bad("unreadable * precision");const auto v=static_cast<std::int32_t>(*p);if(v>=0)precision=v;++i;}
            else{int v=0;while(i<f.size()&&f[i]>='0'&&f[i]<='9'){v=v*10+(f[i]-'0');++i;}precision=v;}}
        // length
        int bits=32;bool hh=false,h=false,longdouble=false;int text=0; // text: 0 default, 1 narrow, 2 wide
        while(i<f.size()){
            const auto c=f[i];
            if(c=='h'){if(i+1<f.size()&&f[i+1]=='h'){hh=true;++i;}else{h=true;text=1;}++i;}
            else if(c=='l'){if(i+1<f.size()&&f[i+1]=='l'){bits=64;++i;}else text=2;++i;}
            else if(c=='w'){text=2;++i;}
            else if(c=='L'){longdouble=true;++i;}
            else if(c=='I'){if(i+2<f.size()&&f[i+1]=='6'&&f[i+2]=='4'){bits=64;i+=3;}else if(i+2<f.size()&&f[i+1]=='3'&&f[i+2]=='2'){bits=32;i+=3;}else ++i;}
            else if(c=='j'){bits=64;++i;}
            else if(c=='z'||c=='t'){bits=32;++i;}
            else break;}
        if(i>=f.size())return bad("truncated conversion");
        const auto conv=static_cast<char>(f[i]);
        std::string spec="%"+flags;if(width)spec+=std::to_string(*width);if(precision)spec+="."+std::to_string(*precision);
        char buffer[512];
        switch(conv){
        case 'd':case 'i':case 'u':case 'o':case 'x':case 'X':{
            std::uint64_t v=0;if(bits==64){const auto x=a.u64();if(!x)return bad("unreadable integer");v=*x;}else{const auto x=a.u32();if(!x)return bad("unreadable integer");v=*x;}
            const bool sign=conv=='d'||conv=='i';
            if(bits!=64){if(hh)v&=0xff;else if(h)v&=0xffff;
                if(sign){std::int64_t s=hh?static_cast<std::int8_t>(v):h?static_cast<std::int16_t>(v):static_cast<std::int32_t>(v);v=static_cast<std::uint64_t>(s);}}
            spec+="ll";spec.push_back(conv);
            if(sign)std::snprintf(buffer,sizeof buffer,spec.c_str(),static_cast<long long>(v));else std::snprintf(buffer,sizeof buffer,spec.c_str(),static_cast<unsigned long long>(v));
            const auto w=widen(buffer);out.insert(out.end(),w.begin(),w.end());break;}
        case 'p':{const auto x=a.u32();if(!x)return bad("unreadable pointer");std::snprintf(buffer,sizeof buffer,"%08X",*x);const auto w=widen(buffer);out.insert(out.end(),w.begin(),w.end());break;}
        case 'c':case 'C':{
            const auto x=a.u32();if(!x)return bad("unreadable character");
            const bool wide_char=text==2||(text==0&&((conv=='c')==wide));
            const std::uint16_t ch=wide_char?static_cast<std::uint16_t>(*x):cp1252_to_utf16(static_cast<std::uint8_t>(*x));
            std::vector<std::uint16_t> s{ch};const int pad=width&&*width>1?*width-1:0;
            if(flags.find('-')==std::string::npos)out.insert(out.end(),static_cast<std::size_t>(pad),' ');out.push_back(ch);if(flags.find('-')!=std::string::npos)out.insert(out.end(),static_cast<std::size_t>(pad),' ');
            break;}
        case 's':case 'S':{
            const auto x=a.u32();if(!x)return bad("unreadable string pointer");
            const bool wide_string=text==2||(text==0&&((conv=='s')==wide));
            std::vector<std::uint16_t> s;
            if(!*x){for(const char c:std::string("(null)"))s.push_back(static_cast<std::uint16_t>(c));}
            else for(std::uint32_t k=0;!precision||k<static_cast<std::uint32_t>(*precision);++k){
                if(wide_string){std::uint16_t c;if(!g.read(*x+2*k,&c,2))return bad("unreadable string");if(!c)break;s.push_back(c);}
                else{std::uint8_t c;if(!g.read(*x+k,&c,1))return bad("unreadable string");if(!c)break;s.push_back(cp1252_to_utf16(c));}}
            const int pad=width&&*width>static_cast<int>(s.size())?*width-static_cast<int>(s.size()):0;
            if(flags.find('-')==std::string::npos)out.insert(out.end(),static_cast<std::size_t>(pad),' ');out.insert(out.end(),s.begin(),s.end());if(flags.find('-')!=std::string::npos)out.insert(out.end(),static_cast<std::size_t>(pad),' ');
            break;}
        case 'e':case 'E':case 'f':case 'F':case 'g':case 'G':case 'a':case 'A':{
            if(longdouble){} // long double is double on MSVC
            const auto x=a.u64();if(!x)return bad("unreadable double");double d;std::memcpy(&d,&*x,8);
            if(std::isnan(d)||std::isinf(d))return bad("printf of inf/nan: the UCRT's spelling is not modelled");
            spec.push_back(conv);std::snprintf(buffer,sizeof buffer,spec.c_str(),d);const auto w=widen(buffer);out.insert(out.end(),w.begin(),w.end());break;}
        case 'n':return bad("%n is disabled in the UCRT");
        default:return bad(std::string("unsupported conversion %")+conv);
        }
    }
    return out;
}
} // namespace sadx
