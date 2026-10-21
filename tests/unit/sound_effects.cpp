#include "game/sound_effects.hpp"
#include <cassert>
#include <memory>
namespace {
struct Sample final:sadx::sound_effects::Sample {};
struct Calls final:sadx::sound_effects::SampleCalls {
    bool present=true;
    sadx::sound_effects::BankView bank(std::uint32_t) override {return {present,{"B42_00_02.WAV"}};}
    sadx::sound_effects::SelectedSample sample(std::uint32_t,std::size_t ordinal) override {
        assert(ordinal==0);return {std::make_shared<Sample>(),38190};
    }
};
}
int main() {
    namespace sfx=sadx::sound_effects;
    sfx::State state;
    Calls calls;
    sfx::select_bank_sample(0,2,state,calls);
    const auto retained=state.selected_sample.resource;
    assert(retained && state.selected_sample.source_size==38190);
    bool missing=false;
    try{sfx::select_bank_sample(0,3,state,calls);}catch(const sfx::Error &e){missing=e.boundary()==sfx::Boundary::SampleIndex;}
    assert(missing && state.selected_sample.resource==retained && state.selected_sample.source_size==38190);
    calls.present=false;
    sfx::select_bank_sample(0,3,state,calls);
    assert(!state.selected_sample.resource && state.selected_sample.source_size==0);
}
