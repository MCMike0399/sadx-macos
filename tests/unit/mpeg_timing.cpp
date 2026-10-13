// mpeg1_timing: a synthetic MPEG-1 system stream (pack, system header, video packets with PTS and PTS+DTS, an
// audio packet, the end code) and the rejections (MPEG-2 pack, broken length, no PTS). With a path argument it
// also reads the game's RE-US.MPG and prints its timing.
//   mpeg-timing-test [MOVIE.MPG]
#include "loader/mpeg_timing.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
using namespace sadx;
void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
void put_ts(std::vector<std::uint8_t>& v,std::uint8_t marker,std::uint64_t ts){
    v.push_back(static_cast<std::uint8_t>((marker<<4)|((ts>>29)&0xe)|1));v.push_back(static_cast<std::uint8_t>(ts>>22));
    v.push_back(static_cast<std::uint8_t>(((ts>>14)&0xfe)|1));v.push_back(static_cast<std::uint8_t>(ts>>7));v.push_back(static_cast<std::uint8_t>(((ts<<1)&0xfe)|1));}
void packet(std::vector<std::uint8_t>& v,std::uint8_t id,const std::vector<std::uint8_t>& body){
    v.insert(v.end(),{0,0,1,id,static_cast<std::uint8_t>(body.size()>>8),static_cast<std::uint8_t>(body.size())});v.insert(v.end(),body.begin(),body.end());}
std::vector<std::uint8_t> stream(std::uint64_t first,std::uint64_t last,std::uint8_t pack_marker=0x21){
    std::vector<std::uint8_t> v={0,0,1,0xba,pack_marker,0,1,0,1,0x80,0x43,0xff};
    packet(v,0xbb,{0x80,0x43,0xff,0x04,0xe1,0xff});
    std::vector<std::uint8_t> b={0xff,0xff,0x40,0x20};put_ts(b,3,first);put_ts(b,1,first-3003); // stuffing, STD, PTS+DTS
    b.insert(b.end(),{0,0,1,0xb3,0x28,0x01,0xe0,0x14,0,0,0,0});packet(v,0xe0,b);       // 640x480, code 4
    std::vector<std::uint8_t> a;put_ts(a,2,first+100);a.push_back(0);packet(v,0xc0,a);
    std::vector<std::uint8_t> c;put_ts(c,2,last);c.push_back(0);packet(v,0xe0,c);
    std::vector<std::uint8_t> d={0x0f,0};packet(v,0xe0,d);                               // no PTS
    v.insert(v.end(),{0,0,1,0xb9});return v;}
}
int main(int argc,char** argv){try{
    std::string error;
    const auto t=mpeg1_timing(stream(37245,37245+90000*10),&error);require(bool(t),error);
    require(t->first_pts==37245&&t->last_pts==37245+900000&&t->video_packets_with_pts==2,"PTS range");
    require(t->width==640&&t->height==480&&t->frame_rate_numerator==30000&&t->frame_rate_denominator==1001,"sequence header");
    require(t->duration_100ns==100000000ULL+333666ULL,"10 s + one 29.97 Hz frame");
    require(!mpeg1_timing(stream(0,90000,0x44),&error)&&error.find("MPEG-1")!=std::string::npos,"an MPEG-2 pack is rejected");
    auto broken=stream(0,90000);broken[17]=0xff;require(!mpeg1_timing(broken,&error),"a broken packet length is rejected");
    std::vector<std::uint8_t> nopts={0,0,1,0xba,0x21,0,1,0,1,0x80,0x43,0xff};packet(nopts,0xe0,{0x0f,0});require(!mpeg1_timing(nopts,&error),"no PTS is rejected");
    std::cout<<"MPEG timing: synthetic stream 10.0334 s, rejections ok";
    if(argc==2){std::ifstream f(argv[1],std::ios::binary);require(bool(f),std::string("cannot read ")+argv[1]);
        const auto m=mpeg1_timing({std::istreambuf_iterator<char>(f),{}},&error);require(bool(m),error);
        std::cout<<"; "<<argv[1]<<": "<<m->width<<"x"<<m->height<<", "<<m->video_packets_with_pts<<" video PTS, duration "<<m->duration_100ns/10000<<" ms";}
    std::cout<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
