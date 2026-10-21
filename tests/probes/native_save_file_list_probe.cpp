#include "game/save_file_list.hpp"
#include <algorithm>
#include <array>
#include <cstring>
namespace files=sadx::file_select;
namespace {
struct Stop{};
struct Search final:files::Search { std::size_t index=0; };
struct File final:files::File { std::size_t index;explicit File(std::size_t i):index(i){} };
struct Calls final:files::SaveFileCalls {
    const std::uint32_t *input;
    std::uint32_t *output;
    files::List &list;
    sadx::save::Data &live;
    unsigned count=0;
    Calls(const std::uint32_t *i,std::uint32_t *o,files::List &l,sadx::save::Data &d):input(i),output(o),list(l),live(d){}
    const std::uint32_t *record(std::size_t n)const{return input+350+n*359;}
    files::FileInfo info(std::size_t n)const {
        const auto *p=record(n);return {std::string(reinterpret_cast<const char *>(p+3)),std::uint64_t(p[1])|(std::uint64_t(p[2])<<32)};
    }
    unsigned nodes()const {unsigned n=0;for(auto p=list;p;p=p->next)++n;return n;}
    void trace(unsigned kind,unsigned ordinal=0,std::string_view path={}) {
        auto *o=output+2048+360*count++;o[0]=kind;o[1]=ordinal;o[2]=nodes();o[3]=static_cast<std::uint32_t>(path.size());std::memcpy(o+4,path.data(),std::min<std::size_t>(path.size(),32));
        const auto data=sadx::save::encode(live);std::memcpy(o+12,data.data(),data.size());
        if(count==input[1])throw Stop{};
    }
    std::optional<files::FoundFile> find_first(std::string_view path)override {
        trace(1,0,path);if(!input[0])return std::nullopt;
        return files::FoundFile{std::make_shared<Search>(),info(0)};
    }
    std::optional<files::FileInfo> find_next(const std::shared_ptr<files::Search> &s)override {
        trace(2);auto search=std::static_pointer_cast<Search>(s);if(++search->index>=input[0])return std::nullopt;return info(search->index);
    }
    void close_search(const std::shared_ptr<files::Search> &)override {trace(3);}
    std::shared_ptr<files::File> open_file(std::string_view path)override {
        for(std::size_t n=0;n<input[0];++n)if(path=="./SAVEDATA/"+info(n).name){trace(4,static_cast<unsigned>(n+1),path);return std::make_shared<File>(n);}
        throw std::runtime_error("fixture file path not found");
    }
    std::uint32_t read_save(const std::shared_ptr<files::File> &f,sadx::save::EncodedData &bytes)override {
        const auto n=std::static_pointer_cast<File>(f)->index;trace(5,static_cast<unsigned>(n+1));const auto *r=record(n);const auto size=std::min<std::uint32_t>(r[0],1392);std::memcpy(bytes.data(),r+11,size);return size==1392;
    }
    void close_file(const std::shared_ptr<files::File> &f)override {trace(6,static_cast<unsigned>(std::static_pointer_cast<File>(f)->index+1));}
};
}
extern "C" void sadx_native_save_file_list_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+160000,0);sadx::save::EncodedData bytes;std::memcpy(bytes.data(),i+2,1392);auto live=files::decode_save(bytes);files::List list;Calls calls(i,o,list,live);
    try{o[0]=files::count_save_files(list,live,calls);}catch(const Stop&){o[1]=1;}
    o[2]=calls.count;o[3]=calls.nodes();bytes=sadx::save::encode(live);std::memcpy(o+16,bytes.data(),1392);
    unsigned n=0;for(auto p=list;p;p=p->next) {
        auto *r=o+512+n++*8;if(!p->file)continue;r[0]=1;std::memcpy(r+1,p->file->name.data(),std::min<std::size_t>(p->file->name.size(),20));r[6]=static_cast<std::uint32_t>(p->file->last_write_time);r[7]=static_cast<std::uint32_t>(p->file->last_write_time>>32);
    }
}

extern "C" void sadx_native_load_selected_save_probe(const std::uint32_t *i,std::uint32_t *o) {
    std::fill(o,o+160000,0);sadx::save::EncodedData bytes;std::memcpy(bytes.data(),i+2,1392);auto live=files::decode_save(bytes);files::List list;Calls calls(i,o,list,live);
    const auto *extra=i+350+i[0]*359;
    if(extra[2]) {
        files::append_save_list(list,std::nullopt,{});
        for(std::size_t n=0;n<i[0];++n){const auto info=calls.info(n);files::append_save_list(list,info.name,info);}
    }
    auto disabled=static_cast<std::int32_t>(extra[0]);auto selected=static_cast<std::uint8_t>(extra[1]);auto rings=static_cast<std::int16_t>(extra[4]);
    const auto fallback=calls.info(extra[3]).name;files::LoadState state{disabled,selected,fallback.c_str(),list,live,rings};
    try{o[0]=static_cast<std::uint32_t>(files::load_selected_save(state,calls));}catch(const Stop&){o[1]=1;}
    o[2]=calls.count;o[3]=calls.nodes();o[4]=static_cast<std::uint16_t>(rings);bytes=sadx::save::encode(live);std::memcpy(o+16,bytes.data(),1392);
    unsigned n=0;for(auto p=list;p;p=p->next){auto *r=o+512+n++*8;if(!p->file)continue;r[0]=1;std::memcpy(r+1,p->file->name.data(),std::min<std::size_t>(p->file->name.size(),20));r[6]=static_cast<std::uint32_t>(p->file->last_write_time);r[7]=static_cast<std::uint32_t>(p->file->last_write_time>>32);}
}
