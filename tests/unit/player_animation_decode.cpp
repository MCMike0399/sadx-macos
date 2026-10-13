#include "player/player_animation_decode.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
int main(int argc,char** argv){using namespace sadx;assert(argc==2);std::ifstream file(argv[1],std::ios::binary);assert(file);std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    assert(!decode_sonic_animations(nullptr,0,0));for(std::size_t size:{0U,1U,63U,64U,512U})assert(!decode_sonic_animations(bytes.data(),size,0));
    auto decoded=decode_sonic_animations(bytes.data(),bytes.size(),4000);assert(decoded);auto& d=*decoded.decoded;assert(d.export_source_address==0x10007dd0&&d.sonic_actions.size()==149&&d.resources.objects==&d.objects);
    auto shifted=decode_sonic_animations(bytes.data(),bytes.size(),9000);assert(shifted);auto& other=*shifted.decoded;assert(other.resources.objects==&other.objects);assert(d.sonic_actions[0]->id!=other.sonic_actions[0]->id);assert(d.locations.at(*d.sonic_actions[0]->id).source_virtual_address==other.locations.at(*other.sonic_actions[0]->id).source_virtual_address);
    auto overflow=decode_sonic_animations(bytes.data(),bytes.size(),std::numeric_limits<std::size_t>::max());assert(!overflow&&overflow.failure==AnimationDecodeFailure::resource_identity_overflow);
    for(const auto& item:d.resources.actions){assert(item.second.object&&item.second.motion);assert(item.second.object->id&&d.objects.count(*item.second.object->id));assert(item.second.motion->id&&d.resources.motions.count(*item.second.motion->id));const auto& loc=d.locations.at(item.first);assert(loc.file_offset+loc.byte_size<=bytes.size());}
    for(const auto& item:d.resources.motions){const auto& m=item.second;assert(m.frame_count&&m.type&&m.input_function&&m.data);if(m.data->id){const auto found=d.resources.node_arrays.find(*m.data->id);if(found==d.resources.node_arrays.end())continue;for(const auto& node:found->second)for(unsigned ch=0;ch<3;++ch){assert(node.keys[ch]&&node.counts[ch]);if(node.keys[ch]->id){const auto& keys=d.resources.key_arrays.at(*node.keys[ch]->id);assert(keys.byte_extent/16>=*node.counts[ch]);}}}}
    for(const auto& item:d.objects){assert(item.second.child&&item.second.sibling);for(const auto& ref:{*item.second.child,*item.second.sibling})if(ref.id)assert(d.objects.count(*ref.id));}
    std::cout<<"Animation PE export/graph/key ranges and explicit artifact namespaces passed\n";
}
