#pragma once
#include "engine/ninja_queue_consumer.hpp"
#include <cstring>
#include <stdexcept>
namespace sadx_test {
struct SpriteAssets {
    sadx::NinjaMath::SineTable table;
    sadx::NinjaMatrix unit;
    sadx::NinjaBlendTables blend;
    std::array<std::uint32_t,8> depth;
    std::array<std::uint32_t,4> returns;
    sadx::NinjaSpriteFrame frame;
    sadx::NinjaSpriteConstants sprite;
    sadx::NinjaQueueConsumerConstants consumer;
};
struct SpriteFixture {
    const SpriteAssets& assets;
    sadx::NinjaMatrixStack stack;
    sadx::NinjaMatrixPrimitives coordinates;
    sadx::X87Context fp;
    sadx::NinjaRenderState render;
    sadx::NinjaTextureSelectionState selection;
    std::vector<std::array<std::uint32_t,5>> trace;
    sadx::NinjaTextureSelectionRuntime texture_runtime;
    sadx::NinjaSpriteProjection projection;
    sadx::NinjaSpriteDisplayRuntime display_runtime;
    std::optional<std::int32_t> display_switch=0;
    std::optional<float> depth_bias=0;
    std::optional<std::array<float,4>> initial_color=std::array<float,4>{1,1,1,1};
    std::shared_ptr<sadx::NinjaRenderQueueArena> arena;
    sadx::NinjaSpriteQueue queue;
    sadx::NinjaSpriteQueueRuntime queue_runtime;
    sadx::NinjaQueueConsumerState consumer_state;
    sadx::NinjaQueueConsumerRuntime consumer_runtime;
    std::shared_ptr<const sadx::NinjaTextureMemoryReference> memory;
    std::shared_ptr<sadx::NinjaTextureListOwner> list;
    std::shared_ptr<sadx::NinjaQueuedSpriteInput> packet;
    SpriteFixture(const SpriteAssets& values,std::uint16_t cw):assets(values),stack({values.unit},1,0),coordinates(values.table,values.unit,stack),fp(cw),
        texture_runtime{render,&values.returns,nullptr,{}},projection{&coordinates,1e-6F,32,32,{}},display_runtime{projection,selection,texture_runtime,values.sprite,{},{},{}},
        arena(std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(65536,0xcd),0,65536,1)),queue(arena),
        consumer_runtime{queue,queue_runtime,consumer_state,display_runtime,values.consumer,initial_color,&values.depth}{
        auto device=std::make_shared<sadx::NinjaRenderDevice>();device->identity=1;render.device_at_3d128b0=device;render.packed_blend_at_3ab9890=0xa8;render.blend_tables=&values.blend;
        render.set_render_state=[this](const auto& c){trace.push_back({0,c.state,c.value,0,c.source_call});return sadx::NinjaRenderCallResult{true,-1};};
        render.set_texture_stage_state=[this](const auto& c){trace.push_back({1,c.stage,c.state,c.value,c.source_call});return sadx::NinjaRenderCallResult{true,-1};};
        texture_runtime.set_texture=[this](const auto& c){trace.push_back({2,c.stage,c.surface_is_null?0U:1U,0,c.source_call});return sadx::NinjaRenderCallResult{true,-1};};
        display_runtime.set_vertex_format=[this](const auto& c){trace.push_back({3,c.format,0,0,c.source_call});return sadx::NinjaRenderCallResult{true,-1};};
        display_runtime.draw=[this](const auto& c){trace.push_back({4,c.vertices.primitive_type,c.vertices.primitive_count,c.vertices.vertex_stride,c.source_call});return sadx::NinjaRenderCallResult{true,-1};};
        auto reference=std::make_shared<sadx::NinjaTextureMemoryReference>();reference->record_ordinal=130;reference->global_id_at_0=7;reference->palette_id_at_4=0xffffffff;reference->format_flags_at_20=0;reference->source_surface_word_at_34=0x11007000;
        // Explicit CPU oracle surface token, never asserted to be a Metal upload.
        reference->runtime_owner=std::make_shared<const int>(1);memory=reference;
        list=std::make_shared<sadx::NinjaTextureListOwner>();list->count=1;list->entries=std::make_shared<sadx::NinjaTextureListOwner::Entries>(131);(**list->entries)[0]=memory;(**list->entries)[130]=memory;list->first_filename_is_invalid_sentinel=false;
        selection.current_list_at_3d0fa24=std::shared_ptr<sadx::NinjaTextureListOwner>{};selection.requested_memory_at_3ce7128=std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};selection.active_memory_at_3d129a0=std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};selection.global_id_at_3d0fdf4=99;
        packet=std::make_shared<sadx::NinjaQueuedSpriteInput>();packet->sprite={{0,0,1e-6F},16,16,0};packet->texture_list=list;packet->animation=std::make_shared<const std::vector<sadx::NinjaSpriteFrame>>(1,values.frame);
        queue.initialize_sort_buckets();auto& q=queue_runtime;q.display_switch_at_3b1117c=&display_switch;q.depth_bias_at_3abd9b8=&depth_bias;q.render_state=&render;q.mode_at_3ab98ac=0;q.flag_at_3abdcfe=0;q.captured_word_at_3abd950=0;q.captured_word_at_3d0f9c8=11;q.captured_word_at_3d0f840=22;q.captured_word_at_3d0f9c4=33;q.color_at_3abd9a8=std::array<float,4>{1,1,1,1};q.color_at_3ab9864=std::array<float,4>{1,1,1,1};q.flag_at_3b12208=6;q.sort_bias_at_3abd9c0=0;q.bucket_bias_at_7dcb60=2048;
        auto& s=consumer_state;s.special_flags_at_909eb4=0;s.fog_profile_at_3abdc6c=0;s.count_at_3abd9d4=3;s.maximum_at_3abd9d8=2;s.count_at_3abd9dc=5;s.maximum_at_3abd9e0=4;s.fog_enabled_at_3d12798=0;s.fog_supported_at_3d128a4=0;s.cached_fog_start_at_90a098=values.consumer.fog_sentinel_at_7dce50;s.cached_fog_end_at_90a09c=values.consumer.fog_sentinel_at_7dce50;
    }
    sadx::NinjaSpriteSubmissionResult enqueue(std::uint32_t flags=0x32){return queue.submit(packet,true,0,flags,0,queue_runtime,&coordinates,fp);}
};
} // namespace sadx_test
