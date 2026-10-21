#include "game/save_file_list.hpp"
#include "game/file_select.hpp"
#include <cassert>
#include <fstream>
#include <random>
int main(int argc, char **argv) {
    std::mt19937 random(505050);
    for(unsigned n=0;n<2048;++n) {
        sadx::save::EncodedData bytes;
        for(auto &byte:bytes)byte=static_cast<std::uint8_t>(random());
        const auto data=sadx::file_select::decode_save(bytes);
        assert(sadx::save::encode(data)==bytes);
    }
    for(int argument=1;argument<argc;++argument) {
        const char *path=argv[argument];
        std::ifstream input(path,std::ios::binary);sadx::save::EncodedData bytes;
        input.read(reinterpret_cast<char *>(bytes.data()),bytes.size());assert(input.gcount()==static_cast<std::streamsize>(bytes.size()));
        assert(sadx::save::encode(sadx::file_select::decode_save(bytes))==bytes);
        assert(sadx::file_select::check_save(bytes));
    }
}
