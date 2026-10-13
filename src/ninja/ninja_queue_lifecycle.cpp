#include "ninja/ninja_queue_lifecycle.hpp"

namespace sadx {
namespace {
constexpr std::uint32_t kBufferBytes=0x88000;
}
NinjaQueueLifecycleResult ninja_queue_reset_407ab0(NinjaQueueLifecycleOwners& o,std::uint32_t base,X87Context& fp){
    NinjaQueueLifecycleResult r;
    const auto stop=[&](NinjaQueueLifecycleFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;return r;};
    auto arena=o.queue.arena();
    if(!arena||arena->bytes().size()<kBufferBytes||arena->reset_407ab0(base)!=NinjaQueueFailure::none)return stop(NinjaQueueLifecycleFailure::arena_failed,0x407ad0);
    o.queue.forget_records_after_arena_reset();
    // 4128f0 -> 402f40(&3b18230)
    std::uint32_t site=0x402f44;
    try{
        NinjaQueueColorOwners color{o.queue_runtime.color_at_3ab9864,o.projection.color_argb_at_3d0f7f0,o.one_at_7dcb10,o.negative_one_at_7dcb0c};
        if(!ninja_queue_color_402f40(o.initial_color_at_3b18230,color,r.color,fp,site))return stop(NinjaQueueLifecycleFailure::unknown_fact,0x402f44,0x3b18230);
    }catch(const X87Error& e){r.fp_failure=e.failure();return stop(NinjaQueueLifecycleFailure::fp_failed,site);}
    // 407330
    if(!o.queue_runtime.render_state)return stop(NinjaQueueLifecycleFailure::render_failed,0x77ed04);
    auto& render=*o.queue_runtime.render_state;
    const auto append=[&](const NinjaRenderResult& c){r.render.commands.insert(r.render.commands.end(),c.commands.begin(),c.commands.end());r.render.ignored_hresults.insert(r.render.ignored_hresults.end(),c.ignored_hresults.begin(),c.ignored_hresults.end());return bool(c);};
    if(!o.depth_mode_at_8928c8)return stop(NinjaQueueLifecycleFailure::unknown_fact,0x401420,0x8928c8);
    if(const auto c=ninja_set_depth_comparison(*o.depth_mode_at_8928c8,o.depth_table_at_88ad9c,render);!append(c))return stop(NinjaQueueLifecycleFailure::render_failed,c.source_boundary);
    if(const auto c=ninja_set_depth_write(1,render);!append(c))return stop(NinjaQueueLifecycleFailure::render_failed,c.source_boundary);
    if(!ninja_issue_render_state(r.render,render,27,1,{0x77ece4,0x77ece9,0x77ecef}))return stop(NinjaQueueLifecycleFailure::render_failed,r.render.source_boundary);
    o.queue_runtime.flag_at_3b12208=0;o.consumer.mode_byte_at_3abdbc0=0; // 412400(0): (0 sar 1)&63, then 401440
    o.queue.initialize_sort_buckets();
    o.words_at_3ab9898=o.words_at_900840;
    o.projection.color_argb_at_3d0f7f0=o.queue_runtime.color_at_3ab9864; // 77ff90(&3ab9864): constant non-null pointer, raw copy
    o.queue_runtime.mode_at_3ab98ac=0;
    o.timing.word_at_3b1117c=0;o.consumer.count_at_3abd9d4=0;o.consumer.count_at_3abd9dc=0;
    r.completed=true;return r;
}
NinjaQueueLifecycleResult ninja_queue_allocate_408630(NinjaQueueLifecycleOwners& o,std::optional<std::uint32_t> heap,X87Context& fp){
    // 780fb0 frees the old buffer: releasing the arena drops its records.
    o.queue.rebind_arena(std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(kBufferBytes,0),0,kBufferBytes,0));
    o.words_at_90084c=o.words_at_900858;
    auto r=ninja_queue_reset_407ab0(o,heap.value_or(0),fp);
    r.alignment_assumed=!heap;return r;
}
} // namespace sadx
