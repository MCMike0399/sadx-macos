#include "engine/character_pack.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace sadx::character_pack;
namespace {
using Bytes=std::vector<std::uint8_t>;
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
Bytes read(const char *path) {std::ifstream f(path,std::ios::binary);require(bool(f),"pack missing");return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
void put(Bytes &b,std::size_t offset,std::uint32_t v) {for(unsigned i=0;i<4;++i)b.at(offset+i)=static_cast<std::uint8_t>(v>>(8*i));}
void reject(Bytes b) {try{CharacterPack::decode(b);}catch(const std::runtime_error &){return;}throw std::runtime_error("invalid selection pack accepted");}
}
int main(int argc,char **argv) {
 try {
  require(argc==3,"usage: selection-pack-test SELECTION TITLE");const auto bytes=read(argv[1]);const auto p=CharacterPack::decode(bytes);const auto old=CharacterPack::decode(read(argv[2]));
  require(p.record_count()==3328 && bytes.size()==766112,"full source resource graph size");
  const auto stand=p.resolve(sadx::ActionHandle{3303});
  require(stand.motion==3304 && p.motion(stand.motion).part_count==77 &&
          p.motion(stand.motion).frame_count==16,"actual Gamma Stand closure");
  for(ResourceId id=1;id<=old.record_count();++id) {const auto &a=p.record(id),&b=old.record(id);require(a.kind==b.kind && a.payload==b.payload,"old title resource namespace changed");}
  for(unsigned i=0;i<7;++i)require(p.action(static_cast<TitleAction>(i)).identity==old.action(static_cast<TitleAction>(i)).identity,"title handle changed");
  const std::array<unsigned,19> frames{32,65,2,16,68,2,16,75,2,32,90,2,32,25,45,2,16,80,2};
  std::array<ResourceId,19> objects{};
  for(unsigned i=0;i<frames.size();++i) {auto a=p.resolve(p.action(static_cast<SelectionAction>(i)));require(p.motion(a.motion).frame_count==frames[i],"selection motion frame count");objects[i]=a.object;}
  require(objects[0]==objects[1] && objects[1]==objects[2],"Sonic source model alias");
  require(objects[3]!=objects[4] && objects[4]==objects[5],"Miles distinct idle model and settled alias");
  require(objects[6]==objects[7] && objects[7]==objects[8],"Knuckles source model alias");
  require(objects[9]==objects[10] && objects[10]==objects[11],"Amy source model alias");
  require(objects[12]==objects[13] && objects[13]==objects[14] && objects[14]==objects[15],"Big source model alias");
  require(objects[16]==objects[17] && objects[17]==objects[18],"Gamma source model alias");
  require(p.action(SelectionAction::SonicSelected).identity==p.action(TitleAction::SonicClearPose).identity,"Sonic selection/title share one resource");
  require(p.action(SelectionAction::KnucklesSelected).identity==p.action(TitleAction::KnucklesClearPose).identity,"Knuckles selection/title share one resource");
  require(p.object(2942).model!=0,"actual source question-mark object imported");
  auto b=bytes;put(b,12,21);reject(b);b=bytes;put(b,16,2);reject(b);b=bytes;put(b,20,8);reject(b);b=bytes;put(b,28,0);reject(b);
  b=bytes;for(unsigned n=0;n<16;++n)b[32+n]=b[16+n];reject(b);
  b=bytes;put(b,28,999999);reject(b);
  for(unsigned n=16;n<368;++n)reject(Bytes(bytes.begin(),bytes.begin()+n));
  try{p.action(static_cast<SelectionAction>(19));throw std::logic_error("invalid selection enum accepted");}catch(const std::runtime_error &){}
  std::cout<<"selection pack:22 bindings,3328 records,19 source sequences/frames, old title namespace and malformed rejection pass\n";
  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
