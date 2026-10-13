#include "game/game_logic_support.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
sadx::GameLogicEdgeState edges(){sadx::GameLogicEdgeState s;for(std::size_t i=0;i<256;++i){s.raw_at_3b0e3e2[i][0]=static_cast<std::uint8_t>(i);s.aggregate_at_3b0eaf0[i]=0x80;}s.special_at_3b0e3ba=2;s.aggregate_special_at_3b0eae8=1;return s;}
void tests(){
    auto s=edges();auto r=sadx::aggregate_game_logic_edges(0,s);require(r&&r.committed_bytes==256&&s.aggregate_at_3b0eaf0[255]==255&&s.aggregate_special_at_3b0eae8==2&&!s.raw_at_3b0e3e2[0][1],"Copy consumes only stride3 lane0");
    s=edges();s.raw_at_3b0e3e2[1][0].reset();r=sadx::aggregate_game_logic_edges(1,s);require(!r&&r.source_boundary==0x40ed40&&r.committed_bytes==0&&s.aggregate_at_3b0eaf0[0]==0x80,"Mode1 next raw read precedes current store");
    s=edges();s.aggregate_at_3b0eaf0[1].reset();r=sadx::aggregate_game_logic_edges(2,s);require(!r&&r.source_boundary==0x40ecae&&r.committed_bytes==0&&s.raw_at_3b0e3e2[0][0]==0,"Mode2 next aggregate read precedes current store");
    s=edges();s.raw_at_3b0e3e2[255][0].reset();r=sadx::aggregate_game_logic_edges(0,s);require(!r&&r.committed_bytes==255&&s.aggregate_at_3b0eaf0[254]==254&&s.aggregate_special_at_3b0eae8==1,"Unavailable source retains committed prefix");
    sadx::GameLogicEdgeState unknown;r=sadx::aggregate_game_logic_edges(-1,unknown);require(r&&r.committed_bytes==0,"Other mode follows actual no-op without consuming missing facts");
    sadx::GameLogicTimingState t;t.word_at_3b11198=0;r=sadx::calculate_game_logic_ticks(t);require(r&&t.word_at_3b1117c==0&&t.word_at_3b1118c==1,"Inactive branch does not require clock or factor facts");
    t.word_at_3b11198=1;t.word_at_3b11180=50;t.word_at_3b11188=1;t.word_at_3b11178=1;r=sadx::calculate_game_logic_ticks(t);require(!r&&r.source_boundary==0x413950&&t.word_at_3b1117c==51&&t.word_at_3b1118c==51&&!r.clamped,"Both uncapped stores precede missing clamp selector");
    t.word_at_3b2a2e4=0;r=sadx::calculate_game_logic_ticks(t);require(r&&t.word_at_3b1117c==6&&r.clamped==true,"Only actual source six-tick clamp applies");
    t.word_at_3b2a2e4=1;r=sadx::calculate_game_logic_ticks(t);require(r&&t.word_at_3b1117c==51&&r.clamped==false,"Nonzero source selector preserves uncapped result");
    t.word_at_3b11180=std::numeric_limits<std::int32_t>::max();t.word_at_3b2a2e4.reset();r=sadx::calculate_game_logic_ticks(t);require(r&&t.word_at_3b1117c==std::numeric_limits<std::int32_t>::min(),"INC wrap yields negative product without reading clamp selector");
    t.word_at_3b11180=1;t.word_at_3b11178=std::numeric_limits<std::int32_t>::max();r=sadx::calculate_game_logic_ticks(t);require(r&&t.word_at_3b1117c==-2,"IMUL retains low32 bits without signed C++ overflow");
    t.word_at_3b11180=std::numeric_limits<std::int32_t>::min();t.word_at_3b11188=-1;r=sadx::calculate_game_logic_ticks(t);require(r.failure==sadx::GameLogicSupportFailure::divide_overflow&&t.word_at_3b1117c==-2,"IDIV overflow retains previous stores");
    t.word_at_3b11188=0;r=sadx::calculate_game_logic_ticks(t);require(r.failure==sadx::GameLogicSupportFailure::divide_by_zero&&t.word_at_3b1118c==-2,"IDIV zero is an explicit source failure");
}
}
int main(){try{tests();std::cout<<"Game logic support: strided partial reads, wrap, source clamp and IDIV boundaries passed.\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
