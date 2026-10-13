#include "runtime/game_runtime.hpp"
#include "game/win32_profile.hpp"
#include <cstdio>
#include <cstring>

namespace sadx {
namespace {
std::string hex(std::uint32_t v){char b[16];std::snprintf(b,sizeof b,"%x",v);return b;}
}
NativeGame::NativeGame(NativeGameConfig config):config_(std::move(config)),speedrun_mod_(speedrun_mod_after_init(config_.speedrun_quick_save_enabled)){
    for(auto& r:controllers_raw_)r=std::make_shared<GameLogicController>();
    for(auto& r:input_raw_)r=std::make_shared<GameLogicController>();
    initialize_source_facts();
}
void NativeGame::initialize_source_facts(){
    const auto* exe=config_.executable.get();
    const auto u8=[&](std::uint32_t a)->std::optional<std::uint8_t>{if(!exe)return std::nullopt;return exe->byte(a);};
    const auto u32=[&](std::uint32_t a)->std::optional<std::uint32_t>{if(!exe)return std::nullopt;return exe->u32(a);};
    const auto s32=[&](std::uint32_t a)->std::optional<std::int32_t>{const auto v=u32(a);if(!v)return std::nullopt;return static_cast<std::int32_t>(*v);};
    mode_at_3abdc7c_=s32(0x3abdc7c);
    timing_.word_at_3b11178=s32(0x3b11178);timing_.word_at_3b1117c=s32(0x3b1117c);timing_.word_at_3b11180=s32(0x3b11180);timing_.word_at_3b11188=s32(0x3b11188);
    timing_.word_at_3b1118c=s32(0x3b1118c);timing_.word_at_3b11198=s32(0x3b11198);timing_.word_at_3b2a2e4=s32(0x3b2a2e4);
    logic_.word_at_3abdf70=u32(0x3abdf70);logic_.word_at_3b11194=s32(0x3b11194);
    loop_.word_at_3abdcf8=u32(0x3abdcf8);loop_.exit_at_3abdf7c=u32(0x3abdf7c);loop_.focus_at_8929d4=u32(0x8929d4);loop_.resync_at_3abdf68=u8(0x3abdf68);
    consumer_.special_flags_at_909eb4=u8(0x909eb4);queue_runtime_.flag_at_3abdcfe=u8(0x3abdcfe);consumer_.fog_profile_at_3abdc6c=u8(0x3abdc6c);
    loop_.special_flags_at_909eb4=&consumer_.special_flags_at_909eb4;loop_.fog_flag_at_3abdcfe=&queue_runtime_.flag_at_3abdcfe;
    loop_.fog_range_at_3abdca0=u32(0x3abdca0);loop_.fog_range_at_3abdca4=u32(0x3abdca4);loop_.fog_range_at_3abdc70=u32(0x3abdc70);loop_.fog_range_at_3abdc74=u32(0x3abdc74);
    loop_.request_at_3b291ad=u8(0x3b291ad);loop_.counter_at_3b291b2=u8(0x3b291b2);loop_.pending_at_3abdf76=u8(0x3abdf76);loop_.pending_at_3abdf79=u8(0x3abdf79);
    loop_.save_phase_at_3b291a4=s32(0x3b291a4);loop_.word_at_3abdf80=u32(0x3abdf80);loop_.mode_at_3b28114=u32(0x3b28114);
    loop_.frames_at_3abdf58=u32(0x3abdf58);loop_.frames_at_3abdf5c=u32(0x3abdf5c);loop_.word_at_3c85804=u32(0x3c85804);
    if(const auto lo=u8(0x3b22de4),hi=u8(0x3b22de5);lo&&hi)loop_.game_mode_at_3b22de4=static_cast<std::uint16_t>(*lo|(*hi<<8));
    mode_state_.byte_at_3abdf74=u8(0x3abdf74);mode_state_.byte_at_3abdcfc=u8(0x3abdcfc);mode_state_.byte_at_3abdcfd=u8(0x3abdcfd);mode_state_.byte_at_3b18db4=u8(0x3b18db4);
    mode_state_.byte_at_3b2a2fa=u8(0x3b2a2fa);mode_state_.byte_at_3b22e1e=u8(0x3b22e1e);mode_state_.byte_at_3abdf78=u8(0x3abdf78);
    mode_state_.word_at_3b2c45c=u32(0x3b2c45c);mode_state_.word_at_3b290e4=u32(0x3b290e4);
    // 3b0ef4c is a pointer; a null load-image value stays unknown (a source fault if reached).
    for(unsigned r=0;r<256;++r)for(unsigned k=0;k<3;++k)edges_.raw_at_3b0e3e2[r][k]=u8(0x3b0e3e2+3*r+k);
    for(unsigned i=0;i<256;++i)edges_.aggregate_at_3b0eaf0[i]=u8(0x3b0eaf0+i);
    edges_.special_at_3b0e3ba=u8(0x3b0e3ba);edges_.aggregate_special_at_3b0eae8=u8(0x3b0eae8);edges_.bytes_at_3b0e3e0={u8(0x3b0e3e0),u8(0x3b0e3e1)};edges_.byte_at_3b0e3b9=u8(0x3b0e3b9);
    input_.flags_at_3b0eaa0=u8(0x3b0eaa0);input_.mask_at_3b0e3b8=u8(0x3b0e3b8);
    renderer_.frequency_low_at_3d08538=u32(0x3d08538);renderer_.frequency_high_at_3d0853c=u32(0x3d0853c);renderer_.frame_multiplier_at_389d7dc=s32(0x389d7dc);
    renderer_.render_factor_at_89295c=s32(0x89295c);renderer_.interval_low_at_3d084a8=u32(0x3d084a8);renderer_.interval_high_at_3d084ac=u32(0x3d084ac);
    if(const auto lo=u32(0x3d08550),hi=u32(0x3d08554);lo&&hi)renderer_.last_counter_at_3d08550=static_cast<std::int64_t>((std::uint64_t(*hi)<<32)|*lo);
    for(unsigned w=0;w<52;++w)controller_template_.write_byte(w,u8(0x3b0eaa8+w));
    for(unsigned r=0;r<4;++r)for(unsigned w=0;w<52;++w)controllers_raw_[r]->write_byte(w,u8(0x3b0e9c8+0x34*r+w));
    for(unsigned i=0;i<4;++i){controller_globals_.words_at_3b0e768[i]=u32(0x3b0e768+4*i);controller_globals_.words_at_3b0e344[i]=u32(0x3b0e344+4*i);
        controller_globals_.words_at_3b0e9b8[i]=u32(0x3b0e9b8+4*i);controller_globals_.bytes_at_3b0e9a4[i]=u8(0x3b0e9a4+i);}
    for(unsigned i=0;i<5;++i)controller_globals_.words_at_3b0e990[i]=u32(0x3b0e990+4*i);
    audio_.word_at_3b29ce0=u32(0x3b29ce0);audio_.word_at_3b29ce4=u32(0x3b29ce4);audio_.word_at_3b29b7c=u32(0x3b29b7c);audio_.byte_at_3b29ce8=u8(0x3b29ce8);
    for(unsigned i=0;i<36;++i)audio_.record_words_at_3b292fc[i]=u32(0x3b292fc+0x38*i);for(unsigned i=0;i<18;++i)audio_.words_at_3b292a8[i]=u32(0x3b292a8+4*i);
    for(unsigned i=0;i<48;++i){menu_.block_at_900e88[i]=u8(0x900e88+i);menu_.block_at_9008b4[i]=u8(0x9008b4+i);}menu_.byte_at_3abdbc1=u8(0x3abdbc1);menu_.byte_at_903e58=u8(0x903e58);
    for(unsigned i=0;i<32;++i)exit_callbacks_.slots_at_3d084b0[i]=u32(0x3d084b0+4*i);
    word_at_3abdc80_=u32(0x3abdc80);word_at_3abdc78_=u32(0x3abdc78);word_at_3abdc90_=u32(0x3abdc90);byte_at_3b280a8_=u8(0x3b280a8);
    // Input mod DLL data (its raw_input records and DreamPads) from its own image.
    const auto* dll=config_.input_mod_dll.get();
    for(unsigned i=0;i<8;++i){
        for(unsigned w=0;w<52;++w){if(dll){input_raw_[i]->write_byte(w,dll->byte(0x1000e748+0x34*i+w));pads_[i].record.write_byte(w,dll->byte(0x1000e950+0x60*i+w));}}
        pads_[i].settings=config_.pad_settings[i];
    }
    input_frame_.demo_cutscene_at_3b2a2e8=s32(0x3b2a2e8);input_frame_.game_mode_at_3abdc7c=&mode_at_3abdc7c_;input_frame_.demo_playing_at_3b2a2e4=&timing_.word_at_3b2a2e4;
    // Render queue facts.
    const auto f32=[&](std::uint32_t a)->std::optional<float>{const auto v=u32(a);if(!v)return std::nullopt;float x;std::memcpy(&x,&*v,4);return x;};
    if(const auto a=f32(0x3b18230),b=f32(0x3b18234),c=f32(0x3b18238),d=f32(0x3b1823c);a&&b&&c&&d)color_at_3b18230_=std::array<float,4>{*a,*b,*c,*d};
    if(const auto a=f32(0x3ab9864),b=f32(0x3ab9868),c=f32(0x3ab986c),d=f32(0x3ab9870);a&&b&&c&&d)queue_runtime_.color_at_3ab9864=std::array<float,4>{*a,*b,*c,*d};
    depth_mode_at_8928c8_=u32(0x8928c8);
    rng_.seed_at_2bcbdf8=u32(0x2bcbdf8);
    {std::optional<std::uint16_t> w;if(const auto lo=u8(0x3b29d08),hi=u8(0x3b29d09);lo&&hi)w=static_cast<std::uint16_t>(*lo|(*hi<<8));reset_.emplace(GameResetState{loop_.word_at_3abdf80,w,u32(0x3b290d0),u32(0x3b0e330),rng_});}
    audio_stream_.initialized_at_3b0ef2c=u32(0x3b0ef2c);window_at_3a8b088_=u32(0x3a8b088);
    for(unsigned i=0;i<3;++i){words_at_900840_[i]=u32(0x900840+4*i);words_at_3ab9898_[i]=u32(0x3ab9898+4*i);words_at_900858_[i]=u32(0x900858+4*i);words_at_90084c_[i]=u32(0x90084c+4*i);}
    for(unsigned i=0;i<8;++i)depth_table_[i]=u32(0x88ad9c+4*i).value_or(0);
    if(exe){std::array<std::uint8_t,88> raw{};bool ok=true;for(unsigned i=0;i<88;++i){const auto b=u8(0x88ad64+i);ok=ok&&b;raw[i]=b.value_or(0);}if(ok)blend_tables_=decode_ninja_blend_tables(raw.data(),raw.size());}
    if(const auto v=f32(0x7dcb10))one_at_7dcb10_=*v;if(const auto v=f32(0x7dcb0c))negative_one_at_7dcb0c_=*v;
    queue_runtime_.flag_at_3b12208=u32(0x3b12208);queue_runtime_.mode_at_3ab98ac=u32(0x3ab98ac);queue_runtime_.display_switch_at_3b1117c=&timing_.word_at_3b1117c;
    consumer_.mode_byte_at_3abdbc0=u8(0x3abdbc0);consumer_.count_at_3abd9d4=s32(0x3abd9d4);consumer_.count_at_3abd9dc=s32(0x3abd9dc);consumer_.maximum_at_3abd9d8=s32(0x3abd9d8);consumer_.maximum_at_3abd9e0=s32(0x3abd9e0);
    // D3D8 device owner: native state table (identity 1 = the one source device).
    if(const auto d=u32(0x3d128b0);d&&*d==0)render_.device_at_3d128b0=NinjaRenderDeviceHandle{}; // created by 791610's 7943c0
    render_.packed_blend_at_3ab9890=u8(0x3ab9890);
    render_.blend_tables=blend_tables_?&*blend_tables_:nullptr;
    render_.set_render_state=[this](const NinjaRenderStateCommand& c){d3d8_.render_states[c.state]=c.value;++d3d8_.render_calls;return NinjaRenderCallResult{true,{}};};
    render_.set_texture_stage_state=[this](const NinjaRenderStageCommand& c){if(c.stage==0)d3d8_.stage0_states[c.state]=c.value;++d3d8_.stage_calls;return NinjaRenderCallResult{true,{}};};
    queue_runtime_.render_state=&render_;
    projection_range_.near_at_3d0f808=f32(0x3d0f808);projection_range_.far_at_3d0f968=f32(0x3d0f968);projection_range_.x_at_90082c=f32(0x90082c);projection_range_.y_bits_at_900830=u32(0x900830);
    projection_range_.cache_x_at_3aad0c0=f32(0x3aad0c0);projection_range_.cache_y_at_3aad0c4=f32(0x3aad0c4);projection_range_.negative_one_at_7dcb0c=negative_one_at_7dcb0c_;projection_range_.limit_at_7dce50=f32(0x7dce50).value_or(0);
    transform_owner_.get=[this](std::uint32_t state)->std::optional<NinjaMatrix>{
        if(const auto found=d3d8_.transforms.find(state);found!=d3d8_.transforms.end())return found->second;
        report_.assumptions.push_back("D3D8 transform "+std::to_string(state)+" read before any SetTransform: using the D3D8 device-creation default (identity); pre-40cad0 device setup is unported");
        NinjaMatrix identity{};identity[0]=identity[5]=identity[10]=identity[15]=1.0F;return identity;};
    transform_owner_.set=[this](std::uint32_t state,const NinjaMatrix& m){d3d8_.transforms[state]=m;return true;};
    leaves_.emplace(ResetLeafState{u32(0x3abdf90),u32(0x3abdfac),depth_mode_at_8928c8_,queue_runtime_.mode_at_3ab98ac,u32(0x912694),u32(0x912698),u32(0x91269c),u32(0x9126a8),u32(0x3b29cf4),u32(0x3b29cf8),
        u32(0x3b11190),u32(0x3c5e8b8),timing_.word_at_3b1118c,u32(0x3b291b4),audio_.record_words_at_3b292fc,{},{},audio_stream_.initialized_at_3b0ef2c,
        u32(0x3abdf94),u32(0x3abdf9c),u32(0x3abdfa0),u32(0x3abdf98),u32(0x3abdfa8),u32(0x3abdfa4),u32(0x3a7cf3c),u32(0x38f6ec0),u32(0x3b0e338),{},u32(0x389d608),u32(0x3c4e458),u32(0x3c4e45c),{},{},{}});
    for(unsigned i=0;i<6;++i){leaves_->set_files_at_3abdf40[i]=u32(0x3abdf40+4*i);leaves_->cam_files_at_3abdf28[i]=u32(0x3abdf28+4*i);}
    for(unsigned i=0;i<36;++i){leaves_->sound_record_flags_at_3b29304[i]=u32(0x3b29304+0x38*i);leaves_->words_at_3b29b90[i]=u32(0x3b29b90+4*i);}
    for(unsigned i=0;i<4;++i)leaves_->entry_words_at_3d08428[i]=u32(0x3d08428+24*i);
    if(const auto a=f32(0x3b280ac),b=f32(0x3b280b0),c=f32(0x3b280b4),d=f32(0x3b280b8);a&&b&&c&&d)color_at_3b280ac_=std::array<float,4>{*a,*b,*c,*d};
    queue_base_at_3abd960_=u32(0x3abd960);consumer_.projection_width_at_3d0f9e4=f32(0x3d0f9e4);
    consumer_.fog_enabled_at_3d12798=u32(0x3d12798);consumer_.fog_supported_at_3d128a4=u32(0x3d128a4);consumer_.fog_mode_at_3d0849c=u32(0x3d0849c);
    consumer_.cached_fog_start_at_90a098=f32(0x90a098);consumer_.cached_fog_end_at_90a09c=f32(0x90a09c);fog_sentinel_at_7dce50_=f32(0x7dce50);
    for(unsigned i=0;i<128;++i){fog_table_at_3b10f78_[i]=f32(0x3b10f78+4*i);fog_device_.w_at_3d12a00[i]=f32(0x3d12a00+4*i);fog_device_.reciprocal_at_3d12c00[i]=f32(0x3d12c00+4*i);}
    fog_device_.color_at_3d12898=u32(0x3d12898);fog_device_.start_index_at_3d12950=u32(0x3d12950);fog_device_.end_index_at_3d0fce0=u32(0x3d0fce0);
    fog_device_.start_value_at_3d0fac0=f32(0x3d0fac0);fog_device_.end_value_at_3d12894=f32(0x3d12894);fog_device_.software_density_at_3d128a0=u32(0x3d128a0);
    if(exe){std::array<std::uint8_t,0xed*4> raw{};bool ok=true;for(std::size_t i=0;i<raw.size()&&ok;++i){const auto b=u8(static_cast<std::uint32_t>(0x389d918+i));ok=b.has_value();raw[i]=b.value_or(0);}
        if(ok)keyboard_table_=decode_ninja_keyboard_table(raw.data(),raw.size());}
    frame_calls_.scene_flags_at_3d08588=u32(0x3d08588);frame_calls_.device_flags_at_3d1289c=u32(0x3d1289c);frame_calls_.vertex_buffer_at_3d128ac=u32(0x3d128ac);
    frame_calls_.hook_at_3d0f7a4=u32(0x3d0f7a4);frame_calls_.hook_at_3d0f7a8=u32(0x3d0f7a8);frame_calls_.hook_at_3d0fa30=u32(0x3d0fa30);frame_calls_.argument_at_3d0fa38=u32(0x3d0fa38);
    for(unsigned i=0;i<16;++i)frame_calls_.hooks_at_3d08af0[i]=u32(0x3d08af0+4*i);
    for(unsigned r=0;r<9;++r)for(unsigned w=0;w<4;++w)frame_calls_.records_at_3d0fe00[r][w]=u32(0x3d0fe00+0x24*r+4*w);
    screen_.word_at_3d0f804=u32(0x3d0f804);screen_.flags_at_3d0f964=u32(0x3d0f964);screen_.word_at_3d0f814=u32(0x3d0f814);
    screen_.width_at_3d0fa20=u32(0x3d0fa20);screen_.height_at_3d0f838=u32(0x3d0f838);screen_.height_at_3d0f9e8=f32(0x3d0f9e8);screen_.width_at_3d0fa04=f32(0x3d0fa04);screen_.height_at_3d0fa08=f32(0x3d0fa08);
    screen_.word_at_3ce7160=u32(0x3ce7160);screen_.left_at_3d0f820=f32(0x3d0f820);screen_.top_at_3d0f824=f32(0x3d0f824);screen_.right_at_3d0f828=f32(0x3d0f828);screen_.bottom_at_3d0f82c=f32(0x3d0f82c);
    screen_.points_at_3ce7164=u32(0x3ce7164);screen_.colors_at_3ce7168=u32(0x3ce7168);screen_.word_at_3ce716c=u32(0x3ce716c);screen_.count_at_3ce7170=u32(0x3ce7170);
    for(unsigned i=0;i<8;++i)screen_.points_at_3ce7184[i]=u32(0x3ce7184+4*i);
    screen_.invalid_at_3d0fd1c=u32(0x3d0fd1c);for(unsigned i=0;i<16;++i)screen_.projection_at_3d129c0[i]=u32(0x3d129c0+4*i);
    // Screen device methods (recording owner; a Metal backend replaces it).
    screen_device_.set_viewport=[this](const D3D8FrameCommand&,const std::array<std::uint32_t,6>& v)->std::optional<std::int32_t>{d3d8_.viewport=v;return 0;};
    screen_device_.set_transform=[this](const D3D8FrameCommand&,std::uint32_t state,const D3dxMatrix& m)->std::optional<std::int32_t>{
        NinjaMatrix f{};std::memcpy(f.data(),m.data(),64);d3d8_.transforms[state]=f;return 0;};
    screen_device_.multiply_transform=[this](const D3D8FrameCommand&,std::uint32_t state,const D3dxMatrix& m)->std::optional<std::int32_t>{
        // D3D8 MultiplyTransform: m * current, with host arithmetic (the D3D runtime's own is not modelled).
        const auto current=transform_owner_.get(state);if(!current)return std::nullopt;
        NinjaMatrix f{},out{};std::memcpy(f.data(),m.data(),64);
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){float sum=0;for(unsigned k=0;k<4;++k)sum+=f[4*r+k]*(*current)[4*k+c];out[4*r+c]=sum;}
        d3d8_.transforms[state]=out;return 0;};
    screen_device_.set_vertex_shader=[this](const D3D8FrameCommand&,std::uint32_t)->std::optional<std::int32_t>{++d3d8_.vertex_formats;return 0;};
    screen_device_.draw_primitive_up=[this](const D3D8FrameCommand&,const NinjaScreenPolygon& p)->std::optional<std::int32_t>{d3d8_.polygons.push_back(p);return 0;};
    // 3d0fa34: zero in the load image until 411e30's 77ff50 stores the native 411e20 callable.
    if(const auto v=u32(0x3d0fa34);v&&*v==0)renderer_.input_callback_at_3d0fa34=nullptr;
    winmain_.voice_language_at_3b0ef38=u32(0x3b0ef38);winmain_.text_language_at_3b0f0e8=u32(0x3b0f0e8);winmain_.clip_level_at_3abdcf0=u32(0x3abdcf0);
    winmain_.bgm_volume_at_909f28=u32(0x909f28);winmain_.screen_at_892950=u32(0x892950);winmain_.sound3d_at_3b0ef28=u32(0x3b0ef28);winmain_.mouse_mode_at_3b0eae0=u32(0x3b0eae0);
    for(unsigned i=0;i<13;++i)if(const auto lo=u8(0x389d634+2*i),hi=u8(0x389d635+2*i);lo&&hi)winmain_.pad_buttons_at_389d634[i]=static_cast<std::uint16_t>(*lo|(*hi<<8));
    for(unsigned i=0;i<17;++i)winmain_.pad_bits_at_8929d8[i]=s32(0x8929d8+4*i);
    memory_.heap_at_389d628=u32(0x389d628);word_at_3d0f7a0_=u32(0x3d0f7a0);
    for(unsigned i=0;i<9;++i)library_.words_at_389d864[i]=u32(0x389d864+4*i);
    library_.word_at_38a5dc4=u32(0x38a5dc4);library_.word_at_38a5dc8=u32(0x38a5dc8);library_.word_at_38a5dcc=u32(0x38a5dcc);library_.scale_at_3d0f800=f32(0x3d0f800);library_.aspect_at_3d0f9c0=f32(0x3d0f9c0);
    for(std::size_t i=0;i<device_.block_at_3d0fae0.size;++i)device_.block_at_3d0fae0.bytes[i]=u8(static_cast<std::uint32_t>(0x3d0fae0+i));
    for(std::size_t i=0;i<device_.caps_at_3d127c0.size;++i)device_.caps_at_3d127c0.bytes[i]=u8(static_cast<std::uint32_t>(0x3d127c0+i));
    device_.direct3d_at_3d11f60=u32(0x3d11f60);for(unsigned i=0;i<6;++i)device_.viewport_at_3d12780[i]=u32(0x3d12780+4*i);
    device_.half_width_at_3d12948=f32(0x3d12948);device_.half_height_at_3d1294c=f32(0x3d1294c);
    for(unsigned i=0;i<7;++i)device_.words_at_3d0fd00[i]=u32(0x3d0fd00+4*i);
    for(unsigned i=0;i<16;++i){device_.matrix_at_3d0fd80[i]=u32(0x3d0fd80+4*i);device_.matrix_at_389d318[i]=u32(0x389d318+4*i);}
    device_.left_at_3d0fd20=f32(0x3d0fd20);device_.top_at_3d0fd24=f32(0x3d0fd24);device_.right_at_3d0fd28=f32(0x3d0fd28);device_.bottom_at_3d0fd2c=f32(0x3d0fd2c);
    for(std::size_t i=0;i<cd_drive_.root_at_3a8af78.size;++i)cd_drive_.root_at_3a8af78.bytes[i]=u8(static_cast<std::uint32_t>(0x3a8af78+i));
    winmain_.media_at_3ab918c=u32(0x3ab918c);winmain_.media_at_3ab9190=u32(0x3ab9190);winmain_.message_text_at_8929b0=u32(0x8929b0);winmain_.message_caption_at_8929c4=u32(0x8929c4);
    window_.last_wparam_at_3d08534=u32(0x3d08534);input_devices_.joysticks_at_3d08814=u32(0x3d08814);input_devices_.mouse_at_3d08834=u32(0x3d08834);
    for(unsigned i=0;i<4;++i){vibration_.strength_at_3d0842e[i]=u8(0x3d0842e +0x18*i);vibration_.duration_at_3d08434[i]=u32(0x3d08434+0x18*i);vibration_.counter_at_3d0843c[i]=u32(0x3d0843c+0x18*i);}window_at_3d0fd30_=u32(0x3d0fd30);instance_at_3d0fd34_=u32(0x3d0fd34);keyboard_.input_pointer_at_3b0e340=u32(0x3b0e340);
    system_.word_at_3d0f83c=u32(0x3d0f83c);system_.word_at_3d0f810=u32(0x3d0f810);system_.pool_at_3ce71b0=u32(0x3ce71b0);system_.size_at_3ce71a4=u32(0x3ce71a4);
    system_.color_at_3ce715c=u32(0x3ce715c);system_.buffer_at_3ce7158=u32(0x3ce7158);system_.scale_at_3ce7154=f32(0x3ce7154);system_.word_at_3d127a0=u32(0x3d127a0);
    system_.word_at_3d0f740=u32(0x3d0f740);system_.flag_at_3ce7150=u32(0x3ce7150);system_.counter_low_at_3ce7130=u32(0x3ce7130);system_.counter_high_at_3ce7134=u32(0x3ce7134);
    system_.direct_input_at_3abdc10=u32(0x3abdc10);system_.mouse_at_3abdc14=u32(0x3abdc14);
    for(std::size_t i=0;i<system_.records_at_3ce6f48.size;++i)system_.records_at_3ce6f48.bytes[i]=u8(static_cast<std::uint32_t>(0x3ce6f48+i));
    for(std::size_t i=0;i<system_.records_at_3ce6e78.size;++i)system_.records_at_3ce6e78.bytes[i]=u8(static_cast<std::uint32_t>(0x3ce6e78+i));
    for(std::size_t i=0;i<system_.block_at_3ce6d80.size;++i)system_.block_at_3ce6d80.bytes[i]=u8(static_cast<std::uint32_t>(0x3ce6d80+i));
    texture_pool_.base_at_3d0f844=u32(0x3d0f844);texture_pool_.count_at_3d0f80c=u32(0x3d0f80c);texture_pool_.word_at_3d0f830=u32(0x3d0f830);texture_pool_.word_at_3d0fa14=u32(0x3d0fa14);
    texture_pool_.word_at_3d0f960=u32(0x3d0f960);texture_pool_.byte_at_3d0f860=u8(0x3d0f860);for(unsigned i=0;i<8;++i)texture_pool_.words_at_3d0f7c0[i]=u32(0x3d0f7c0+4*i);
    for(std::size_t i=0;i<texture_pool_.records_at_3a8b0a0.size;++i)texture_pool_.records_at_3a8b0a0.bytes[i]=u8(static_cast<std::uint32_t>(0x3a8b0a0+i));
    callbacks_.count_at_3ce712c=u32(0x3ce712c);keyboard_.records_at_3d08818=u32(0x3d08818);keyboard_.count_at_3d0881c=u32(0x3d0881c);
    if(const auto d=u32(0x3d08820);d&&*d==0)keyboard_.device_known=true; // a nonzero load-image COM pointer has no native owner
    for(unsigned i=0;i<4;++i){auto& rec=keyboard_.records_at_3ce6e38[i];const auto at=0x3ce6e38+0xc*i;rec.modifiers=u8(at);rec.byte1=u8(at+1);for(unsigned k=0;k<6;++k)rec.keys[k]=u8(at+2+k);rec.pointer=u32(at+8);
        for(unsigned k=0;k<4;++k)keyboard_.words_at_3ce6e68[i][k]=u8(0x3ce6e68+4*i+k);}
    {   // 40c090 leaves: words without another owner start from the load image.
        const auto u16=[&](std::uint32_t a)->std::optional<std::uint16_t>{const auto lo=u8(a),hi=u8(a+1);if(!lo||!hi)return std::nullopt;return static_cast<std::uint16_t>(*lo|(*hi<<8));};
        if(const auto v=u32(0x3b2c578);v&&*v==0)scene_globals_.event_task_at_3b2c578=TaskHandle{};
        sonic_globals_.stage_at_3b22dcc=u16(0x3b22dcc);
        mode_leaves_.emplace(mode_state_,timing_,input_frame_.demo_cutscene_at_3b2a2e8,scene_globals_.event_task_at_3b2c578,rng_,loop_.game_mode_at_3b22de4,sonic_globals_.stage_at_3b22dcc);
        auto& m=*mode_leaves_;
        m.character_at_3b22dc0=u16(0x3b22dc0);
        for(auto [o,a]:{std::pair{&m.byte_at_3b0ef34,0x3b0ef34u},{&m.byte_at_3b0ef35,0x3b0ef35u},{&m.byte_at_3b0f128,0x3b0f128u},{&m.byte_at_3b0ef48,0x3b0ef48u},{&m.byte_at_3abe134,0x3abe134u},{&m.byte_at_3b0f0e0,0x3b0f0e0u},
            {&m.byte_at_3b290d8,0x3b290d8u},{&m.byte_at_3b290e0,0x3b290e0u},{&m.byte_at_3b290e8,0x3b290e8u},{&m.byte_at_3b2a2f8,0x3b2a2f8u},{&m.byte_at_3b2a2f9,0x3b2a2f9u},{&m.byte_at_3b2a2fb,0x3b2a2fbu},
            {&m.byte_at_3b2c565,0x3b2c565u},{&m.byte_at_3b2c574,0x3b2c574u}})*o=u8(a);
        for(auto [o,a]:{std::pair{&m.word_at_3b0f13c,0x3b0f13cu},{&m.word_at_3b0f108,0x3b0f108u},{&m.word_at_3b0ef44,0x3b0ef44u},{&m.word_at_3b290dc,0x3b290dcu},{&m.word_at_3b2a2f0,0x3b2a2f0u},{&m.word_at_3b2a2f4,0x3b2a2f4u},
            {&m.word_at_3b2a2fc,0x3b2a2fcu},{&m.word_at_3b2b358,0x3b2b358u},{&m.word_at_3b2c47c,0x3b2c47cu},{&m.word_at_3b2919c,0x3b2919cu},{&m.word_at_3b0f0f8,0x3b0f0f8u},{&m.word_at_3b0f0fc,0x3b0f0fcu},
            {&m.word_at_3b0f100,0x3b0f100u},{&m.word_at_3b0f10c,0x3b0f10cu},{&m.word_at_3b0f0f4,0x3b0f0f4u},{&m.word_at_3b2c560,0x3b2c560u},{&m.word_at_914788,0x914788u},{&m.word_at_3b18244,0x3b18244u}})*o=u32(a);
        m.half_at_3b0f0e4=u16(0x3b0f0e4);m.half_at_3b2a300=u16(0x3b2a300);
        const auto load=[&](auto& block){for(std::size_t i=0;i<block.size;++i)block.bytes[i]=u8(static_cast<std::uint32_t>(block.base+i));};
        load(m.block_at_3b18250);load(m.block_at_3b0ef9c);load(m.block_at_3c4ae08);load(m.block_at_3b290c4);load(m.block_at_90ed54);load(m.block_at_3b2b5f9);m.color_at_3b29d5c=u32(0x3b29d5c);m.fp=&fp_;
        m.half_at_3b22e1c=u16(0x3b22e1c);m.half_at_3b18db8=u16(0x3b18db8);m.word_at_3b22e20=u32(0x3b22e20);m.callback_at_3d08420=u32(0x3d08420);
    }
    for(unsigned i=0;i<36;++i){const auto at=0x3b292fc+0x38*i;auto& f=sound_fields_;
        f.words_at_3b29300[i]=u32(at+4);f.ids_at_3b29308[i]=u32(at+0xc);f.frequency_at_3b2930c[i]=u32(at+0x10);f.volume_at_3b29310[i]=u32(at+0x14);f.pan_at_3b29318[i]=u32(at+0x1c);
        f.current_volume_at_3b29c28[i]=u32(0x3b29c28+4*i);}
    sound_fields_.listener_at_3b2cbb0=u32(0x3b2cbb0);
    frame_owners_.list_at_3b2c584=u32(0x3b2c584);frame_owners_.count_at_3b0e334=u32(0x3b0e334);frame_owners_.timer_at_3abdf88=u32(0x3abdf88);frame_owners_.buffer_at_3abdf84=u32(0x3abdf84);
    level_fog_start_at_3abdc60_=f32(0x3abdc60);level_fog_end_at_3abdc64_=f32(0x3abdc64);level_fog_color_at_3abdc68_=u32(0x3abdc68);
    {NinjaMath::SineTable t{};bool ok=exe!=nullptr;for(unsigned i=0;i<1025&&ok;++i){const auto v=f32(0x88af78+4*i);ok=v.has_value();if(ok)t[i]=*v;}if(ok)sine_table_at_88af78_=t;}
    view_.emplace(GameViewState{f32(0x8928c0),f32(0x8928c4),f32(0x7dcb28).value_or(0),f32(0x7dcb24).value_or(0),f32(0x7dcc98).value_or(0),f32(0x7dd594).value_or(0),f32(0x7dd590).value_or(0),
        byte_at_3b280a8_,loop_.mode_at_3b28114,u32(0x3abd9cc),u32(0x3b28094),f32(0x3b28098),f32(0x3b2809c),f32(0x3b280a0),f32(0x3b280a4),u32(0x3d0f9ec),u32(0x3d0f9f0),u32(0x3aad0bc),u32(0x3ab98ec),u32(0x3b2cbb4),
        {u32(0x3ce7174),u32(0x3ce7178),u32(0x3ce717c),u32(0x3ce7180)},color_at_3b280ac_});
    mode_leaves_->screen_width_at_8928c0=&view_->screen_width_at_8928c0;mode_leaves_->screen_height_at_8928c4=&view_->screen_height_at_8928c4;
    mode_leaves_->clear_colors_at_3ce7174=&view_->clear_colors_at_3ce7174;
    mode_leaves_->word_at_3b2c420=u32(0x3b2c420);mode_leaves_->state_at_3b29d6c=u32(0x3b29d6c);mode_leaves_->word_at_3b29d68=u32(0x3b29d68);
    for(std::size_t i=0;i<mode_leaves_->block_at_3b2c5e8.size;++i)mode_leaves_->block_at_3b2c5e8.bytes[i]=u8(static_cast<std::uint32_t>(0x3b2c5e8+i));
    {NinjaMatrix unit{};bool ok=exe!=nullptr;for(unsigned i=0;i<16&&ok;++i){const auto v=f32(0x389d650+4*i);ok=v.has_value();if(ok)unit[i]=*v;}if(ok)unit_at_389d650_=unit;}
    {   // 4086f0 consumer composition from the load image.
        const auto bytes=[&](std::initializer_list<std::pair<std::uint32_t,unsigned>> runs,std::vector<std::uint8_t>& out){
            for(const auto& [a,n]:runs)for(unsigned i=0;i<n;++i){const auto b=u8(a+i);if(!b)return false;out.push_back(*b);}return true;};
        std::vector<std::uint8_t> raw;
        if(bytes({{0x7db2c4,4},{0x7dce6c,4},{0x7e9b40,4},{0x7dcb2c,4},{0x7dcb10,4}},raw))sprite_constants_=decode_ninja_sprite_constants(raw.data(),raw.size());
        raw.clear();if(bytes({{0x7dcb0c,4},{0x7dce50,4}},raw))consumer_constants_=decode_ninja_queue_consumer_constants(raw.data(),raw.size());
        for(unsigned i=0;i<4;++i)texture_returns_at_389d458_[i]=u32(0x389d458+4*i).value_or(0);
        const auto null_or_unknown=[&](std::uint32_t a,auto& field){if(const auto v=u32(a);v&&*v==0)field=typename std::decay_t<decltype(field)>::value_type{};};
        null_or_unknown(0x3d0fa24,texture_selection_.current_list_at_3d0fa24);null_or_unknown(0x3ce7128,texture_selection_.requested_memory_at_3ce7128);
        null_or_unknown(0x3d129a0,texture_selection_.active_memory_at_3d129a0);texture_selection_.global_id_at_3d0fdf4=u32(0x3d0fdf4);
        texture_runtime_.emplace(NinjaTextureSelectionRuntime{render_,&texture_returns_at_389d458_,nullptr,[this](const NinjaTextureBindCommand&){++d3d8_.texture_binds;return NinjaRenderCallResult{true,{}};}});
        if(sprite_constants_&&consumer_constants_){
            display_runtime_.emplace(NinjaSpriteDisplayRuntime{projection_,texture_selection_,*texture_runtime_,*sprite_constants_,
                [this](float w,X87Context& fp)->NinjaSpriteDepthResult{NinjaDepthRuntime d{screen_,device_,sse_,fp,sse_reciprocal_by_division};
                    report_screen_depth();const auto r=ninja_projected_depth_78dbe0(w,d);return {r.value,r.source_boundary};},
                [this](const NinjaSpriteVertexFormatCommand&){++d3d8_.vertex_formats;return NinjaRenderCallResult{true,{}};},
                [this](const NinjaSpriteIssueCommand& c){d3d8_.draws.push_back(c);return NinjaRenderCallResult{true,{}};}});
            consumer_runtime_.emplace(NinjaQueueConsumerRuntime{queue_,queue_runtime_,consumer_,*display_runtime_,*consumer_constants_,color_at_3b18230_,&depth_table_});
        }
    }
    lifecycle_.emplace(NinjaQueueLifecycleOwners{queue_,queue_runtime_,consumer_,timing_,projection_,color_at_3b18230_,&depth_table_,one_at_7dcb10_,negative_one_at_7dcb0c_,depth_mode_at_8928c8_,
        words_at_900840_,words_at_3ab9898_,words_at_900858_,words_at_90084c_});
}
void NativeGame::report_screen_depth(){
    if(screen_depth_reported_)return;screen_depth_reported_=true;
    report_.assumptions.push_back("78dbe0 -> D3DX8 772615: the reference CPU is taken to have SSE without 3DNow! and no DisableD3DXPSGP registry value, so the dispatch table holds 774310/774150; RCPPS is modelled as IEEE division (the reference CPU's estimate table was not captured)");
    bool zero=true;for(const auto& w:screen_.projection_at_3d129c0)zero=zero&&w&&*w==0;
    if(zero)report_.assumptions.push_back("78dbe0: 3d129c0 (the projection matrix) keeps load-image zeros because its writer 791170 (reached from WinMain's 781570) is not run natively, so projected depths are NaN");
}
NinjaFogOwners NativeGame::fog_owners(){
    return {consumer_,queue_runtime_.flag_at_3abdcfe,queue_runtime_.display_switch_at_3b1117c,&render_,fog_sentinel_at_7dce50_.value_or(0),&fog_device_,&fog_table_at_3b10f78_,
        &level_fog_start_at_3abdc60_,&level_fog_end_at_3abdc64_,&level_fog_color_at_3abdc68_};
}
GameLogicOwnerResult NativeGame::unported(std::uint32_t callee,std::uint32_t site,const std::string& path){
    const auto found=unported_index_.find(callee);
    if(found==unported_index_.end()){unported_index_[callee]=report_.unported.size();report_.unported.push_back({callee,site,path+">"+hex(callee),1});}
    else ++report_.unported[found->second].calls;
    if(config_.policy==OwnerPolicy::stop_at_unported){if(!report_.stopped_at)report_.stopped_at=report_.unported[unported_index_[callee]];return {GameLogicOwnerState::unavailable,callee,{}};}
    return {GameLogicOwnerState::completed,0,{}};
}
NativeGameReport NativeGame::run(){
    report_={};unported_index_.clear();failure_detail_.clear();
    const auto done=[](GameLogicOwnerState s){return GameLogicOwnerResult{s,0,{}};};
    const auto status=[&](bool ok,std::uint32_t at){return ok?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::failed,at,{}};};
    // 785330 pdVibMxStop: the pinned Input mod's Jumps replace it with rumble::pdVibMxStop_r (EAX 0).
    const auto vibration_stop=[&](const std::string& path)->GameLogicOwnerResult{
        const auto ok=input_mod_vibration_stop(rumble_tasks_,pads_,{[&](std::uint32_t){return unported(0x4b4160,0x10008d97,path+">785330(Input pdVibMxStop_r)").state==GameLogicOwnerState::completed;},
            [](std::size_t){return true;}}); // SDL rumble stop: no native haptics owner yet (no pad has rumble flags until RumbleA_r/RumbleB_r are ported)
        return ok?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::unavailable,0x785330,{}};};
    // 40b1a0 (64fc70, then the 16 MiB heap: TaskWorkArena reproduces its fill and headers) and
    // 40b460 (51a740, then the 4096-slot pool: list heads cleared by the fresh scheduler).
    const auto task_reset=[&](std::uint32_t callee)->GameLogicOwnerResult{
        if(callee==0x40b1a0){word_at_3c839fc_=0;word_at_3c83a00_=0;task_arena_=std::make_shared<TaskWorkArena>();return done(GameLogicOwnerState::completed);}
        for(auto& w:words_at_3c60108_)w=0;
        if(!task_arena_)return GameLogicOwnerResult{GameLogicOwnerState::failed,0x40b46b,{}};
        scheduler_=std::make_unique<TaskScheduler>();const auto r=scheduler_->initialize_source_pool(task_arena_);
        return r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::failed,0x40b466,{}};};
    const auto reset=[&](const std::string& path)->GameLogicOwnerResult{
        GameResetRuntime rr{*reset_,[&](const GameResetCall& c)->GameLogicOwnerResult{
            const ResetLeafNested nested=[&](const ResetLeafCall& n){return unported(n.callee,n.source_call,path+">40bf40>"+hex(c.callee));};
            if(c.callee==0x40b1a0||c.callee==0x40b460)return task_reset(c.callee);
            if(c.callee==0x785330)return vibration_stop(path+">40bf40");
            if(c.callee==0x4208e0){
                GameViewRuntime vr{*view_,*lifecycle_,queue_base_at_3abd960_.value_or(0),[&](const GameViewCall& n)->GameLogicOwnerResult{
                    if(n.callee==0x403180){float a,b;std::memcpy(&a,&*n.arguments[0],4);std::memcpy(&b,&*n.arguments[1],4);const auto f=ninja_set_projection_range_403180(a,b,projection_range_,transform_owner_,fp_);
                        return f?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::failed,f.source_boundary,{}};}
                    if(n.callee==0x7815c0&&sine_table_at_88af78_){const auto f=ninja_set_fov_7815c0(static_cast<std::int32_t>(*n.arguments[0]),*sine_table_at_88af78_,projection_,consumer_.projection_width_at_3d0f9e4,fp_);
                        return f?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::failed,f.source_boundary,{}};}
                    return unported(n.callee,n.source_call,path+">40bf40>4208e0");}};
                const auto r=run_game_view_4208e0(vr,fp_);
                return r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==GameViewFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}};
            }
            if(c.callee==0x411af0||c.callee==0x411b40){
                if(!fog_sentinel_at_7dce50_)return GameLogicOwnerResult{GameLogicOwnerState::failed,0x4119b0,{}};
                auto owners=fog_owners();
                const auto f=ninja_fog_411af0_411b40(c.callee==0x411af0,owners,fp_);
                if(f.failure==NinjaFogFailure::unlifted_callee)return unported(f.callee_boundary,f.source_boundary,path+">40bf40>"+hex(c.callee));
                return status(bool(f),f.source_boundary);}
            const auto leaf=run_reset_leaf(c.callee,c.argument,*leaves_,nested);
            if(leaf.outcome==ResetLeafOutcome::completed)return done(GameLogicOwnerState::completed);
            if(leaf.outcome==ResetLeafOutcome::not_a_leaf)return unported(c.callee,c.source_call,path+">40bf40");
            if(leaf.outcome==ResetLeafOutcome::nested_failed&&leaf.nested.state==GameLogicOwnerState::unavailable&&leaf.nested.source_boundary==leaf.source_boundary)
                return unported(leaf.source_boundary,leaf.source_boundary,path+">40bf40>"+hex(c.callee)+"(unported body)");
            return {leaf.outcome==ResetLeafOutcome::nested_failed?leaf.nested.state:GameLogicOwnerState::failed,leaf.source_boundary,{}};}};
        const auto r=run_game_reset_40bf40(rr);
        return r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==GameResetFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}};
    };
    // 40c030 owners.
    StartupRuntime startup{{edges_,input_,controller_globals_,controller_template_,controllers_raw_,controller_pointers_,&input_raw_,audio_,menu_,exit_callbacks_,mode_at_3abdc7c_,word_at_3abdc80_,word_at_3abdc78_,word_at_3abdc90_},{}};
    startup.invoke=[&](const StartupCall& c)->GameLogicOwnerResult{
        switch(c.operation){
        // Profile: the loader WriteCalls 40c045 to EventGameLoopInit (dllmain.cpp:1647): no enabled mod exports
        // OnInitGameLoop and CrashGuard is off, so like 78c5a0's RET it changes no state; 40c04a does not read EAX.
        case StartupOperation::ret_78c5a0:return done(GameLogicOwnerState::completed);
        case StartupOperation::set_mode_411e30:{const auto r=set_game_logic_mode(1,1,timing_,renderer_);return status(bool(r),r.source_boundary);}
        case StartupOperation::reset_40bf40:return reset("40cad0>40c030");
        case StartupOperation::audio_410510:{
            AudioStreamOwners owners;
            owners.initialize_device=[this](AudioSoundObject&,const AudioDeviceRequest&)->std::optional<std::int32_t>{
                report_.assumptions.push_back("410510/44de80: no native audio backend yet; the device owner reports DSERR_NODRIVER and the source continues without sound (the Windows reference has sound)");
                return static_cast<std::int32_t>(0x88780078u);};
            const auto r=run_audio_stream_init_410510(audio_stream_,window_at_3a8b088_,owners);return status(bool(r),r.source_boundary);}
        case StartupOperation::queue_408630:{const auto r=ninja_queue_allocate_408630(*lifecycle_,std::nullopt,fp_);
            if(r.alignment_assumed)report_.assumptions.push_back("408630: Windows HeapAlloc address unknown; render-queue cursor assumes a 32-byte-aligned base");
            return status(bool(r),r.source_boundary);}
        default:{
            const auto leaf=run_reset_leaf(c.callee,{},*leaves_,[&](const ResetLeafCall& n){return unported(n.callee,n.source_call,"40cad0>40c030>"+hex(c.callee));});
            if(leaf.outcome==ResetLeafOutcome::not_a_leaf)return unported(c.callee,c.source_call,"40cad0>40c030");
            if(leaf.outcome==ResetLeafOutcome::completed)return done(GameLogicOwnerState::completed);
            return {leaf.outcome==ResetLeafOutcome::nested_failed?leaf.nested.state:GameLogicOwnerState::failed,leaf.source_boundary,{}};}
        }
    };
    // 40c090 owners.
    GameModeRuntime mode{loop_,mode_at_3abdc7c_,mode_state_,{}};
    mode.invoke=[&](const GameModeCall& c)->GameModeOwnerResult{
        if(c.callee==0x411e30){const auto r=set_game_logic_mode(static_cast<std::int32_t>(*c.arguments[0]),static_cast<std::int32_t>(*c.arguments[1]),timing_,renderer_);return {status(bool(r),r.source_boundary),0};}
        if(c.callee==0x77ed40){const auto r=set_game_frame_multiplier(static_cast<std::int32_t>(*c.arguments[0]),renderer_);return {status(bool(r),r.source_boundary),0};}
        if(c.callee==0x40bf40)return {reset("40cad0>40c090"),0};
        if(c.callee==0x426050){ // 426050, then the loader's ProcessCodes at its RET (426063); 40c9b6 does not read EAX
            const auto leaf=run_game_mode_leaf(0x426050,c.arguments[0],*mode_leaves_,{});
            if(leaf.outcome!=GameModeLeafOutcome::completed)return {{GameLogicOwnerState::failed,leaf.source_boundary,{}},std::nullopt};
            SpeedrunFrameRuntime frame_events{speedrun_mod_,loop_.game_mode_at_3b22de4,mode_at_3abdc7c_,sonic_globals_.stage_at_3b22dcc,
                [&](std::uint32_t site,std::uint32_t callee){return unported(callee,site,"40cad0>40c090>426050>426063(ProcessCodes: Speedrun OnFrame)");}};
            const auto r=run_mod_loader_process_codes(frame_events);
            if(r.required_fact)failure_detail_=" (Speedrun OnFrame requires "+hex(*r.required_fact)+")";
            if(r)return {done(GameLogicOwnerState::completed),std::nullopt};
            return {{r.failure==ModFrameFailure::missing_owner?GameLogicOwnerState::unavailable:r.failure==ModFrameFailure::cancelled?GameLogicOwnerState::cancelled:GameLogicOwnerState::failed,r.source_boundary,{}},std::nullopt};
        }
        const auto leaf=run_game_mode_leaf(c.callee,c.arguments[0],*mode_leaves_,[&](const GameModeLeafCall& n)->GameLogicOwnerResult{
            if(n.callee==0x4030d0){const auto r=ninja_set_blend(*n.arguments[0],*n.arguments[1],render_);return status(bool(r),r.source_boundary);}
            if(n.callee==0x785330)return vibration_stop("40cad0>40c090>"+hex(c.callee));
            if(n.callee==0x40b1a0||n.callee==0x40b460)return task_reset(n.callee);
            if(n.callee==0x411e30){const auto r=set_game_logic_mode(static_cast<std::int32_t>(*n.arguments[0]),static_cast<std::int32_t>(*n.arguments[1]),timing_,renderer_);return status(bool(r),r.source_boundary);}
            if(n.callee==0x77ed40){const auto r=set_game_frame_multiplier(static_cast<std::int32_t>(*n.arguments[0]),renderer_);return status(bool(r),r.source_boundary);}
            return unported(n.callee,n.source_call,"40cad0>40c090>"+hex(c.callee));});
        if(leaf.outcome==GameModeLeafOutcome::completed)return {done(GameLogicOwnerState::completed),leaf.eax};
        if(leaf.outcome==GameModeLeafOutcome::not_a_leaf)return {unported(c.callee,c.source_call,"40cad0>40c090"),0};
        return {{leaf.outcome==GameModeLeafOutcome::nested_failed?leaf.nested.state:GameLogicOwnerState::failed,leaf.source_boundary,{}},std::nullopt};
    };
    // 411d00 owners.
    // 411d00's D3D8 frame thunks. Device methods a configured backend does not
    // supply fall back to the recording owner (D3D8StateOwner, D3D_OK).
    D3D8FrameDevice frame_device=config_.frame_device;bool recording_device=false;
    if(!frame_device.begin_scene){recording_device=true;frame_device.begin_scene=[this](const D3D8FrameCommand&){++d3d8_.begin_scenes;return std::optional<std::int32_t>{0};};}
    if(!frame_device.end_scene){recording_device=true;frame_device.end_scene=[this](const D3D8FrameCommand&){++d3d8_.end_scenes;return std::optional<std::int32_t>{0};};}
    if(!frame_device.present){recording_device=true;frame_device.present=[this](const D3D8FrameCommand&){++d3d8_.presents;return std::optional<std::int32_t>{0};};}
    if(!frame_device.clear){recording_device=true;frame_device.clear=[this](const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t,float,std::uint32_t){++d3d8_.clears;d3d8_.last_clear_flags=flags;return std::optional<std::int32_t>{0};};}
    if(!frame_device.get_render_state)frame_device.get_render_state=[this](const D3D8FrameCommand&,std::uint32_t state)->std::optional<std::uint32_t>{
        if(const auto found=d3d8_.render_states.find(state);found!=d3d8_.render_states.end())return found->second;
        // D3D8 device-creation defaults: ZWRITEENABLE TRUE, ZFUNC LESSEQUAL. Other states have no recorded value.
        if(state!=0xe&&state!=0x17)return std::nullopt;
        report_.assumptions.push_back("D3D8 render state "+std::to_string(state)+" read before any SetRenderState: using the D3D8 device-creation default; pre-40cad0 device setup (7943c0) is unported");
        return state==0xe?1U:4U;};
    bool frame_calls_reported=false;
    FrameCompletionRuntime frame{loop_,logic_,renderer_,FrameCompletionPolicy::speedrun_2_0,{}};
    frame.invoke=[&](const FrameCompletionCall& c)->GameLogicOwnerResult{
        if(c.operation==FrameCompletionOperation::limiter_781060){const auto r=run_speedrun_frame_limiter(renderer_,timing_,config_.limiter_cadence,config_.clock);return status(bool(r),r.source_boundary);}
        if(!frame_calls_reported){frame_calls_reported=true;
            std::string order;for(std::uint32_t i=0;i<callbacks_.count_at_3ce712c.value_or(0)&&i<callbacks_.slots_at_3d0fa40.size();++i)order+=(order.empty()?"":", ")+(callbacks_.slots_at_3d0fa40[i]?hex(*callbacks_.slots_at_3d0fa40[i]):std::string("unknown"));
            report_.assumptions.push_back("780030/7959a0: natively the 3d0fa40 callback list holds the ported registrations in order ("+order+") and the 3d08af0 hook table holds 789bd0; the skipped 788000 would also register the 78818f callback and the 795380 hook (788007>795420) on the reference");
            report_.assumptions.push_back("78ba50: the 3d0fe00 dynamic vertex-buffer records hold only 791610's zero stores (their creators among the skipped device owners are unported), so Clear's 794800 walk finds no buffers");
            if(recording_device)report_.assumptions.push_back("D3D8 frame methods: no display backend configured; the recording owner returns D3D_OK for BeginScene/EndScene/Clear/Present");}
        const std::string path="40cad0>411d00>"+hex(c.callee);
        NinjaFrameRuntime rt{frame_calls_,callbacks_,renderer_,&input_,render_,frame_device,NinjaPresentPolicy::mod_loader,
            [&](const NinjaFrameCall& n)->GameLogicOwnerResult{
                if(n.callee==0x77f090){ // keyboard poll (77f090 -> 791bd0), registered by 77f0a0
                    if(!keyboard_table_)return {GameLogicOwnerState::failed,0x791c65,{}};
                    const auto k=ninja_keyboard_poll_791bd0(keyboard_,*keyboard_table_);return status(bool(k),k.source_boundary);}
                if(n.callee==0x77f110||n.callee==0x77efc0){ // jmp 792100 (joysticks) / jmp 791df0 (mouse)
                    const NinjaInputDeviceBody body=[&](std::uint32_t b){return unported(b,b,path+">"+hex(n.callee));};
                    const auto d=n.callee==0x77f110?run_joystick_poll_792100(input_devices_,body):run_mouse_poll_791df0(input_devices_,body);
                    if(d)return {GameLogicOwnerState::completed,0,{}};
                    return {d.failure==NinjaInputDeviceFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,d.source_boundary,{}};}
                if(n.callee==0x785190){ // the vibration update 785240 registered
                    NinjaVibrationRuntime vib{vibration_,leaves_->entry_words_at_3d08428,[&](const NinjaVibrationCall& v)->std::pair<GameLogicOwnerResult,std::optional<std::uint32_t>>{
                        return {unported(v.callee,v.source_call,path+">785190"),std::nullopt};}};
                    const auto v=run_vibration_785190(vib);
                    if(v)return {GameLogicOwnerState::completed,0,{}};
                    return {report_.stopped_at?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,v.source_boundary,{}};}
                if(n.callee==kNinjaReturnOnly)return {GameLogicOwnerState::completed,0,{}}; // 78c5a0: RET (registered by 793e70)
                if(n.callee==0x7912e0){ // EndScene's viewport refresh (a no-op unless the loader's reset_device set 3d0fd1c)
                    NinjaViewportRuntime vp{screen_,device_,render_,screen_device_,fp_};const auto v=run_viewport_refresh_7912e0(vp);
                    if(v.required_fact)failure_detail_=" (7912e0 requires "+hex(*v.required_fact)+")";
                    return v?GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}}:GameLogicOwnerResult{GameLogicOwnerState::failed,v.source_boundary,{}};}
                if(n.callee==0x77dfc0){ // BeginScene's polygon (the loader's uiscale calls an unhooked copy of the original)
                    if(!texture_runtime_||!view_||!n.arguments[0]||!n.arguments[1]||!n.arguments[2]||!n.arguments[3])return {GameLogicOwnerState::failed,0x78b922,{}};
                    report_screen_depth();
                    NinjaPolygonRuntime rt{[&](std::uint32_t a)->std::optional<std::uint32_t>{
                            if(a==0x3ce7164)return screen_.points_at_3ce7164;if(a==0x3ce7168)return screen_.colors_at_3ce7168;
                            if(a==0x3ce716c)return screen_.word_at_3ce716c;if(a==0x3ce7170)return screen_.count_at_3ce7170;
                            if(a>=0x3ce7174&&a<0x3ce7184)return view_->clear_colors_at_3ce7174[(a-0x3ce7174)/4];
                            if(a>=0x3ce7184&&a<0x3ce71a4)return screen_.points_at_3ce7184[(a-0x3ce7184)/4];
                            return std::nullopt;},
                        render_,texture_selection_,*texture_runtime_,frame_calls_.vertex_buffer_at_3d128ac,screen_device_,NinjaDepthRuntime{screen_,device_,sse_,fp_,sse_reciprocal_by_division}};
                    float argument;std::memcpy(&argument,&*n.arguments[2],4);
                    const auto p=run_screen_polygon_77dfc0(*n.arguments[0],*n.arguments[1],argument,*n.arguments[3],rt);
                    if(p.status.required_fact)failure_detail_=" (77dfc0 requires "+hex(*p.status.required_fact)+")";
                    return p?GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}}:GameLogicOwnerResult{GameLogicOwnerState::failed,p.status.source_boundary,{}};}
                if(n.callee==0x789bd0){ // the message pump hook 789c50 registered
                    if(!message_pump_reported_){message_pump_reported_=true;
                        report_.assumptions.push_back("789bd0: the host message queue is empty (no window events headless; the app front end supplies them), so 3d08534 takes 789bd0's uninitialised MSG.wParam: unknown");}
                    GameMessagePumpRuntime pump{window_,window_at_3d0fd30_,[](std::uint32_t)->std::optional<std::optional<GameMessage>>{return std::optional<GameMessage>{};},
                        [&](const GameMessage&){return unported(0x401900,0x789bd0,path+">789bd0(loader HandleWindowMessages_r)");},
                        GameMessagePumpProfile::mod_loader,[](const GameMessage&)->std::optional<bool>{return false;}}; // the loader replaces 789bd0
                    const auto p=run_message_pump_789bd0(pump);
                    return p?GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}}:GameLogicOwnerResult{GameLogicOwnerState::failed,p.source_boundary,{}};}
                return unported(n.callee,n.source_call,path);}};
        const auto r=run_ninja_frame_call(c.callee,rt);
        if(r)return done(GameLogicOwnerState::completed);
        if(r.required_fact)failure_detail_=" ("+hex(c.callee)+" requires "+hex(*r.required_fact)+")";
        if(r.failure==NinjaFrameFailure::missing_owner&&r.pending_call)return {GameLogicOwnerState::unavailable,r.source_boundary,{}};
        return {r.failure==NinjaFrameFailure::cancelled?GameLogicOwnerState::cancelled:GameLogicOwnerState::failed,r.source_boundary,{}};
    };
    InputModFrameOwners input_owners{pads_,controllers_raw_,input_raw_,controller_pointers_};
    GameLoopRuntime loop{loop_,logic_,{}};
    loop.invoke=[&](const GameLoopCall& c)->GameLoopOwnerResult{
        switch(c.operation){
        case GameLoopOperation::init_40c030:{const auto r=run_startup_40c030(startup);report_.startup_completed=bool(r);
            return {r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==StartupFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}},{}};}
        case GameLoopOperation::ret_51f680:return {done(GameLogicOwnerState::completed),{}};
        case GameLoopOperation::work_40d4d0:case GameLoopOperation::work_40eab0:case GameLoopOperation::work_42f990:case GameLoopOperation::work_40d3c0:
        case GameLoopOperation::work_420840:case GameLoopOperation::keys_40ef30:case GameLoopOperation::key_40ef20:{
            GameFrameOwnersRuntime owners{frame_owners_,*view_,byte_at_3b280a8_,leaves_->word_at_3b0e338,controller_globals_.words_at_3b0e990[2],input_.mask_at_3b0e3b8,edges_,render_,&depth_table_,
                sine_table_at_88af78_,projection_,consumer_.projection_width_at_3d0f9e4,color_at_3b18230_,
                NinjaQueueColorOwners{queue_runtime_.color_at_3ab9864,projection_.color_argb_at_3d0f7f0,one_at_7dcb10_,negative_one_at_7dcb0c_},fp_,
                [&](const GameFrameOwnerCall& n){return unported(n.callee,n.source_call,"40cad0>"+hex(c.callee));}};
            const auto r=run_game_frame_owner(c.callee,c.arguments[0],owners);
            if(r.failure==GameFrameOwnerFailure::not_ported){auto owner=unported(c.callee,c.source_call,"40cad0");return {owner,owner.state==GameLogicOwnerState::completed?std::optional<std::uint32_t>{0}:std::nullopt};}
            if(r.required_fact)failure_detail_=" ("+hex(c.callee)+" requires "+hex(*r.required_fact)+")";
            return {r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==GameFrameOwnerFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}},r.eax};}
        case GameLoopOperation::consume_4086f0:{
            if(!consumer_runtime_)return {GameLogicOwnerResult{GameLogicOwnerState::failed,0x4086f0,{}},{}};
            const auto r=ninja_consume_queue(*consumer_runtime_,fp_);
            if(r.required_fact)failure_detail_=" (4086f0 requires "+hex(*r.required_fact)+")";
            if(r.failure==NinjaQueueConsumerFailure::unlifted_callee)return {unported(r.callee_boundary,r.source_boundary,"40cad0>4086f0"),{}};
            return {status(bool(r),r.source_boundary),{}};}
        case GameLoopOperation::mode_4250d0:{
            SoundManagerRuntime sound{audio_,leaves_->sound_record_flags_at_3b29304,leaves_->words_at_3b29b90,sound_fields_,[&](const SoundManagerCall& n)->SoundManagerOwnerResult{
                if(n.callee==0x410100){const auto r=run_reset_leaf(0x410100,n.arguments[0],*leaves_,{});
                    return {r.outcome==ResetLeafOutcome::completed?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::failed,r.source_boundary,{}},r.eax};}
                auto owner=unported(n.callee,n.source_call,"40cad0>4250d0");return {owner,owner.state==GameLogicOwnerState::completed?std::optional<std::uint32_t>{0}:std::nullopt};}};
            const auto r=run_sound_manager_4250d0(sound);
            return {r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==SoundManagerFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}},{}};}
        case GameLoopOperation::message_77f090:{
            if(!keyboard_table_)return {GameLogicOwnerResult{GameLogicOwnerState::failed,0x791c65,{}},{}};
            const auto k=ninja_keyboard_poll_791bd0(keyboard_,*keyboard_table_);return {status(bool(k),k.source_boundary),{}};}
        case GameLoopOperation::set_401410:{const auto leaf=run_reset_leaf(0x401410,c.arguments[0],*leaves_,{});
            return {leaf.outcome==ResetLeafOutcome::completed?done(GameLogicOwnerState::completed):GameLogicOwnerResult{GameLogicOwnerState::failed,leaf.source_boundary,{}},{}};}
        case GameLoopOperation::work_420830:byte_at_3b280a8_=1;return {done(GameLogicOwnerState::completed),{}};
        case GameLoopOperation::queue_reset_407ab0:{const auto r=ninja_queue_reset_407ab0(*lifecycle_,0,fp_);return {status(bool(r),r.source_boundary),{}};}
        case GameLoopOperation::depth_77ed00:{const auto r=ninja_set_depth_comparison(*c.arguments[0],&depth_table_,render_);return {status(bool(r),r.source_boundary),{}};}
        case GameLoopOperation::input_40f460:{
            // Updated profile: 40f460 -> 40fdb3 -> loader OnInput (built-in SDL off) -> Input 2.6.1, then Speedrun's empty OnInput.
            const auto r=input_mod_on_input(input_owners,keyboard_pad,gamepads,input_frame_,config_.input_constants,sse_);
            if(!r&&r.failure==InputModFailure::demo_unported)return {unported(0x10006595,c.source_call,"40cad0>40f460(Input OnInput demo)"),{}};
            return {status(bool(r),r.source_boundary),{}};}
        case GameLoopOperation::work_40c090:{const auto r=run_game_mode_40c090(mode);
            return {r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==GameModeFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}},{}};}
        case GameLoopOperation::frame_411d00:{const auto r=run_frame_completion(frame);
            return {r?done(GameLogicOwnerState::completed):GameLogicOwnerResult{r.failure==FrameCompletionFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,r.source_boundary,{}},{}};}
        case GameLoopOperation::idle_781060:{const auto r=run_speedrun_frame_limiter(renderer_,timing_,config_.limiter_cadence,config_.clock);return {status(bool(r),r.source_boundary),{}};}
        default:{auto owner=unported(c.callee,c.source_call,"40cad0");return {owner,owner.state==GameLogicOwnerState::completed?std::optional<std::uint32_t>{0}:std::nullopt};}
        }
    };
    const auto describe=[&](const GameLoopResult& r){return "loop failure "+std::to_string(static_cast<int>(r.failure))+" at "+hex(r.source_boundary)+(r.pending_call?" calling "+hex(r.pending_call->callee):"")+" nested "+hex(r.nested_source_boundary)+failure_detail_;};
    // WinMain 4025b0's settings span (after the CRT static initializers, before 781070 -> 791610).
    const auto winmain_settings=[&]()->bool{
        const Win32Profile profile(config_.sonicdx_ini.value_or(std::vector<std::uint8_t>{}));
        if(!config_.sonicdx_ini)report_.assumptions.push_back("WinMain: no sonicDX.ini supplied; every GetPrivateProfile read returns its fallback (as Win32 does for an absent file)");
        for(const auto& u:profile.unsupported)report_.assumptions.push_back("sonicDX.ini "+u+": not interpreted identically to Win32");
        report_.assumptions.push_back("WinMain GetPrivateProfileIntA/StringA: Win32 semantics re-implemented natively (case-insensitive lookup, trimmed values, sign + decimal digits), not traced");
        std::vector<std::string> skipped;
        GameWinMainRuntime rt{winmain_,{renderer_.render_factor_at_89295c,consumer_.special_flags_at_909eb4,leaves_->config_at_38f6ec0,leaves_->word_at_3abdfac,leaves_->config_at_3a7cf3c,leaves_->word_at_3abdf90},config_.loader_settings,
            [&](const WinMainProfileRead& r)->std::optional<std::int32_t>{return profile.get_int(r.section,r.key,r.fallback);},
            [&](const WinMainProfileRead& r)->std::optional<std::string>{return profile.get_string(r.section,r.key,r.string_fallback,r.size);},
            [&](const WinMainCall& c)->WinMainOwnerResult{
                const auto ok=[](std::optional<std::uint32_t> eax={}){return WinMainOwnerResult{{GameLogicOwnerState::completed,0,{}},eax};};
                switch(c.callee){
                case 0x7db040:return ok(0x7f000000u);          // LoadLibraryA("imm32.dll"): a host module token
                case 0x7db044:return ok(0x7f001000u);          // GetProcAddress(ImmDisableIME): a host procedure token
                case 0x7f001000:return ok();                   // ImmDisableIME(0)
                case 0x7db1e0:report_.assumptions.push_back("WinMain 40269a: MessageBoxA(\"media player can't found\") reached because 3ab918c/3ab9190 keep load-image zeros (the CRT static constructor that sets them is unported); no message box is shown natively");return ok(1);
                case 0x7db1dc:--cursor_display_count_;return ok(static_cast<std::uint32_t>(cursor_display_count_)); // ShowCursor(FALSE)
                case 0x401a20:{ // CD-drive scan over the host's drives
                    CdDriveHost host{[](std::uint32_t size)->std::optional<std::pair<std::uint32_t,std::vector<std::uint8_t>>>{
                            const std::vector<std::uint8_t> bytes{'C',':','\\',0,0};
                            if(size<bytes.size())return std::pair<std::uint32_t,std::vector<std::uint8_t>>{static_cast<std::uint32_t>(bytes.size()),{}};
                            return std::pair<std::uint32_t,std::vector<std::uint8_t>>{static_cast<std::uint32_t>(bytes.size()-1),bytes};},
                        [](const std::string&)->std::optional<std::uint32_t>{return 3;},  // DRIVE_FIXED
                        [](const std::string&)->std::optional<bool>{return false;}};
                    report_.assumptions.push_back("401a20: host drives are one fixed C:\\ with no CD-ROM holding the disc, so 3a8af78 stays empty (the reference machine's drives were not captured; the digital install needs no disc)");
                    report_.assumptions.push_back("780f60 njMalloc: HeapCreate/HeapAlloc results are native identities (handle 0x510000, blocks from 0x50000000); the reference's heap addresses were not captured");
                    const auto r=run_cd_drive_scan_401a20(cd_drive_,memory_,heap_,host);
                    if(!r)return {{GameLogicOwnerState::failed,r.source_boundary,{}},std::nullopt};
                    return ok();}
                default:skipped.push_back(hex(c.callee)+" (site "+hex(c.source_call)+")");return ok();}}};
        const auto r=run_winmain_settings(rt);
        report_.assumptions.push_back("WinMain imm32.dll/ImmDisableIME: host tokens; macOS has no IME counterpart and the call changes no game state");
        if(!skipped.empty()){std::string list;for(const auto& sk:skipped)list+=(list.empty()?"":", ")+sk;
            report_.assumptions.push_back("WinMain settings span: unported pre-40cad0 owners skipped, their globals stay load-image values: "+list);}
        if(!r){report_.failure="WinMain settings failed at "+hex(r.source_boundary)+(r.required_fact?" (requires "+hex(*r.required_fact)+")":"");return false;}
        return true;};
    // Pre-40cad0 device setup 791610: only its fog initializer 794ec0 runs natively.
    report_.assumptions.push_back("791610 GetDeviceCaps: the native Metal device declares D3DPRASTERCAPS_WFOG (table fog emulated by the backend) and no other caps; the Windows reference GPU's D3DCAPS8 were not captured");
    report_.assumptions.push_back("7958d0/793f70: registry value CTRLFLAG0 absent (3d08ae0 = 0), as 7958d0 zeroes it before the registry read; the reference machine's registry was not captured");
    // CRT static init 7c4e90 -> 401ac0 (before WinMain): model DLL bindings.
    if(std::any_of(config_.model_dlls.begin(),config_.model_dlls.end(),[](const auto& p){return bool(p);})){
        // Profile: 401ac0's LoadLibraryA("SYSTEM\\chrmodels.dll") loads the mod loader (the CHRMODELS.dll proxy), whose
        // DllMain patches 401ae1..401ae6 (nop; call LoadChrmodels) during that same call. LoadChrmodels loads
        // CHRMODELS_orig.dll and stores its handle in 3ab9170 (DataPointer chrmodelshandle) after 401adf's
        // `mov [esi],eax`, so slot 0's handle and GetProcAddress source are CHRMODELS_orig.dll.
        std::string error;std::array<std::uint32_t,7> handles{};
        model_set_=map_model_dlls(config_.model_dlls,0x20000000,handles,&error);
        if(!error.empty()){report_.failure="model DLL mapping: "+error;return report_;}
        for(std::size_t i=0;i<7;++i)if(model_set_.images[i])model_set_.handles[i]=model_set_.images[i]->image_base();
        report_.assumptions.push_back("401ac0: slot 0 follows the pinned mod loader's (eb4cdcca, DLL 3293cf35) 401ae1/401ae2 patch from source: LoadChrmodels stores CHRMODELS_orig.dll's handle in 3ab9170 and its exports are bound; the loader's machine code was not executed against this claim");
        report_.assumptions.push_back("401ac0: model DLLs all prefer 0x10000000; the native loader maps them at ascending 64 KiB-aligned bases from 0x20000000 with relocations applied (the reference's load addresses were not captured)");
        const auto b=bind_model_dlls_401ac0(model_set_,model_bindings_);if(!b){report_.failure="401ac0 forwarded export "+b.export_name;return report_;}
        mode_leaves_->model_bindings=&model_bindings_;mode_leaves_->model_dlls=&model_set_;
    }
    // WinMain: settings, then 781070 -> 793d80 -> 791610 (device setup) before 401830 (system init).
    if(!winmain_settings())return report_;
    // 401830's owners (also used for 781070's own 77f150 call), with the pinned Input mod's 77f0d7/77f03e/77f205 patches.
    std::vector<std::string> system_skipped;
    NinjaSystemInitRuntime sys{system_,{memory_,heap_,crt_heap_,frame_calls_,device_.block_at_3d0fae0,library_.words_at_389d864,texture_pool_,texture_selection_,callbacks_,queue_runtime_.captured_word_at_3d0f9c8,leaves_->entry_words_at_3d08428,
            &view_->clear_colors_at_3ce7174,window_at_3d0fd30_,instance_at_3d0fd34_},fp_,
        [&](const NinjaSystemCall& c)->NinjaSystemOwnerResult{
            const auto ok=[](std::optional<std::uint32_t> eax={}){return NinjaSystemOwnerResult{{GameLogicOwnerState::completed,0,{}},eax};};
            switch(c.callee){
            case 0x780180:{ // njInitMatrix(3a7cf48, 0x80, 0): slot 0 = the 389d650 unit with m[0] and m[5] negated, depth 1
                if(!unit_at_389d650_||!sine_table_at_88af78_)return {{GameLogicOwnerState::failed,0x780180,{}},std::nullopt};
                std::vector<NinjaMatrix> slots(0x80);for(unsigned k=0;k<0x80;++k)for(unsigned i=0;i<16;++i){const auto v=config_.executable->u32(0x3a7cf48+0x40*k+4*i);std::uint32_t b=v.value_or(0);std::memcpy(&slots[k][i],&b,4);}
                slots[0]=*unit_at_389d650_;slots[0][0]=fp_.to_float(fp_.from_float(slots[0][0]).negated());slots[0][5]=fp_.to_float(fp_.from_float(slots[0][5]).negated());
                matrix_stack_.emplace(std::move(slots),1,0U);coordinates_.emplace(*sine_table_at_88af78_,*unit_at_389d650_,*matrix_stack_);projection_.coordinates=&*coordinates_;
                return ok();}
            case 0x77f0a0:{ // keyboard init: the Input mod's CreateSADXKeyboard replaces 791b10
                const auto k=ninja_keyboard_init_77f0a0(keyboard_,callbacks_,{},NinjaKeyboardProfile::input_mod);
                return k?ok():NinjaSystemOwnerResult{{GameLogicOwnerState::failed,k.source_boundary,{}},std::nullopt};}
            case 0x77dbb0:return ok(0x0d1a0000u); // DirectInput8Create: a host IDirectInput8 token written to 3abdc10
            case 0x40bb4f:return ok(0x0d1a1000u); // CreateDevice(GUID_SysMouse): a host device token written to 3abdc14
            case 0x40bb5f:case 0x40bb71:case 0x40bb7c:return ok(0u); // SetDataFormat, SetCooperativeLevel, Acquire
            default:system_skipped.push_back(hex(c.callee)+" (site "+hex(c.source_call)+")");return ok();}},
        [&]()->std::optional<std::int64_t>{return config_.clock.counter?config_.clock.counter():std::nullopt;}};
    // WinMain 402c45: 781070(892940) "Sonic Library Initialize", which reaches 793d80 -> 791610 (device setup).
    {
        std::vector<std::string> skipped;
        const auto skip=[&](std::uint32_t callee,std::uint32_t site){skipped.push_back(hex(callee)+" (site "+hex(site)+")");return GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}};};
        NinjaDeviceHost host{
            [](std::uint32_t)->std::optional<std::uint32_t>{return 0x0d3d0000u;},  // Direct3DCreate8: a host IDirect3D8 token
            [](const D3D8FrameCommand&)->std::optional<std::vector<std::uint8_t>>{
                std::vector<std::uint8_t> caps(0xd4,0);caps[0x24+2]=0x10; // RasterCaps (3d127e4) = D3DPRASTERCAPS_WFOG; every other cap 0
                return caps;},
            [](const D3D8FrameCommand&)->std::optional<std::array<std::uint32_t,6>>{return std::array<std::uint32_t,6>{0,0,640,480,0,0x3f800000};},
            [&](const D3D8FrameCommand&,std::uint32_t flags,std::uint32_t,float,std::uint32_t)->std::optional<std::int32_t>{++d3d8_.clears;d3d8_.last_clear_flags=flags;return 0;}};
        NinjaDeviceInitRuntime device_rt{device_,frame_calls_,render_,renderer_,fp_,host,[&](const NinjaDeviceCall& c)->GameLogicOwnerResult{
            switch(c.callee){
            case 0x7943c0:{ // device creation (the loader's CreateDirect3DDevice_r behind 794000)
                auto device=std::make_shared<NinjaRenderDevice>();device->identity=1;render_.device_at_3d128b0=device;
                report_.assumptions.push_back("791610 -> 7943c0: device creation (the loader's CreateDirect3DDevice_r at 794000) is not ported; a native device with the loader's 640x480 windowed back buffer is created, and its caps declare only D3DPRASTERCAPS_WFOG");
                return {GameLogicOwnerState::completed,0,{}};}
            case 0x794ec0:{ // fog init reads the caps GetDeviceCaps just wrote
                std::uint32_t raster=0;for(unsigned k=0;k<4;++k)raster|=static_cast<std::uint32_t>(device_.caps_at_3d127c0.bytes[0x24+k].value_or(0))<<(8*k);
                NinjaFogDeviceOwners fog{consumer_,fog_device_,&render_};const auto f=ninja_fog_initialize_794ec0(fog,{raster,0U},fp_);
                return f?GameLogicOwnerResult{GameLogicOwnerState::completed,0,{}}:GameLogicOwnerResult{GameLogicOwnerState::failed,f.source_boundary,{}};}
            default:return skip(c.callee,c.source_call);}}};
        GameLibraryInitRuntime lib{library_,0x892940,
            [&](std::uint32_t a)->std::optional<std::uint32_t>{
                if(a==0x892950)return winmain_.screen_at_892950;
                if(a==0x89295c){if(!renderer_.render_factor_at_89295c)return std::nullopt;return static_cast<std::uint32_t>(*renderer_.render_factor_at_89295c);}
                return config_.executable?config_.executable->u32(a):std::nullopt;},  // config words without another owner: load image
            [&](std::uint32_t a)->std::optional<std::uint32_t>{return a==0x3d12788?device_.viewport_at_3d12780[2]:a==0x3d1278c?device_.viewport_at_3d12780[3]:std::nullopt;},
            fp_,[&](const GameLibraryCall& c)->GameLibraryOwnerResult{
                switch(c.callee){
                case 0x77ff40:word_at_3d0f7a0_=c.arguments[0];return {{GameLogicOwnerState::completed,0,{}},std::nullopt};
                case 0x793d80:{ // 791610, then 7800a0
                    const auto d=run_device_setup_791610(device_rt);
                    if(!d)return {{d.failure==NinjaDeviceFailure::missing_owner?GameLogicOwnerState::unavailable:GameLogicOwnerState::failed,d.source_boundary,{}},std::nullopt};
                    skip(0x7800a0,0x793d85);return {{GameLogicOwnerState::completed,0,{}},std::nullopt};}
                case 0x788000:skip(c.callee,c.source_call);return {{GameLogicOwnerState::completed,0,{}},0u}; // assumed to succeed on the first try
                case 0x793e70:{ // 77ffc0(78c5a0): a frame callback that only returns
                    const auto d=run_debug_text_register_793e70(callbacks_);
                    return {{d?GameLogicOwnerState::completed:GameLogicOwnerState::failed,d.source_boundary,{}},0u};}
                case 0x780c30:{ // screen setup (config +0x14 flags, +0x18, +0x1c)
                    if(!c.arguments[0]||!c.arguments[1]||!c.arguments[2]||!view_)return {{GameLogicOwnerState::failed,0x78113f,{}},std::nullopt};
                    NinjaScreenInitRuntime rt{NinjaViewportRuntime{screen_,device_,render_,screen_device_,fp_},renderer_,consumer_.projection_width_at_3d0f9e4,projection_,callbacks_,
                        queue_runtime_.captured_word_at_3d0f840,queue_runtime_.captured_word_at_3d0f9c4,system_.pool_at_3ce71b0,system_.size_at_3ce71a4,view_->clear_colors_at_3ce7174};
                    const auto sr=run_screen_init_780c30(*c.arguments[0],*c.arguments[1],*c.arguments[2],rt);
                    return {{sr?GameLogicOwnerState::completed:GameLogicOwnerState::failed,sr.source_boundary,{}},std::nullopt};}
                case 0x789c50:{ // 7959c0(789bd0), then SetForegroundWindow([3d0fd30]) (a host no-op)
                    const auto h=ninja_hook_register_7959c0(frame_calls_,0x789bd0);
                    if(!h)return {{GameLogicOwnerState::failed,h.source_boundary,{}},std::nullopt};
                    if(!window_at_3d0fd30_)return {{GameLogicOwnerState::failed,0x789c5a,{}},std::nullopt};
                    return {{GameLogicOwnerState::completed,0,{}},std::nullopt};}
                case 0x77f150:{const auto r=run_controllers_77f150(sys); // the same owner 401830 calls later
                    return {{r?GameLogicOwnerState::completed:GameLogicOwnerState::failed,r.source_boundary,{}},std::nullopt};}
                default:skip(c.callee,c.source_call);return {{GameLogicOwnerState::completed,0,{}},std::nullopt};}}};
        const auto r=run_library_init_781070(lib);
        if(!skipped.empty()){std::string list;for(const auto& sk:skipped)list+=(list.empty()?"":", ")+sk;
            report_.assumptions.push_back("781070/791610: unported pre-40cad0 owners skipped, their globals stay load-image values (788000 is taken to succeed on its first try): "+list);}
        if(!r){report_.failure="781070 library init failed at "+hex(r.source_boundary)+(r.required_fact?" (requires "+hex(*r.required_fact)+")":"");return report_;}
    }
    {
        for(unsigned i=0;i<27;++i)system_.tables_at_389d888[i]=config_.executable?config_.executable->u32(0x389d888+4*i):std::nullopt;
        const auto r=run_system_init_401830(sys);
        report_.assumptions.push_back("401830: the matrix stack is as njInitMatrix leaves it; the Input mod's CreateSADXKeyboard replaces 77f0a0's 791b10 (no DirectInput keyboard device: the game's 791bd0 poll returns early and the Input mod's keyboard drives input), and its NOPs remove 77efd0's 791d60 and 77f150's 7928e0");
        report_.assumptions.push_back("401830 40bb20: the DirectInput8 interface and mouse device are host tokens; 3d0fd30/3d0fd34 (window and instance) keep load-image values because window creation (793ef0) is unported; 78e720's CRT malloc returns native identities (handle 0x520000, blocks from 0x60000000)");
        if(!system_skipped.empty()){std::map<std::string,int> counts;std::vector<std::string> order;for(const auto& sk:system_skipped){if(!counts[sk]++)order.push_back(sk);}
            std::string list;for(const auto& sk:order)list+=(list.empty()?"":", ")+sk+(counts[sk]>1?" x"+std::to_string(counts[sk]):"");
            report_.assumptions.push_back("401830 and 781070's 77f150: unported owners skipped, their globals stay load-image values: "+list);}
        if(!r){report_.failure="401830 system init failed at "+hex(r.source_boundary)+(r.required_fact?" (requires "+hex(*r.required_fact)+")":"");return report_;}
    }
    report_.assumptions.push_back("WinMain 402c92..402d7c is not run natively: the ShowCursor loop, the cliplevel inits 5850f0/556e40/556d60/568d20/4f68e0/5adc40/600700, the model DLLs' SetClip_EC00/EC01/EC31 exports, 781570 (which reaches 791170, the 3d129c0 projection writer), 781250, 40e2e0 and the debug prints; their globals keep load-image values");
    auto r=run_game_loop_startup(loop);
    if(!r){if(!report_.stopped_at)report_.failure=describe(r);return report_;}
    while(r.next==GameLoopNext::iteration&&report_.frames_completed<config_.max_frames){
        r=run_game_loop_iteration(loop);
        if(!r){if(!report_.stopped_at)report_.failure=describe(r);return report_;}
        ++report_.frames_completed;
    }
    if(r.next==GameLoopNext::shutdown){r=run_game_loop_shutdown(loop);if(!r&&!report_.stopped_at)report_.failure=describe(r);report_.exited=bool(r);}
    return report_;
}
} // namespace sadx
