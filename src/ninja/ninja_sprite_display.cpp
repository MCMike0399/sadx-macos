#include "ninja/ninja_sprite_display.hpp"
namespace sadx {
NinjaSpriteDisplayResult ninja_display_sprite(const std::optional<std::shared_ptr<const NinjaQueuedSpriteInput>>& input,
    std::int32_t index,std::uint32_t flags,NinjaQueueRecordHandle source_record,NinjaSpriteDisplayRuntime& runtime,X87Context& fp){
    NinjaSpriteDisplayResult r;auto stop=[&](NinjaSpriteDisplayFailure failure,std::uint32_t at){r.failure=failure;r.source_boundary=at;return r;};
    if(!input)return stop(NinjaSpriteDisplayFailure::unknown_input,0x77e398);
    if(!*input){r.returned=true;return r;}
    const auto& packet=**input;
    if(!packet.texture_list)return stop(NinjaSpriteDisplayFailure::unknown_texture_list,0x77e3a0);
    if(!*packet.texture_list){r.returned=true;return r;}
    auto& render=runtime.texture_runtime.render;
    // The source forms an address before selecting the list; dereferencing the
    // frame happens only after77f3d0, so an invalid frame retains its call facts.
    r.texture_calls.push_back(ninja_select_texture_list(packet.texture_list,runtime.textures,runtime.texture_runtime));
    if(!r.texture_calls.back()){r.nested_source_boundary=r.texture_calls.back().source_boundary;return stop(NinjaSpriteDisplayFailure::texture_failed,0x77e3bb);}
    if(!packet.animation)return stop(NinjaSpriteDisplayFailure::missing_animation,0x77e3c0);
    const auto offset=static_cast<std::uint32_t>(index)*20U;
    if(offset%20U)return stop(NinjaSpriteDisplayFailure::unsupported_animation_offset,0x77e3c0);
    const auto ordinal=std::size_t(offset/20U);if(ordinal>=packet.animation->size())return stop(NinjaSpriteDisplayFailure::animation_short,0x77e3c0);
    const auto& frame=(*packet.animation)[ordinal];
    r.texture_calls.push_back(ninja_select_texture_index(static_cast<std::uint32_t>(static_cast<std::int32_t>(frame.texture_index)),runtime.textures,runtime.texture_runtime));
    if(!r.texture_calls.back()){r.nested_source_boundary=r.texture_calls.back().source_boundary;return stop(NinjaSpriteDisplayFailure::texture_failed,0x77e3c4);}
    r.geometry=ninja_sprite_geometry(packet.sprite,frame,flags,runtime.constants,runtime.projection,fp);
    if(!*r.geometry){r.nested_source_boundary=r.geometry->source_boundary;return stop(NinjaSpriteDisplayFailure::geometry_failed,r.geometry->source_boundary);}
    if(r.geometry->partial.clipped){r.returned=true;return r;}
    if(!ninja_issue_texture_stage_state(r.stage,render,0,13,3,{0x77e5c2,0x77e5c7,0x77e5d0}))return stop(NinjaSpriteDisplayFailure::render_failed,r.stage.source_boundary);
    if(!ninja_issue_texture_stage_state(r.stage,render,0,14,3,{0x77e5d6,0x77e5db,0x77e5e4}))return stop(NinjaSpriteDisplayFailure::render_failed,r.stage.source_boundary);
    const auto prepared=ninja_prepare_texture_draw_states(render);
    r.render=prepared;if(!prepared)return stop(NinjaSpriteDisplayFailure::render_failed,prepared.source_boundary);
    if(!(flags&16))return stop(NinjaSpriteDisplayFailure::world_transform_unavailable,0x77e716);
    const bool alpha=(flags&32)!=0;
    if(!ninja_issue_render_state(r.render,render,27,alpha?1:0,{0x791940,0x791947,alpha?0x791950U:0x791970U}))return stop(NinjaSpriteDisplayFailure::render_failed,r.render.source_boundary);
    if(!ninja_issue_texture_stage_state(r.stage,render,0,4,alpha?4:3,{alpha?0x791956U:0x791976U,alpha?0x79195bU:0x79197bU,alpha?0x791964U:0x791984U}))return stop(NinjaSpriteDisplayFailure::render_failed,r.stage.source_boundary);
    std::uint32_t site=0x77e60a;
    try{
        const auto camera_z=r.geometry->partial.camera_position->z;
        auto loaded=fp.from_float(camera_z);site=0x77e60e;const auto compared=fp.compare_float_memory(loaded,runtime.constants.minimum_depth);
        std::optional<X87Value> depth;
        // TEST AH,05 / JP takes reciprocal/provider for greater/equal/unordered.
        // LESS alone executes actual FLDzero instead.
        if(compared!=X87Comparison::less){
            site=0x77e629;auto reciprocal=fp.from_float(runtime.constants.one);site=0x77e630;reciprocal=fp.divide_float_memory(reciprocal,camera_z);site=0x77e634;r.reciprocal=fp.to_float(reciprocal);
            if(!runtime.projected_depth)return stop(NinjaSpriteDisplayFailure::depth_unavailable,0x77e637);
            r.depth=runtime.projected_depth(*r.reciprocal,fp);if(!*r.depth){r.nested_source_boundary=r.depth->source_boundary;return stop(NinjaSpriteDisplayFailure::depth_unavailable,0x77e637);}depth=*r.depth->value;
        }else {site=0x77e621;depth=fp.from_float(runtime.constants.zero);r.depth=NinjaSpriteDepthResult{depth,0};}
        r.vertices=ninja_sprite_vertices(r.geometry->partial,*depth,fp);
        if(!*r.vertices){r.nested_source_boundary=r.vertices->source_boundary;return stop(NinjaSpriteDisplayFailure::geometry_failed,r.vertices->source_boundary);}
    }catch(const X87Error& e){r.fp_failure=e.failure();r.fp_exceptions=e.exceptions();return stop(NinjaSpriteDisplayFailure::fp_failed,site);}
    if(!render.device_at_3d128b0)return stop(NinjaSpriteDisplayFailure::unknown_device,0x77e6db);
    if(!*render.device_at_3d128b0)return stop(NinjaSpriteDisplayFailure::null_device,0x77e6e0);
    r.format=NinjaSpriteVertexFormatCommand{*render.device_at_3d128b0,0x144,0x77e6e8};
    if(!runtime.set_vertex_format)return stop(NinjaSpriteDisplayFailure::missing_vertex_backend,0x77e6e8);
    const auto format_receipt=runtime.set_vertex_format(*r.format);if(!format_receipt.completed)return stop(NinjaSpriteDisplayFailure::vertex_backend_unavailable,0x77e6e8);r.ignored_format_hresult=format_receipt.original_hresult;
    if(!render.device_at_3d128b0)return stop(NinjaSpriteDisplayFailure::unknown_device,0x77e6f4);
    if(!*render.device_at_3d128b0)return stop(NinjaSpriteDisplayFailure::null_device,0x77e6f9);
    r.issue=NinjaSpriteIssueCommand{*render.device_at_3d128b0,*r.vertices->command,runtime.textures.active_memory_at_3d129a0,std::move(source_record),0x77e703};
    if(!runtime.draw)return stop(NinjaSpriteDisplayFailure::missing_draw_backend,0x77e703);
    const auto draw_receipt=runtime.draw(*r.issue);if(!draw_receipt.completed)return stop(NinjaSpriteDisplayFailure::draw_backend_unavailable,0x77e703);r.ignored_draw_hresult=draw_receipt.original_hresult;
    // Exact78c5a0 is RET. No unrelated cleanup/display effects are invented.
    r.returned=true;return r;
}
} // namespace sadx
