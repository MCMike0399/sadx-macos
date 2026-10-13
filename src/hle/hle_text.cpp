#include "hle/hle_text.hpp"
namespace sadx {
namespace {
constexpr std::uint16_t high1252[32]={0x20AC,0x0081,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,0x0160,0x2039,0x0152,0x008D,0x017D,0x008F,
                                      0x0090,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,0x0161,0x203A,0x0153,0x009D,0x017E,0x0178};
}
std::uint16_t cp1252_to_utf16(std::uint8_t b){return b>=0x80&&b<0xa0?high1252[b-0x80]:b;}
std::optional<std::uint8_t> utf16_to_cp1252(std::uint16_t u){
    if(u<0x80||(u>=0xa0&&u<=0xff))return static_cast<std::uint8_t>(u);
    for(unsigned i=0;i<32;++i)if(high1252[i]==u)return static_cast<std::uint8_t>(0x80+i);
    return std::nullopt;}
std::optional<std::vector<std::uint16_t>> utf8_to_utf16(const std::vector<std::uint8_t>& s){
    std::vector<std::uint16_t> out;
    for(std::size_t i=0;i<s.size();){
        const auto b=s[i];std::uint32_t cp;std::size_t n;
        if(b<0x80){cp=b;n=1;}else if((b&0xe0)==0xc0){cp=b&0x1f;n=2;}else if((b&0xf0)==0xe0){cp=b&0x0f;n=3;}else if((b&0xf8)==0xf0){cp=b&0x07;n=4;}else return std::nullopt;
        if(i+n>s.size())return std::nullopt;
        for(std::size_t k=1;k<n;++k){if((s[i+k]&0xc0)!=0x80)return std::nullopt;cp=(cp<<6)|(s[i+k]&0x3f);}
        if(cp>=0x10000){cp-=0x10000;out.push_back(static_cast<std::uint16_t>(0xd800+(cp>>10)));out.push_back(static_cast<std::uint16_t>(0xdc00+(cp&0x3ff)));}
        else out.push_back(static_cast<std::uint16_t>(cp));
        i+=n;}
    return out;}
std::vector<std::uint8_t> utf16_to_utf8(const std::vector<std::uint16_t>& s){
    std::vector<std::uint8_t> out;
    for(std::size_t i=0;i<s.size();++i){
        std::uint32_t cp=s[i];
        if(cp>=0xd800&&cp<0xdc00&&i+1<s.size()&&s[i+1]>=0xdc00&&s[i+1]<0xe000){cp=0x10000+((cp-0xd800)<<10)+(s[i+1]-0xdc00);++i;}
        if(cp<0x80)out.push_back(static_cast<std::uint8_t>(cp));
        else if(cp<0x800){out.push_back(static_cast<std::uint8_t>(0xc0|(cp>>6)));out.push_back(static_cast<std::uint8_t>(0x80|(cp&0x3f)));}
        else if(cp<0x10000){out.push_back(static_cast<std::uint8_t>(0xe0|(cp>>12)));out.push_back(static_cast<std::uint8_t>(0x80|((cp>>6)&0x3f)));out.push_back(static_cast<std::uint8_t>(0x80|(cp&0x3f)));}
        else{out.push_back(static_cast<std::uint8_t>(0xf0|(cp>>18)));out.push_back(static_cast<std::uint8_t>(0x80|((cp>>12)&0x3f)));out.push_back(static_cast<std::uint8_t>(0x80|((cp>>6)&0x3f)));out.push_back(static_cast<std::uint8_t>(0x80|(cp&0x3f)));}}
    return out;}
std::uint16_t ctype1(std::uint16_t u){
    if(u<0x80){
        std::uint16_t t=0;const int c=u;
        if(c>='A'&&c<='Z')t|=0x101;if(c>='a'&&c<='z')t|=0x102;if(c>='0'&&c<='9')t|=0x4;
        if(c==' '||(c>=9&&c<=13))t|=0x8;if(c<32||c==127)t|=0x20;if(c==' '||c=='\t')t|=0x40;
        if((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))t|=0x80;
        if(c>32&&c<127&&!(t&0x107))t|=0x10;
        return t;}
    if(u<0xa0)return 0x20;                                   // C1 controls
    if(u==0xa0)return 0x48;                                  // no-break space: SPACE|BLANK
    if(u==0xd7||u==0xf7)return 0x10;
    if((u>=0xc0&&u<=0xde)||u==0x160||u==0x152||u==0x17d||u==0x178)return 0x101;
    if((u>=0xdf&&u<=0xff)||u==0xaa||u==0xb5||u==0xba||u==0x161||u==0x153||u==0x17e||u==0x192)return 0x102;
    if(u==0xb2||u==0xb3||u==0xb9)return 0x14;                // superscript digits: DIGIT|PUNCT
    return 0x10;                                             // the rest of Latin-1 and the 1252 extras: punctuation
}
std::uint16_t to_lower16(std::uint16_t u){
    if((u>='A'&&u<='Z')||(u>=0xc0&&u<=0xde&&u!=0xd7))return static_cast<std::uint16_t>(u+32);
    if(u==0x160||u==0x152||u==0x17d)return static_cast<std::uint16_t>(u+1);if(u==0x178)return 0xff;return u;}
std::uint16_t to_upper16(std::uint16_t u){
    if((u>='a'&&u<='z')||(u>=0xe0&&u<=0xfe&&u!=0xf7))return static_cast<std::uint16_t>(u-32);
    if(u==0x161||u==0x153||u==0x17e)return static_cast<std::uint16_t>(u-1);if(u==0xff)return 0x178;return u;}
} // namespace sadx
