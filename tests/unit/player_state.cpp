#include "player/player_state.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char** argv) {
    try {
        sadx::X87Context context(0x027f);
        require(argc==2, "usage: player-state-test PLAYER_PARAMETER_TABLE");
        std::ifstream input(argv[1],std::ios::binary);
        require(input.good(), "parameter table is missing");
        std::array<std::uint8_t, 8*132> bytes{};
        input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
        require(input.gcount()==static_cast<std::streamsize>(bytes.size()) && input.peek()==EOF,
                "table must contain exactly eight 132-byte records");
        static_assert(sizeof(sadx::PlayerParameters)==132);
        static_assert(offsetof(sadx::PlayerParameters,lim_h_spd)==8);
        static_assert(offsetof(sadx::PlayerParameters,rad)==0x70);
        static_assert(offsetof(sadx::PlayerParameters,weight)==0x78);
        sadx::PlayerEventFlags empty{};
        for(std::uint8_t row=0; row<8; ++row) {
            auto decoded=sadx::decode_player_parameters(bytes.data()+row*132,132);
            require(static_cast<bool>(decoded), "record decoder rejected the original table");
            // The tested arm64 ABI stores these value-only fields without
            // padding. This checks every source bit, not just selected values.
            require(std::memcmp(&*decoded.parameters,bytes.data()+row*132,132)==0,
                    "source parameter bit patterns were changed");
            const auto fresh=sadx::seed_player_fields(row,0,*decoded.parameters,empty, context);
            require(static_cast<bool>(fresh), "known character row rejected");
            require(fresh.fields->equipment==0 && fresh.fields->task_id==1,
                    "new-save fields or unset task ID incorrect");
            const auto occupied=sadx::seed_player_fields(row,6,*decoded.parameters,empty, context);
            require(occupied.fields->task_id==6, "existing task ID overwritten");
        }
        const auto sonic=sadx::decode_player_parameters(bytes.data(),132);
        require(sonic.parameters->jump2_timer==60 && sonic.parameters->rad==4 &&
                sonic.parameters->lim_h_spd==16 && sonic.parameters->weight==0.08F,
                "wrong source table schema or row");
        sadx::PlayerEventFlags all{}; all.fill(255);
        const std::array<std::uint16_t,8> expected{0x1f,0,0x18,0x60,0,0x180,0x1e00,0x1800};
        for(std::uint8_t row=0;row<8;++row) {
            auto result=sadx::seed_player_fields(row,0,*sonic.parameters,all, context);
            require(result.fields->equipment==expected[row], "character upgrade mask incorrect");
        }
        all.fill(2);
        require(sadx::seed_player_fields(0,0,*sonic.parameters,all, context).fields->equipment==0x1f,
                "event bytes incorrectly interpreted as bit zero");
        sadx::PlayerEventFlags amy_only{}; amy_only[0x152]=1;
        require(sadx::seed_player_fields(5,0,*sonic.parameters,amy_only, context).fields->equipment==0x100,
                "Amy equipment byte offset lost");
        require(sadx::seed_player_fields(0,0,*sonic.parameters,amy_only, context).fields->equipment==0,
                "unrelated character event applied");
        const auto seed=sadx::seed_player_fields(0,0,*sonic.parameters,empty, context);
        require(seed.fields->motion.speed.x==0 && seed.fields->motion.acceleration.y==0 &&
                seed.fields->task_scale.z==1 && seed.fields->motion_friction==1 &&
                seed.fields->animation_mode==0 && seed.fields->island_number==0 &&
                seed.fields->shadow_scale==0.8F, "initial owning field stores incorrect");
        require(sadx::seed_player_fields(8,0,*sonic.parameters,empty, context).failure==
                sadx::PlayerSeedFailure::unknown_character_row, "unmapped character normalized into Sonic");
        require(!sadx::decode_player_parameters(nullptr,132) &&
                !sadx::decode_player_parameters(bytes.data(),131), "invalid parameter range accepted");
        auto special=bytes;
        const std::uint32_t nan=0x7fc12345, negative_zero=0x80000000;
        std::memcpy(special.data()+4,&nan,4); std::memcpy(special.data()+8,&negative_zero,4);
        const auto preserved=sadx::decode_player_parameters(special.data(),132);
        std::uint32_t actual_nan, actual_zero;
        std::memcpy(&actual_nan,&preserved.parameters->pos_error,4);
        std::memcpy(&actual_zero,&preserved.parameters->lim_h_spd,4);
        require(actual_nan==nan && actual_zero==negative_zero, "numeric source identity normalized");
        std::cout<<"Player seed: eight source rows, original bit patterns, upgrade masks and field stores passed.\n"
                   "Full PInitialize resource/start-position/task behavior remains incomplete.\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
