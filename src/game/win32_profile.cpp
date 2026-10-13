#include "game/win32_profile.hpp"
#include <cctype>
namespace sadx {
namespace {
std::string lower(std::string s){for(auto& c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return s;}
std::string trim(const std::string& s){
    std::size_t a=0,b=s.size();
    while(a<b&&(s[a]==' '||s[a]=='\t'))++a;
    while(b>a&&(s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r'))--b;
    return s.substr(a,b-a);}
}
Win32Profile::Win32Profile(const std::vector<std::uint8_t>& text){
    std::string section,line;
    const auto finish=[&]{
        const auto t=trim(line);line.clear();
        if(t.empty()||t[0]==';')return;
        if(t[0]=='['){const auto close=t.find(']');section=trim(t.substr(1,close==std::string::npos?std::string::npos:close-1));return;}
        const auto eq=t.find('=');if(eq==std::string::npos)return;
        Entry e{section,trim(t.substr(0,eq)),trim(t.substr(eq+1))};
        const auto& v=e.value;std::size_t i=(!v.empty()&&(v[0]=='+'||v[0]=='-'))?1:0;
        if(!v.empty()&&(v[0]=='"'||v[0]=='\''))unsupported.push_back(e.section+"."+e.key+": quoted value");
        if(v.size()>i+1&&v[i]=='0'&&(v[i+1]=='x'||v[i+1]=='X'))unsupported.push_back(e.section+"."+e.key+": hexadecimal value");
        std::size_t digits=0;while(i+digits<v.size()&&std::isdigit(static_cast<unsigned char>(v[i+digits])))++digits;
        if(digits>9)unsupported.push_back(e.section+"."+e.key+": value may overflow 32 bits");
        entries_.push_back(std::move(e));};
    for(const auto b:text){if(b=='\n')finish();else line.push_back(static_cast<char>(b));}
    finish();
}
std::optional<std::string> Win32Profile::value(const std::string& section,const std::string& key)const{
    const auto s=lower(section),k=lower(key);
    for(const auto& e:entries_)if(lower(e.section)==s&&lower(e.key)==k)return e.value;
    return std::nullopt;
}
std::int32_t Win32Profile::get_int(const std::string& section,const std::string& key,std::int32_t fallback)const{
    const auto v=value(section,key);if(!v)return fallback;
    std::size_t i=0;while(i<v->size()&&((*v)[i]==' '||(*v)[i]=='\t'))++i;
    bool negative=false;if(i<v->size()&&((*v)[i]=='+'||(*v)[i]=='-')){negative=(*v)[i]=='-';++i;}
    std::uint32_t n=0;while(i<v->size()&&std::isdigit(static_cast<unsigned char>((*v)[i]))){n=n*10U+static_cast<std::uint32_t>((*v)[i]-'0');++i;}
    return static_cast<std::int32_t>(negative?0U-n:n);
}
std::string Win32Profile::get_string(const std::string& section,const std::string& key,const std::string& fallback,std::uint32_t size)const{
    auto v=value(section,key).value_or(fallback);
    if(size==0)return {};
    if(v.size()>size-1)v.resize(size-1);
    return v;
}
} // namespace sadx
