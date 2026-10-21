#include "game/title_menu.hpp"
#include "engine/frame_loop_native.hpp"
#include "cpu/x87_math.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
using namespace sadx;
namespace tmn = sadx::title_menu;
namespace {
TaskHandle handle(std::uint32_t id) {
    if (!id) return {};
    auto owner = std::make_shared<std::uint32_t>(id);
    return TaskHandle(owner, reinterpret_cast<Task *>(owner.get()));
}
std::uint32_t identity(const TaskHandle &task) {
    return task ? *reinterpret_cast<const std::uint32_t *>(task.get()) : 0;
}
struct Stop {};
struct Probe : tmn::Calls, tmn::SoundCalls, tmn::MenuTextureCalls, tmn::SoundManagerCalls, tmn::DialogCalls, tmn::SequenceCalls, tmn::TitleCalls, tmn::ObjectCalls, tmn::TitleInitCalls, tmn::TitleUpdateCalls, tmn::TitleDrawCalls, tmn::RunCalls {
    const std::uint32_t *in;
    X87Context fp{127};
    std::vector<std::uint32_t> trace;
    std::array<tmn::SequenceData, 3> data;
    tmn::State state;
    std::shared_ptr<tmn::SequenceChildWork> child;
    std::uint32_t sequence_exec = 0, sequence_destroy = 0, child_exec = 0;
    std::uint32_t dialog_exec = 0, dialog_display = 0;
    std::uint32_t title_exec = 0, title_display = 0;
    std::uint32_t count = 0;
    explicit Probe(const std::uint32_t *input) : in(input), data{{{static_cast<tmn::MenuMode>(input[15])}, {static_cast<tmn::MenuMode>(input[15]+1)}, {static_cast<tmn::MenuMode>(input[15]+2)}}} {}
    void call(std::uint32_t id, std::uint32_t a = 0, std::uint32_t b = 0,
              std::uint32_t c = 0, std::uint32_t d = 0) {
        trace.insert(trace.end(), {id, a, b, c, d});
        if (++count == in[16]) throw Stop{};
    }
    static std::uint32_t bits(float f) { std::uint32_t v; std::memcpy(&v, &f, 4); return v; }
    void destroy_sound_manager(const TaskHandle &task) override { call(34,identity(task)); }
    void close_media(std::uint32_t mode) override { call(35,mode); }
    TaskHandle create_dialog_task() override {
        call(36,0,5); dialog_exec = in[4]&4 ? 0 : 1;
        return handle(in[4]&4 ? 0 : 3);
    }
    std::shared_ptr<tmn::DialogWork> allocate_dialog_work(const TaskHandle &) override {
        call(37,68); return std::make_shared<tmn::DialogWork>();
    }
    void install_dialog_callbacks(const TaskHandle &) override { dialog_exec = 1; dialog_display = 1; }
    void set_entrance(std::int32_t value) override { call(38,value); }
    void release_menu_textures() override { call(39); }
    void finalize_selection() override { call(40); }
    void create_menu_objects(const tmn::SequenceData *data_value) override { call(41,data_value?1:0); }
    void destroy_sequence_child(const TaskHandle &task) override { call(42,identity(task)); }
    TaskHandle create_title_task() override {
        call(43,8,3); title_exec=in[4]&1?0:1; return handle(in[4]&1?0:4);
    }
    std::shared_ptr<tmn::TitleWork> allocate_title_work(const TaskHandle &) override {
        call(44,1,60); return std::make_shared<tmn::TitleWork>();
    }
    void install_title_callbacks(const TaskHandle &) override { title_exec=1;title_display=1; }
    void calculate_ticks() override { call(46); }
    TaskHandle object(std::uint32_t mode,const tmn::ModeSelection &selection) {
        call(45,mode,static_cast<std::uint32_t>(selection.previous),
             static_cast<std::uint32_t>(selection.next),static_cast<std::uint32_t>(selection.next_sub));
        return handle(mode);
    }
    TaskHandle create_character_select(const tmn::ModeSelection &s) override { return object(1,s); }
    TaskHandle create_file_select(const tmn::ModeSelection &s) override { return object(2,s); }
    TaskHandle create_level_select(const tmn::ModeSelection &s) override { return object(3,s); }
    TaskHandle create_sound_test(const tmn::ModeSelection &s) override { return object(4,s); }
    TaskHandle create_title(const tmn::ModeSelection &s) override { return object(5,s); }
    TaskHandle create_title_menu(const tmn::ModeSelection &s) override { return object(6,s); }
    TaskHandle create_options(const tmn::ModeSelection &s) override { return object(7,s); }
    TaskHandle create_explanation(const tmn::ModeSelection &s) override { return object(8,s); }
    TaskHandle create_subgame_list(const tmn::ModeSelection &s) override { return object(9,s); }
    TaskHandle create_emblem_results(const tmn::ModeSelection &s) override { return object(10,s); }
    TaskHandle create_internet_demo(const tmn::ModeSelection &s) override { return object(11,s); }
    TaskHandle create_tv_settings(const tmn::ModeSelection &s) override { return object(12,s); }
    TaskHandle create_puti(const tmn::ModeSelection &s) override { return object(13,s); }
    void set_stage(std::uint8_t a,std::uint8_t b) override { call(47,a,b); }
    void initialize_character_lighting() override { call(48); }
    void load_mode_textures(tmn::MenuMode mode) override { call(49,static_cast<std::uint32_t>(mode)); }
    void load_japanese_title_textures() override { call(50); }
    void prepare_menu_voice() override { call(51); }
    void set_title_menu_previous() override { call(52); }
    void initialize_title(tmn::TitleWork &) override { call(53); }
    std::uint32_t fade_out_color(float t) override { call(54,bits(t)); return in[15]; }
    std::uint32_t fade_in_color(float t) override { call(55,bits(t)); return in[15]; }
    void update_title_input(tmn::TitleWork &) override { call(56); }
    void release_mode_textures(tmn::MenuMode mode) override { call(57,static_cast<std::uint32_t>(mode)); }
    void release_japanese_title_textures() override { call(58); }
    void default_blending() override { call(59); }
    void set_base_color(std::uint32_t value) override { call(60,value); }
    void draw_title_background(tmn::TitleWork &) override { call(61); }
    void set_depth_compare(std::int32_t v) override { call(62,v); }
    void poll_controls() override { call(63); }
    void enable_control() override { call(64); }
    void run_tasks() override { call(65); }
    void disable_fog() override { call(66); }
    void display_tasks() override { call(67); }
    std::int32_t soft_reset_check(std::int32_t v) override { call(68,v); return in[2]; }
    void delete_all_tasks() override { call(69); }
    void free_regular_object_textures() override { call(70); }
    void destroy_sequence_task(const TaskHandle &task) override { call(71,identity(task)); }
    void stop_music() override { call(1); }
    void initialize_menu_sound() override { call(2); }
    void set_frame_rate(std::int32_t a, std::int32_t b) override { call(3,a,b); }
    void set_wait_vsync(std::int32_t a) override { call(4,a); }
    void initialize_heap() override { call(5); }
    void initialize_tasks() override { call(6); }
    void set_background(std::uint32_t a,std::uint32_t b,std::uint32_t c) override { call(7,a,b,c); }
    void set_fade_color(std::uint32_t a) override { call(8,a); }
    void release_loading_font() override { call(9); }
    void free_character_textures() override { call(10); }
    void release_all_textures() override { call(11); }
    void initialize_texture_drawing() override { call(12); }
    void initialize_render_state() override { call(13); }
    void set_lighting(bool a) override { call(14,137,a); }
    void set_view_angle(std::int32_t a) override { call(15,a); }
    void set_clip(float a,float b) override { call(16,bits(a),bits(b)); }
    void set_camera() override { call(17); }
    void load_regular_object_textures(std::int32_t a) override { call(18,a); }
    void load_player_motion(std::int32_t a) override { call(19,a); }
    void load_loading_font() override { call(20); }
    void copy_loading_font() override { call(21); }
    void release_font_resource() override { call(32); }
    void select_sound_bank(std::int32_t bank) override { call(31,bank); }
    void load_menu_texture(tmn::MenuTextureId id) override { call(33,id.value); }
    void set_font_color(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d) override { call(22,a,b,c,d); }
    void draw_text(std::string_view text,float a,float b,float c) override {
        if (text != "Now Loading...") throw std::runtime_error("wrong text");
        call(23,bits(a),bits(b),bits(c));
    }
    TaskHandle create_sequence_task() override { call(24,0,3); sequence_exec = in[4]&1 ? 0 : 1; return handle(in[4]&1 ? 0 : 1); }
    std::shared_ptr<tmn::SequenceWork> allocate_sequence_work(const TaskHandle &) override {
        call(25,1,36); return std::make_shared<tmn::SequenceWork>();
    }
    void initialize_menu_data() override { call(26); }
    void reset_selected_trial_level() override { call(27); }
    const tmn::SequenceData &sequence_data(std::int32_t entry) override {
        if (entry < 0 || entry > 2) throw std::runtime_error("wrong sequence");
        return data[entry];
    }
    TaskHandle create_sequence_child() override { call(28,0,3); child_exec = in[4]&2 ? 0 : 1; return handle(in[4]&2 ? 0 : 2); }
    std::shared_ptr<tmn::SequenceChildWork> allocate_child_work(const TaskHandle &) override {
        call(29,1,8); child = std::make_shared<tmn::SequenceChildWork>(); return child;
    }
    void install_child_callback(const TaskHandle &) override { child_exec = 1; }
    TaskHandle initialize_dialog() override { call(30); return handle(in[4]&4 ? 0 : 3); }
    void install_sequence_callbacks(const TaskHandle &) override { sequence_exec = 1; sequence_destroy = 1; }
    void sequence_already_active() override { throw Stop{}; }
};
}
extern "C" void sadx_native_title_menu(const std::uint32_t *in,std::uint32_t *out) {
    Probe p(in);
    p.state.trial={static_cast<std::uint8_t>(in[8]),static_cast<std::uint8_t>(in[9])};
    std::array<ActionHandle,9> actions{};
    tmn::ActionBindings bindings{actions[0],actions[1],actions[2],actions[3],actions[4],
                                 actions[5],actions[6],actions[7],actions[8]};
    tmn::CharacterActions imported{{in[8]},{in[9]},{in[10]},{in[11]},{in[12]},{in[13]},{in[14]}};
    std::uint8_t font = in[3], voice_state = in[6];
    std::uint8_t voice = in[5];
    std::uint32_t loops = in[7];
    tmn::SharedState shared{font,voice,voice_state,loops};
    tmn::MenuSoundState sound{handle(in[8]),handle(in[9])};
    tmn::MenuTextureState textures{{in[10],in[11]},static_cast<std::int32_t>(in[12])};
    const std::array<tmn::MenuTextureId,3> ids{{{in[13]%30},{in[14]%30},{in[15]%30}}};
    auto manager = handle(in[8]);
    cleanup::TrackState tracks{static_cast<std::int32_t>(in[9]), static_cast<std::int32_t>(in[10]),
                              static_cast<std::int32_t>(in[11]), static_cast<std::int32_t>(in[12]),
                              in[13], in[14]};
    tmn::DialogWork dialog;
    dialog.parameters = reinterpret_cast<const tmn::DialogParameters *>(static_cast<std::uintptr_t>(in[8]));
    dialog.phase=in[9];dialog.color=in[10];
    std::memcpy(&dialog.scale_x,in+11,4);std::memcpy(&dialog.scale_y,in+12,4);
    std::memcpy(&dialog.fade,in+13,4);
    dialog.repeat_frames=in[14];dialog.opening_frames=in[14]>>16;
    std::memcpy(dialog.choice_ids.data(),in+8,16);
    dialog.previous_choice=in[15];dialog.blink_step=in[7];
    auto canonical_dialog=dialog;
    tmn::SequenceWork sequence;
    sequence.data=&p.data[0];sequence.mode=static_cast<tmn::MenuMode>(in[8]);
    sequence.old_mode=static_cast<tmn::MenuMode>(in[9]);sequence.fixed_mode=static_cast<tmn::MenuMode>(in[10]);
    sequence.advance=static_cast<tmn::MenuResult>(in[11]);sequence.return_value=in[12];
    sequence.waiting_mode=static_cast<tmn::MenuMode>(in[13]);sequence.frames=in[14];
    sequence.waiting=in[14]>>16;sequence.fade_end=in[14]>>24;
    sequence.fade_out=in[15];sequence.fade_out_secondary=in[15]>>8;
    auto canonical_sequence=sequence;
    canonical_sequence.mode=static_cast<tmn::MenuMode>(in[9]);
    canonical_sequence.old_mode=static_cast<tmn::MenuMode>(in[10]);
    canonical_sequence.return_value=in[11];
    std::int32_t demo_mode=in[7];
    tmn::SequenceChildWork updating_child{&p.data[0],static_cast<std::int32_t>(in[15])};
    frame::ModeState tick_mode{static_cast<std::int32_t>(in[9]),static_cast<std::int32_t>(in[10]),
                              static_cast<std::int32_t>(in[11])};
    const std::int32_t elapsed=in[8],starting=in[12],playing_demo=in[13];
    frame_loop::TickState tick_state{tick_mode,loops,elapsed,starting,playing_demo};
    frame_loop::TickFailure tick_failure=frame_loop::TickFailure::None;
    tmn::SequenceData object_data;
    object_data.born_mode=static_cast<tmn::MenuMode>(in[8]%14);
    object_data.initial_transition={static_cast<tmn::MenuMode>(in[9]),static_cast<tmn::MenuMode>(in[10]),
                                    static_cast<tmn::MenuMode>(in[11])};
    for(std::uint32_t i=0;i<in[1]%4;++i) {
        object_data.following.push_back({static_cast<tmn::MenuMode>(in[12+i]%13+1),
            {static_cast<tmn::MenuMode>(in[8+i]),static_cast<tmn::MenuMode>(in[9+i]),
             static_cast<tmn::MenuMode>(in[10+i])}});
    }
    auto direct_title=std::make_shared<tmn::TitleWork>();
    direct_title->phase=static_cast<tmn::TitlePhase>(in[8]);
    direct_title->previous=static_cast<tmn::MenuMode>(in[9]);direct_title->next=static_cast<tmn::MenuMode>(in[10]);
    std::memcpy(&direct_title->depth,in+11,4);std::memcpy(&direct_title->fade_progress,in+12,4);
    direct_title->color=in[13];direct_title->submode=static_cast<tmn::TitleSubmode>(in[14]);
    direct_title->alert_task=handle(in[15]);direct_title->vm_status=in[8];direct_title->timer=in[9];
    direct_title->blink_timer=in[10];direct_title->music_started=in[11] ? 1U : 0U;
    direct_title->wave_timer=in[12];std::memcpy(&direct_title->cloud_offset,in+13,4);direct_title->cloud_index=in[14];
    auto canonical_title=std::make_shared<tmn::TitleWork>(*direct_title);
    canonical_title->next=static_cast<tmn::MenuMode>(in[5]);
    const std::int32_t title_language=in[1];
    const std::uint8_t title_character=in[2],skip_title=in[3];
    std::uint32_t adventure_loop_count=in[10];
    std::uint32_t pause_mode=in[7];
    std::int32_t run_result=0;
    bool completed = false;
    try {
        switch (in[0]) {
        case 0: tmn::prepare_title_assets(bindings,imported); break;
        case 1: tmn::prepare_title_dialogs(shared,p); break;
        case 2: tmn::reset_title_resources(p); break;
        case 3: tmn::create_sequence(p.state,in[1],p); break;
        case 4: tmn::initialize_title_menu(p.state,shared,
                   {static_cast<std::uint8_t>(in[1]),static_cast<std::uint8_t>(in[2])},p); break;
        case 5: tmn::initialize_menu_sound(sound,p); break;
        case 6: tmn::release_loading_font(shared,p); break;
        case 7: tmn::load_menu_textures(textures, std::vector<tmn::MenuTextureId>(ids.begin(), ids.begin() + in[1]%4), p); break;
        case 8: tmn::initialize_menu_data(textures, {}, p); break;
        case 9: tmn::close_sound_manager(manager,p); break;
        case 10: tmn::delete_sound_manager(manager,tracks,p); break;
        case 11:
            p.state.dialog_task=handle(3);
            tmn::initialize_dialog_work(dialog,in[4]&8?canonical_dialog:dialog,loops); break;
        case 12:
            p.state.dialog_task=handle(3);
            tmn::reset_dialog_work(dialog,in[4]&8?canonical_dialog:dialog); break;
        case 13: tmn::create_dialog(p.state,loops,p); break;
        case 14:
            p.state.sequence_task=handle(1);
            tmn::update_sequence(sequence,in[4]&8?canonical_sequence:sequence,demo_mode,p); break;
        case 15: tmn::update_sequence_child(handle(1),updating_child,p); break;
        case 16:
            tmn::create_title(p.state,{static_cast<tmn::MenuMode>(in[8]),static_cast<tmn::MenuMode>(in[9]),
                                     static_cast<tmn::MenuMode>(in[10])},p); break;
        case 17: tmn::create_menu_objects(object_data,tick_mode.pending_logic_ticks,p); break;
        case 18:
            tick_failure=frame_loop::calculate_ticks(tick_state);
            if(tick_failure!=frame_loop::TickFailure::None) throw Stop{};
            break;
        case 19: {
            p.state.title_task=handle(4);p.state.title_work=in[4]&8?canonical_title:direct_title;
            tmn::TitleInitState shared_title{title_language,title_character,skip_title,*p.state.title_work};
            tmn::initialize_title(*direct_title,shared_title,p); break;
        }
        case 20: {
            p.state.title_task=handle(4);p.state.title_work=direct_title;
            p.state.sequence_task=handle(1);p.state.sequence_work=std::make_shared<tmn::SequenceWork>();
            p.state.sequence_work->data=&p.data[0];p.state.sequence_work->mode=static_cast<tmn::MenuMode>(in[5]);
            p.state.sequence_work->old_mode=static_cast<tmn::MenuMode>(in[6]);
            p.state.sequence_work->fixed_mode=static_cast<tmn::MenuMode>(in[7]);
            const std::int32_t title_elapsed=in[9];
            tmn::TitleTiming timing{title_elapsed,loops,adventure_loop_count,title_language};
            tmn::update_title(*direct_title,p.state,timing,p,p.fp);break;
        }
        case 21: tmn::draw_title(*direct_title,p);break;
        case 22:
        case 23:
        case 24:
        case 26: {
            const bool present=(in[0]==22 || in[0]==24 || !(in[4]&1));
            if(present) {
                p.state.sequence_task=handle(1);
                if(!(in[4]&2) || in[2]!=0 || in[0]==23 || in[0]==26) {
                    p.state.sequence_work=std::make_shared<tmn::SequenceWork>(sequence);
                }
            }
            if(in[0]==22)run_result=tmn::run_title_menu(p.state,shared,pause_mode,p);
            else if(in[0]==23)run_result=tmn::close_title_menu(p.state,p);
            else if(in[0]==24)run_result=tmn::run_title_tasks(p.state,pause_mode,p);
            else tmn::destroy_sequence_manager(p.state,p);
            break;
        }
        case 25: tmn::draw_loading_text(shared,p);break;
        case 27: tmn::reset_selected_trial_level(p.state.trial);break;
        default: throw std::runtime_error("unknown probe operation");
        }
        completed = true;
    } catch (const Stop &) {}
    std::fill(out,out+512,0);
    for (std::size_t i=0;i<9;++i) out[i]=actions[i].identity;
    out[9]=font; out[10]=static_cast<std::uint8_t>(voice); out[11]=voice_state; out[12]=loops;
    out[13]=p.state.sequence_task?1:0; out[14]=p.state.dialog_task?3:0;
    if (p.state.sequence_work) {
        const auto &w=*p.state.sequence_work;
        out[15]=w.data?static_cast<std::uint32_t>(w.data-p.data.data())+1:0; out[16]=static_cast<std::uint32_t>(w.mode); out[17]=static_cast<std::uint32_t>(w.old_mode); out[18]=static_cast<std::uint32_t>(w.fixed_mode);
        out[19]=static_cast<std::uint32_t>(w.advance); out[20]=w.return_value; out[21]=static_cast<std::uint32_t>(w.waiting_mode);
        out[22]=static_cast<std::uint16_t>(w.frames); out[23]=w.waiting;
        out[24]=w.fade_end; out[25]=w.fade_out; out[26]=w.fade_out_secondary;
    }
    if (p.child) { out[27]=p.child->data?static_cast<std::uint32_t>(p.child->data-p.data.data())+1:0; out[28]=p.child->frames; }
    out[29]=p.sequence_exec; out[30]=p.sequence_destroy; out[31]=p.child_exec;
    out[32]=completed; out[33]=p.count;
    out[34]=sound.music_task?in[8]:0; out[35]=sound.volume_task?in[9]:0;
    out[36]=textures.loaded[0];out[37]=textures.loaded[1];out[38]=textures.ready;
    out[39]=manager?in[8]:0;
    out[40]=tracks.current_song;out[41]=tracks.previous_song;out[42]=tracks.real_previous_song;
    out[43]=tracks.current_voice;out[44]=tracks.song_queue_state;out[45]=tracks.voice_queue_state;
    const auto emit_dialog = [&](const tmn::DialogWork &d, std::uint32_t *dest) {
        dest[0]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(d.parameters));
        dest[1]=d.phase;dest[2]=d.color;dest[3]=Probe::bits(d.scale_x);dest[4]=Probe::bits(d.scale_y);
        dest[5]=Probe::bits(d.fade);dest[6]=static_cast<std::uint16_t>(d.repeat_frames) |
            (static_cast<std::uint32_t>(static_cast<std::uint16_t>(d.opening_frames))<<16);
        std::memcpy(dest+7,d.choice_ids.data(),16);dest[11]=d.previous_choice;dest[12]=d.blink_step;
    };
    if (in[0]==11 || in[0]==12) {
        emit_dialog(dialog,out+46);emit_dialog(in[4]&8?canonical_dialog:dialog,out+59);
    } else if (in[0]==13 && p.state.dialog_work) {
        emit_dialog(*p.state.dialog_work,out+46);emit_dialog(*p.state.dialog_work,out+59);
    }
    out[72]=p.dialog_exec;out[73]=p.dialog_display;
    const auto emit_sequence = [&](const tmn::SequenceWork &w,std::uint32_t *dest) {
        dest[0]=w.data?1:0;dest[1]=static_cast<std::uint32_t>(w.mode);
        dest[2]=static_cast<std::uint32_t>(w.old_mode);dest[3]=static_cast<std::uint32_t>(w.fixed_mode);
        dest[4]=static_cast<std::uint32_t>(w.advance);dest[5]=w.return_value;
        dest[6]=static_cast<std::uint32_t>(w.waiting_mode);dest[7]=static_cast<std::uint16_t>(w.frames);
        dest[8]=w.waiting;dest[9]=w.fade_end;dest[10]=w.fade_out;dest[11]=w.fade_out_secondary;
    };
    if(in[0]==14) {
        emit_sequence(sequence,out+74);emit_sequence(in[4]&8?canonical_sequence:sequence,out+86);
    }
    out[98]=demo_mode;out[99]=updating_child.frames;
    out[100]=p.state.title_task?4:0;
    if(p.state.title_work) {
        const auto &w=*p.state.title_work;
        out[101]=static_cast<std::uint32_t>(w.phase);out[102]=static_cast<std::uint32_t>(w.previous);
        out[103]=static_cast<std::uint32_t>(w.next);out[104]=Probe::bits(w.depth);
        out[105]=Probe::bits(w.fade_progress);out[106]=w.color;out[107]=static_cast<std::uint32_t>(w.submode);
        out[108]=w.alert_task?1:0;out[109]=static_cast<std::uint8_t>(w.vm_status);out[110]=w.timer;
        out[111]=w.blink_timer;out[112]=w.music_started?1:0;out[113]=w.wave_timer;
        std::memcpy(out+114,&w.cloud_offset,4);out[115]=w.cloud_index;
    }
    out[116]=p.title_exec;out[117]=p.title_display;
    if(in[0] >= 19 && in[0] <= 21) {
        const auto &w=*direct_title;
        out[120]=static_cast<std::uint32_t>(w.phase);out[121]=static_cast<std::uint32_t>(w.previous);
        out[122]=static_cast<std::uint32_t>(w.next);out[123]=Probe::bits(w.depth);
        out[124]=Probe::bits(w.fade_progress);out[125]=w.color;out[126]=static_cast<std::uint32_t>(w.submode);
        out[127]=w.alert_task?1:0;out[128]=static_cast<std::uint8_t>(w.vm_status);out[129]=w.timer;
        out[130]=w.blink_timer;out[131]=w.music_started?1:0;out[132]=w.wave_timer;
        std::memcpy(out+133,&w.cloud_offset,4);out[134]=w.cloud_index;
    }
    out[135]=p.fp.exception_flags();out[136]=adventure_loop_count;
    out[118]=tick_mode.pending_logic_ticks;out[119]=static_cast<std::uint32_t>(tick_failure);
    out[137]=pause_mode;out[138]=run_result;
    out[139]=p.state.trial.stage;out[140]=p.state.trial.act;
    std::copy(p.trace.begin(),p.trace.end(),out+141);
}
