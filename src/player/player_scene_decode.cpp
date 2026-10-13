#include "player/player_scene_decode.hpp"
#include <cstring>
#include <limits>

namespace sadx {
namespace {
class Image {
public:
    Image(const std::uint8_t* data,std::size_t size):data_(data),size_(size) {
        if(!data || size<64 || read16(0)!=0x5a4d)return;
        const auto pe=static_cast<std::size_t>(read32(0x3c));
        if(pe>size || size-pe<24 || read32(pe)!=0x4550)return;
        const auto optional=pe+24U;
        const auto optional_size=read16(pe+20);
        if(optional>size || optional_size>size-optional || optional_size<32 || read16(optional)!=0x10b || read16(pe+4)!=0x14c)return;
        base_=read32(optional+28);const auto count=read16(pe+6);
        const auto section=optional+optional_size;
        if(section>size || static_cast<std::size_t>(count)>(size-section)/40)return;
        for(std::size_t i=0;i<count;++i){
            const auto offset=section+i*40;Region r{read32(offset+12),read32(offset+16),read32(offset+20)};
            if(r.raw>size || r.size>size-r.raw)return;regions_.push_back(r);
        }
        valid_=true;
    }
    bool valid()const{return valid_;}
    const std::uint8_t* range(std::uint32_t address,std::size_t bytes,std::size_t* offset=nullptr)const{
        if(!valid_ || address<base_)return nullptr;const auto rva=address-base_;
        const std::uint8_t* result=nullptr;std::size_t selected_offset=0;
        for(const auto& r:regions_)if(rva>=r.rva && static_cast<std::uint64_t>(rva-r.rva)+bytes<=r.size){
            if(result)return nullptr;selected_offset=static_cast<std::size_t>(r.raw)+(rva-r.rva);result=data_+selected_offset;
        }
        if(result && offset)*offset=selected_offset;return result;
    }
    static std::uint16_t u16(const std::uint8_t* p){return static_cast<std::uint16_t>(p[0]|(std::uint16_t{p[1]}<<8));}
    static std::uint32_t u32(const std::uint8_t* p){return std::uint32_t{p[0]}|(std::uint32_t{p[1]}<<8)|(std::uint32_t{p[2]}<<16)|(std::uint32_t{p[3]}<<24);}
    static float f32(const std::uint8_t* p){const auto bits=u32(p);float f;std::memcpy(&f,&bits,4);return f;}
    static std::int32_t i32(const std::uint8_t* p){const auto bits=u32(p);std::int32_t value;std::memcpy(&value,&bits,4);return value;}
    static NinjaVector vector(const std::uint8_t* p){return {f32(p),f32(p+4),f32(p+8)};}
private:
    struct Region{std::uint32_t rva,size,raw;};
    std::uint16_t read16(std::size_t at)const{return at<=size_ && size_-at>=2?u16(data_+at):0;}
    std::uint32_t read32(std::size_t at)const{return at<=size_ && size_-at>=4?u32(data_+at):0;}
    const std::uint8_t* data_;std::size_t size_;std::uint32_t base_=0;bool valid_=false;std::vector<Region> regions_;
};
SceneDecodeResult decode(const Image& image,std::uint32_t table_address){
    const auto fail=[&](SceneDecodeFailure f,std::uint32_t address){return SceneDecodeResult{f,0,{},address};};
    if(!image.valid())return fail(SceneDecodeFailure::invalid_pe_image,table_address);
    if(!table_address)return fail(SceneDecodeFailure::null_landtable,0);
    std::size_t offset=0;const auto header=image.range(table_address,28,&offset);
    if(!header)return fail(SceneDecodeFailure::unavailable_source_range,table_address);
    DecodedSceneLandTable table;const auto count_bits=Image::u16(header);
    std::memcpy(&table.source_count,&count_bits,2);table.source_flags=Image::u16(header+4);
    table.table_location={table_address,offset,28,"LANDTABLE"};table.source_land_array_address=Image::u32(header+12);
    if(table.source_count<0)return fail(SceneDecodeFailure::negative_land_count,table_address);
    const auto byte_count=static_cast<std::size_t>(table.source_count)*36;
    const auto entries=image.range(table.source_land_array_address,byte_count);
    if(byte_count && !entries)return fail(SceneDecodeFailure::unavailable_source_range,table.source_land_array_address);
    std::unordered_map<std::uint32_t,std::size_t> object_ids,model_ids,vector_ids,meshset_ids,index_ids,material_ids;
    const auto vectors=[&](std::uint32_t address,std::size_t count)->std::optional<SceneResourceReference>{
        if(!address)return SceneResourceReference{};
        if(count>std::numeric_limits<std::size_t>::max()/12)return std::nullopt;
        std::size_t at=0;const auto bytes=image.range(address,count*12,&at);
        if(!bytes)return std::nullopt;
        const auto id=vector_ids.emplace(address,vector_ids.size()).first->second;
        auto& array=table.vector_arrays[id];
        if(array.size()<count){array.clear();array.reserve(count);for(std::size_t n=0;n<count;++n)array.push_back(Image::vector(bytes+n*12));}
        auto& location=table.vector_locations[id];
        if(location.byte_size<count*12 || location.record_kind.empty())location={address,at,count*12,"NJS_VECTOR array"};
        return SceneResourceReference{id};
    };
    const auto indices=[&](std::uint32_t address,std::uint16_t type,std::uint16_t count)->std::optional<SceneResourceReference>{
        if(!address)return SceneResourceReference{};
        const auto kind=type&0xc000U;std::size_t words=0;
        if(kind==0 || kind==0x4000U)words=static_cast<std::size_t>(count)*(kind==0?3:4);
        else {
            for(std::size_t n=0;n<count;++n){
                const auto current=static_cast<std::uint64_t>(address)+words*2;
                if(current>std::numeric_limits<std::uint32_t>::max())return std::nullopt;
                const auto header=image.range(static_cast<std::uint32_t>(current),2);
                if(!header)return std::nullopt;
                const auto length=Image::u16(header)&0x7fffU;
                if(!image.range(static_cast<std::uint32_t>(current),(static_cast<std::size_t>(length)+1)*2))return std::nullopt;
                words+=static_cast<std::size_t>(length)+1;
            }
        }
        std::size_t at=0;const auto bytes=image.range(address,words*2,&at);if(!bytes)return std::nullopt;
        const auto id=index_ids.emplace(address,index_ids.size()).first->second;
        auto& array=table.index_arrays[id];
        if(array.size()<words){array.clear();array.reserve(words);for(std::size_t n=0;n<words;++n)array.push_back(Image::u16(bytes+n*2));}
        auto& location=table.index_locations[id];
        if(location.byte_size<words*2 || location.record_kind.empty())location={address,at,words*2,"NJS_MESHSET index stream"};
        return SceneResourceReference{id};
    };
    SceneDecodeFailure set_failure=SceneDecodeFailure::unavailable_meshsets;std::uint32_t set_failure_address=0;
    const auto meshsets=[&](std::uint32_t address,std::uint16_t count)->std::optional<SceneResourceReference>{
        if(!address)return SceneResourceReference{};
        std::size_t at=0;const auto bytes=image.range(address,static_cast<std::size_t>(count)*28,&at);if(!bytes)return std::nullopt;
        const auto id=meshset_ids.emplace(address,meshset_ids.size()).first->second;
        auto& array=table.mesh_set_arrays[id];
        for(std::size_t n=array.size();n<count;++n){
            const auto p=bytes+n*28;const auto type=Image::u16(p),meshes=Image::u16(p+2);const auto pointer=Image::u32(p+4);
            const auto ref=indices(pointer,type,meshes);
            if(!ref){set_failure=SceneDecodeFailure::unavailable_index_stream;set_failure_address=pointer;return std::nullopt;}
            array.push_back(SceneMeshSet{type,meshes,*ref});
        }
        auto& location=table.meshset_locations[id];
        if(location.byte_size<static_cast<std::size_t>(count)*28 || location.record_kind.empty())location={address,at,static_cast<std::size_t>(count)*28,"NJS_MESHSET array"};
        return SceneResourceReference{id};
    };
    const auto materials=[&](std::uint32_t address,std::uint16_t count)->std::optional<SceneResourceReference>{
        if(!address)return SceneResourceReference{};
        std::size_t at=0;const auto bytes=image.range(address,static_cast<std::size_t>(count)*20,&at);if(!bytes)return std::nullopt;
        const auto id=material_ids.emplace(address,material_ids.size()).first->second;
        auto& array=table.material_arrays[id];
        for(std::size_t n=array.size();n<count;++n){SceneMaterial value;for(std::size_t k=0;k<5;++k)value.words[k]=Image::u32(bytes+n*20+k*4);array.push_back(value);}
        auto& location=table.material_locations[id];
        if(location.byte_size<static_cast<std::size_t>(count)*20 || location.record_kind.empty())location={address,at,static_cast<std::size_t>(count)*20,"NJS_MATERIAL array"};
        return SceneResourceReference{id};
    };
    std::vector<std::uint32_t> object_queue;
    const auto object_reference=[&](std::uint32_t address){
        if(!address)return SceneResourceReference{};
        const auto inserted=object_ids.emplace(address,object_ids.size());
        if(inserted.second)object_queue.push_back(address);return SceneResourceReference{inserted.first->second};
    };
    for(std::size_t i=0;i<static_cast<std::size_t>(table.source_count);++i){
        const auto p=entries+i*36;
        table.entries.push_back(SceneStaticLandEntry{Image::vector(p),Image::f32(p+12),object_reference(Image::u32(p+24)),Image::u32(p+32),Image::u32(p+16),Image::u32(p+20),Image::u32(p+28)});
    }
    for(std::size_t index=0;index<object_queue.size();++index){
                const auto object_address=object_queue[index];const auto object_id=object_ids.at(object_address);
                const auto object=image.range(object_address,52,&offset);
                if(!object)return fail(SceneDecodeFailure::unavailable_object,object_address);
                table.object_locations[object_id]={object_address,offset,52,"NJS_OBJECT"};
                SceneGeometryObject native;native.position=Image::vector(object+8);
                native.angles=std::array<std::int32_t,3>{Image::i32(object+20),Image::i32(object+24),Image::i32(object+28)};
                native.scale=Image::vector(object+32);native.evaluation_flags=Image::u32(object);
                native.child=object_reference(Image::u32(object+44));native.sibling=object_reference(Image::u32(object+48));
                const auto model_address=Image::u32(object+4);SceneResourceReference model_ref;
                if(model_address){
                    auto model_inserted=model_ids.emplace(model_address,model_ids.size());model_ref.id=model_inserted.first->second;
                    if(model_inserted.second){
                        const auto model=image.range(model_address,40,&offset);
                        if(!model)return fail(SceneDecodeFailure::unavailable_model,model_address);
                        auto& native_model=table.models[*model_ref.id];native_model.center=Image::vector(model+24);native_model.radius=Image::f32(model+36);
                        const auto count=Image::i32(model+8);native_model.vertex_count=count;native_model.meshset_count=Image::u16(model+20);
                        if(count<0)return fail(SceneDecodeFailure::invalid_vertex_count,model_address+8);
                        const auto vertex_address=Image::u32(model),normal_address=Image::u32(model+4),set_address=Image::u32(model+12);
                        const auto vertex_ref=vectors(vertex_address,static_cast<std::size_t>(count));
                        if(!vertex_ref)return fail(SceneDecodeFailure::unavailable_vertices,vertex_address);native_model.vertices=*vertex_ref;
                        const auto normal_ref=vectors(normal_address,static_cast<std::size_t>(count));
                        if(!normal_ref)return fail(SceneDecodeFailure::unavailable_normals,normal_address);native_model.normals=*normal_ref;
                        set_failure_address=set_address;const auto set_ref=meshsets(set_address,*native_model.meshset_count);
                        if(!set_ref)return fail(set_failure,set_failure_address);native_model.meshsets=*set_ref;
                        native_model.material_count=Image::u16(model+22);const auto material_address=Image::u32(model+16);
                        const auto material_ref=materials(material_address,*native_model.material_count);
                        if(!material_ref)return fail(SceneDecodeFailure::unavailable_materials,material_address);native_model.materials=*material_ref;
                        table.model_locations[*model_ref.id]={model_address,offset,40,"NJS_MODEL"};
                    }
                }
                native.model=model_ref;table.objects[object_id]=native;
    }
    return {SceneDecodeFailure::none,0,std::move(table),table_address};
}
}
SceneDecodeResult decode_scene_landtable(const std::uint8_t* bytes,std::size_t size,std::uint32_t address){return decode(Image(bytes,size),address);}
SceneDecodeResult decode_stage_scene_landtable(const std::uint8_t* bytes,std::size_t size,std::int8_t stage,std::int8_t act){
    const Image image(bytes,size);if(!image.valid())return {SceneDecodeFailure::invalid_pe_image,0,{},0};
    const auto index=static_cast<std::int32_t>(stage)*8+static_cast<std::int32_t>(act);
    const auto slot=static_cast<std::uint32_t>(0x97da08U+static_cast<std::uint32_t>(index)*4U);
    const auto pointer=image.range(slot,4);
    if(!pointer)return {SceneDecodeFailure::unavailable_source_range,0x43b748,{},slot};
    auto result=decode(image,Image::u32(pointer));result.source_boundary=result.failure==SceneDecodeFailure::none?0:0x43b751;return result;
}
} // namespace sadx
