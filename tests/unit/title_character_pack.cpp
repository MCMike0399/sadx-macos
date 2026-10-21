#include "engine/character_pack.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
using namespace sadx::character_pack;
using Bytes = std::vector<std::uint8_t>;
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void put(Bytes& b, std::size_t o, std::uint32_t v) {
    for (unsigned i = 0; i != 4; ++i) b.at(o + i) = static_cast<std::uint8_t>(v >> (i * 8));
}
void reject(Bytes b) {
    try { CharacterPack::decode(b); } catch (const std::runtime_error&) { return; }
    throw std::runtime_error("invalid title descriptor accepted");
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: title-test PACK");
        std::ifstream f(argv[1], std::ios::binary);
        require(bool(f), "title pack exists");
        const Bytes bytes{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
        const auto p = CharacterPack::decode(bytes);
        require(p.record_count() == 1157 && bytes.size() == 273316, "source graph size");
        const std::array<std::uint32_t, 7> frames{65,11,75,14,30,25,45};
        const std::array<std::uint64_t, 7> identities{1,329,359,675,704,1096,1127};
        std::set<std::uint64_t> unique;
        std::array<ResourceId, 7> objects{};
        for (std::uint32_t i=0; i<7; ++i) {
            const auto h=p.action(static_cast<TitleAction>(i));
            require(h.identity == identities[i], "source descriptor identity");
            unique.insert(h.identity);
            const auto a=p.resolve(h);
            require(p.motion(a.motion).frame_count == frames[i], "source motion length");
            objects[i]=a.object;
        }
        require(unique.size()==7, "action handles have one shared namespace");
        require(objects[0]==objects[1] && objects[2]==objects[3] && objects[4]==objects[5] && objects[5]==objects[6], "source object aliases retained");
        require(objects[0]!=objects[2] && objects[0]!=objects[4] && objects[2]!=objects[4], "different character object graphs retained");
        require(p.action(StartupAction::ClearPose).identity==p.action(TitleAction::SonicClearPose).identity && p.action(StartupAction::ClearPoseTransition).identity==p.action(TitleAction::SonicClearPoseTransition).identity, "startup and title borrow same Sonic actions");
        auto b=bytes;put(b,12,6);reject(b);
        b=bytes;put(b,16,9);reject(b);
        b=bytes;put(b,16+2*12,0);put(b,20+2*12,100);reject(b);
        b=bytes;put(b,20+6*12,31);reject(b);
        b=bytes;put(b,24+3*12,0);reject(b);
        b=bytes;put(b,24+3*12,999999);reject(b);
        b=bytes;put(b,24,2);reject(b);
        for (std::size_t i=16; i<100; ++i) reject(Bytes(bytes.begin(),bytes.begin()+i));
        try { p.action(static_cast<TitleAction>(7)); throw std::logic_error("invalid enum accepted"); } catch(const std::runtime_error&) {}
        std::cout << "combined title pack: 7 source actions, aliases, frames, 1157 records and descriptor rejection pass\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << e.what()<<'\n';return 1; }
}
