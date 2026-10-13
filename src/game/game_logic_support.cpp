#include "game/game_logic_support.hpp"
#include <cstring>
#include <limits>
namespace sadx {
namespace {
struct Missing{};
std::int32_t signed_bits(std::uint32_t bits){std::int32_t out;std::memcpy(&out,&bits,4);return out;}
}
GameLogicSupportResult aggregate_game_logic_edges(std::int32_t mode,GameLogicEdgeState& s){
    GameLogicSupportResult r;
    const auto need=[&](const std::optional<std::uint8_t>& value,GameLogicSupportFailure why,std::uint32_t site,std::optional<std::size_t> index={}){if(!value){r.failure=why;r.source_boundary=site;r.byte_index=index;throw Missing{};}return *value;};
    const auto raw=[&](std::size_t i,std::uint32_t p){return need(s.raw_at_3b0e3e2[i][0],GameLogicSupportFailure::unknown_raw_byte,p,i);};
    const auto aggregate=[&](std::size_t i,std::uint32_t p){return need(s.aggregate_at_3b0eaf0[i],GameLogicSupportFailure::unknown_aggregate_byte,p,i);};
    try {
        if(mode==0){for(std::size_t i=0;i<256;++i){s.aggregate_at_3b0eaf0[i]=raw(i,0x40ede2);++r.committed_bytes;}
            s.aggregate_special_at_3b0eae8=need(s.special_at_3b0e3ba,GameLogicSupportFailure::unknown_special_byte,0x40edf5);
        }else if(mode==1){
            constexpr std::array<std::uint32_t,8> raws{{0x40ed35,0x40ed40,0x40ed50,0x40ed61,0x40ed72,0x40ed83,0x40ed94,0x40eda5}};
            constexpr std::array<std::uint32_t,8> aggs{{0x40ed38,0x40ed48,0x40ed59,0x40ed6a,0x40ed7b,0x40ed8c,0x40ed9d,0x40edae}};
            for(std::size_t base=0;base<256;base+=8){auto source=raw(base,raws[0]);for(std::size_t lane=0;lane<8;++lane){const auto prior=aggregate(base+lane,aggs[lane]);const auto value=static_cast<std::uint8_t>(prior|source);
                    // Actual next-source load precedes the current store.
                    if(lane<7)source=raw(base+lane+1,raws[lane+1]);s.aggregate_at_3b0eaf0[base+lane]=value;++r.committed_bytes;}}
            const auto prior=need(s.aggregate_special_at_3b0eae8,GameLogicSupportFailure::unknown_aggregate_special_byte,0x40edc6);
            const auto source=need(s.special_at_3b0e3ba,GameLogicSupportFailure::unknown_special_byte,0x40edcc);s.aggregate_special_at_3b0eae8=static_cast<std::uint8_t>(prior|source);
        }else if(mode==2){
            constexpr std::array<std::uint32_t,8> raws{{0x40eca9,0x40ecb7,0x40ecc3,0x40ecd1,0x40ecdf,0x40eced,0x40ecfb,0x40ed09}};
            constexpr std::array<std::uint32_t,8> aggs{{0x40eca3,0x40ecae,0x40ecbb,0x40ecc8,0x40ecd6,0x40ece4,0x40ecf2,0x40ed00}};
            for(std::size_t base=0;base<256;base+=8){auto prior=aggregate(base,aggs[0]);for(std::size_t lane=0;lane<8;++lane){const auto source=raw(base+lane,raws[lane]);const auto value=static_cast<std::uint8_t>(prior|source);
                    // Actual next-aggregate load precedes the current store.
                    if(lane<7)prior=aggregate(base+lane+1,aggs[lane+1]);s.raw_at_3b0e3e2[base+lane][0]=value;++r.committed_bytes;}}
            const auto source=need(s.special_at_3b0e3ba,GameLogicSupportFailure::unknown_special_byte,0x40ed19);
            const auto prior=need(s.aggregate_special_at_3b0eae8,GameLogicSupportFailure::unknown_aggregate_special_byte,0x40ed1f);s.special_at_3b0e3ba=static_cast<std::uint8_t>(prior|source);
        }
        r.completed=true;return r;
    }catch(Missing){return r;}
}
GameLogicSupportResult calculate_game_logic_ticks(GameLogicTimingState& s){
    GameLogicSupportResult r;
    const auto need=[&](const std::optional<std::int32_t>& v,std::uint32_t site){if(!v){r.failure=GameLogicSupportFailure::unknown_timing_word;r.source_boundary=site;throw Missing{};}return *v;};
    try {
        if(need(s.word_at_3b11198,0x413920)==0){s.word_at_3b1117c=0;s.word_at_3b1118c=1;r.clamped=false;r.completed=true;return r;}
        const auto dividend=need(s.word_at_3b11180,0x413929),divisor=need(s.word_at_3b11188,0x41392f);
        if(divisor==0){r.failure=GameLogicSupportFailure::divide_by_zero;r.source_boundary=0x41392f;return r;}
        if(dividend==std::numeric_limits<std::int32_t>::min()&&divisor==-1){r.failure=GameLogicSupportFailure::divide_overflow;r.source_boundary=0x41392f;return r;}
        r.quotient=dividend/divisor;r.incremented_quotient=signed_bits(static_cast<std::uint32_t>(*r.quotient)+1U);
        const auto factor=need(s.word_at_3b11178,0x41393b);r.product=signed_bits(static_cast<std::uint32_t>(*r.incremented_quotient)*static_cast<std::uint32_t>(factor));
        // Both source stores precede the optional clamp-selector read.
        s.word_at_3b1117c=*r.product;s.word_at_3b1118c=*r.product;
        if(*r.product>6){if(need(s.word_at_3b2a2e4,0x413950)==0){s.word_at_3b1117c=6;s.word_at_3b1118c=6;r.clamped=true;}else r.clamped=false;}
        else r.clamped=false;
        r.completed=true;return r;
    }catch(Missing){return r;}
}
} // namespace sadx
