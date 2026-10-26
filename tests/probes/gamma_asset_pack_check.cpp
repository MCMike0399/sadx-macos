// Development-only strict loading of the appended pack; no runtime metadata reader.
#include "engine/character_pack.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace sadx::character_pack;
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 4, "usage: gamma-asset-pack-check NEW_PACK OLD_PACK STAND_ID");
        const auto pack = CharacterPack::load(argv[1]);
        const auto old = CharacterPack::load(argv[2]);
        require(old.record_count() == 3302, "wrong original Sonic resource namespace");
        require(pack.record_count() > old.record_count(), "Gamma closure was not appended");
        for (ResourceId id = 1; id <= old.record_count(); ++id) {
            const auto& before = old.record(id);
            const auto& after = pack.record(id);
            require(before.kind == after.kind && before.payload == after.payload,
                    "existing encoded resource changed");
        }
        for (unsigned index = 0; index < 7; ++index) {
            const auto action = static_cast<TitleAction>(index);
            require(pack.action(action).identity == old.action(action).identity,
                    "existing title binding changed");
        }
        for (unsigned index = 0; index < 19; ++index) {
            const auto action = static_cast<SelectionAction>(index);
            require(pack.action(action).identity == old.action(action).identity,
                    "existing selection binding changed");
        }
        const auto stand_id = static_cast<ResourceId>(std::stoul(argv[3]));
        const auto stand = pack.resolve(sadx::ActionHandle{stand_id});
        const auto motion = pack.motion(stand.motion);
        require(motion.part_count == 77 && motion.frame_count == 16 &&
                    motion.channels == 3 && motion.interpolation == 2,
                "request0 is not the actual Stand payload");
        const auto selected = pack.resolve(pack.action(SelectionAction::GammaSelected));
        require(stand.object == selected.object && stand.motion != selected.motion,
                "Gamma Stand must share the real model and retain its distinct motion");
        std::cout << "strict SDXACT03 load: " << pack.record_count()
                  << " records, original 3302 and all bindings exact, real77-part Stand\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
