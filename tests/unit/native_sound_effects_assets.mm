#include "apps/native_sound_effects_assets.hpp"
#include <cassert>
#include <iostream>
int main(int argc,char **argv) {
    if(argc!=2)throw std::runtime_error("native-sound-effects-assets IMPORTED_DIRECTORY");
    @autoreleasepool {
        const auto directory=std::filesystem::path(argv[1]);
        const auto table=sadx::native::app::load_sound_effects_assets(directory);
        assert(table.front().tone==1382 && table.front().bank==8);
        assert(table[62].tone==39 && table[62].bank==1);
        assert(table.back().tone==-1 && table.back().bank==UINT32_MAX);
        const auto metadata=sadx::native::app::read_metadata(directory/"sound-effects.json");
        assert([metadata[@"sample_metadata"] isKindOfClass:NSArray.class]);
        std::cout<<"Native sound-effect table: actual importer/Cocoa roundtrip checked\n";
    }
}
