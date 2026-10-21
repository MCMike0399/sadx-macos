#include "game/character_select_menu.hpp"
#include "cpu/x87_math.hpp"
#include "math/ninja_math.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace m = sadx::character_select_menu;
namespace {
float num(std::uint32_t x) { float f; std::memcpy(&f, &x, 4); return f; }
std::uint32_t bits(float x) { std::uint32_t f; std::memcpy(&f, &x, 4); return f; }
struct Calls : m::Calls, m::DrawCalls {
    const std::uint32_t *v;
    std::vector<std::uint32_t> trace;
    std::size_t completion_arguments = 0;
    explicit Calls(const std::uint32_t *p) : v(p) {}
    void rec(unsigned k, unsigned x=0) { trace.push_back(k); trace.push_back(x); }
    void default_blending() override { rec(54); }
    void base_color(unsigned x) override { rec(55,x); }
    void sky_background(float x) override { rec(56,bits(x)); }
    void select_texture(m::TextureList x) override { rec(40, static_cast<unsigned>(x)); }
    void material(unsigned x) override { rec(41,x); }
    void colors(unsigned a,unsigned b,unsigned c,unsigned d) override { trace.insert(trace.end(),{42,a,b,c,d}); }
    void draw_texture(unsigned i,float x,float y,float z) override { trace.insert(trace.end(),{43,i,bits(x),bits(y),bits(z)}); }
    void draw_scaled(unsigned i,float x,float y,float z,float a,float b) override { trace.insert(trace.end(),{44,i,bits(x),bits(y),bits(z),bits(a),bits(b)}); }
    void title_back(float x,float y,float z,float a,float b) override { trace.insert(trace.end(),{45,bits(x),bits(y),bits(z),bits(a),bits(b)}); }
    void set_depth_comparison(int x) override { rec(46,x); }
    void restore_depth_comparison() override { rec(47); }
    void round_panel(unsigned i,float x,float y,float z,float a,float b) override { trace.insert(trace.end(),{48,i,bits(x),bits(y),bits(z),bits(a),bits(b)}); }
    unsigned interpolate_colors(unsigned a,unsigned b,float t) override { trace.insert(trace.end(),{49,a,b,bits(t)});return bits(t)^a^b; }
    void character_name(int i,unsigned c,float x,float y,float z,float a,float b) override { trace.insert(trace.end(),{50,static_cast<unsigned>(i),c,bits(x),bits(y),bits(z),bits(a),bits(b)}); }
    void character_label(int i,unsigned c,float x,float y,float z) override { trace.insert(trace.end(),{51,static_cast<unsigned>(i),c,bits(x),bits(y),bits(z)}); }
    unsigned character_completion(int i) override {
        // Reference ABI: the caller's prepared Number arguments remain on the
        // source stack during the preceding count-only call. Project their
        // actual semantic values from the following Number call, preserving
        // the unchanged original verifier's complete stack receipt.
        completion_arguments = trace.size() + 2;
        trace.insert(trace.end(), {52, static_cast<unsigned>(i), 0, 0, 0, 0, 0, 0});
        return 37;
    }
    void draw_number(unsigned value, int digits, bool zero_fill, float x, float y, float z, float spacing) override {
        const std::array<unsigned, 6> arguments{static_cast<unsigned>(digits), zero_fill ? 1U : 0U,
                                               bits(x), bits(y), bits(z), bits(spacing)};
        for (std::size_t i = 0; i < arguments.size(); ++i)
            trace.at(completion_arguments + i) = arguments[i];
        rec(53, value);
    }
    void load_mode_textures(m::Mode x) override { rec(1, static_cast<unsigned>(x)); }
    void create_preview() override { rec(2); }
    void request_music(int x) override { rec(3, x); }
    void request_voice(int x) override { rec(4, x); }
    bool character_enabled(int x) override { rec(5, x); return v[30]; }
    bool trial_enabled(int x) override { rec(6, x); return v[31]; }
    bool character_cleared(int x) override { rec(7, x); return v[32]; }
    void release_loading_font() override { rec(8); }
    void calculate_ticks() override { rec(9); }
    void navigation_sound() override { rec(10); }
    bool confirm_pressed() override { rec(11); return v[33]; }
    bool cancel_pressed() override { rec(12); return v[34]; }
    bool selection_enabled() override { rec(13); return v[35]; }
    void transition_menu(m::Mode x) override { rec(14, static_cast<unsigned>(x)); }
    void cancel_sound() override { rec(15); }
    void open_dialog(int x) override { rec(16, x); }
    std::int8_t dialog_result() override { rec(17); return static_cast<std::int8_t>(v[36]); }
    void character_voice(int x) override { rec(18, x); }
    void select_preview(int x) override { rec(19, x); }
    int character_count() override { rec(20); return v[37]; }
    void set_stage(std::int16_t a, std::int16_t b) override { rec(21, static_cast<unsigned>(a)|(static_cast<unsigned>(b)<<16)); }
    void delete_preview() override { rec(22); }
    std::uint32_t fade_in_color(float f) override { rec(23,bits(f)); return bits(f)^12345; }
    std::uint32_t fade_out_color(float f) override { rec(24,bits(f)); return bits(f)^67890; }
    void release_mode_textures(m::Mode x) override { rec(25,static_cast<unsigned>(x)); }
};
}
extern "C" void sadx_native_character_select_menu_probe(const std::uint32_t *v, std::uint32_t *o, const float *sine) {
    std::fill_n(o,1024,0);
    m::Work w{};
    w.presentation.phase=static_cast<m::Phase>(v[1]); w.presentation.previous=static_cast<m::Mode>(v[2]);
    w.presentation.next=static_cast<m::Mode>(v[3]); w.presentation.depth=num(v[4]);
    w.presentation.fade_progress=num(v[5]); w.presentation.color=v[6]; w.cursor_angle=v[7];
    w.submode=v[8]; w.selected_character=v[9]; w.selected_file=v[10]; w.angle_progress=num(v[11]);
    w.frames=static_cast<std::int16_t>(v[12]); w.dialog_status=v[13]; w.wake=v[14];
    auto variant=static_cast<std::uint8_t>(v[15]), selected=static_cast<std::uint8_t>(v[16]);
    auto cursor=static_cast<std::uint8_t>(v[17]); auto voice=static_cast<std::uint16_t>(v[22]);
    auto subgame=static_cast<std::uint8_t>(v[23]); auto ready=static_cast<int>(v[18]), ticks=static_cast<int>(v[19]);
    auto increment=static_cast<int>(v[20]), saved=static_cast<int>(v[21]);
    m::Sequence seq{}; seq.mode=static_cast<m::Mode>(v[24]); seq.old_mode=static_cast<m::Mode>(v[25]);
    seq.fixed_mode=static_cast<m::Mode>(v[26]); seq.return_value=v[27];
    seq.fade_out=static_cast<std::uint8_t>(v[28]); seq.fade_end=static_cast<std::uint8_t>(v[28]>>8);
    m::State state{variant,selected,cursor,ready,ticks,v[29],increment,saved,voice,subgame,seq};
    Calls calls(v); sadx::X87Context fp{static_cast<std::uint16_t>(v[38])};
    sadx::NinjaMath::SineTable table{};std::copy_n(sine,1025,table.begin());sadx::NinjaMath math(table,fp);
    auto sx=num(v[39]),sy=num(v[40]);std::array<float,7> widths{};
    for(unsigned i=0;i<7;++i)widths[i]=num(v[41+i]);
    m::DrawState view{sx,sy,widths};
    if(v[0]==7)m::draw(w,state,view,calls,calls,math,fp);
    else if(v[0]==8)o[29]=m::selection_enabled(w.selected_character,state,calls);
    else if(v[0]==3)m::draw_background(w,state,view,calls,fp);
    else if(v[0]==4)m::draw_labels(w,state,calls,calls,fp);
    else if(v[0]==5)m::draw_carousel(w,state,view,calls,calls,math,fp);
    else if(v[0]==6)m::draw_emblems(w,state,view,calls,fp);
    else if(v[0]==0)m::initialize(w,state,calls);
    else if(v[0]==1)m::update_controls(w,state,calls,fp);
    else m::update(w,state,calls,fp);
    std::uint32_t data[]={static_cast<unsigned>(w.presentation.phase),static_cast<unsigned>(w.presentation.previous),
    static_cast<unsigned>(w.presentation.next),bits(w.presentation.depth),bits(w.presentation.fade_progress),
    w.presentation.color,static_cast<unsigned>(w.cursor_angle),static_cast<unsigned>(w.submode),
    static_cast<unsigned>(w.selected_character),w.selected_file,bits(w.angle_progress),
    static_cast<std::uint16_t>(w.frames),static_cast<unsigned>(w.dialog_status),w.wake,
    selected,cursor,static_cast<unsigned>(increment),static_cast<unsigned>(saved),voice,
    static_cast<unsigned>(seq.fixed_mode),static_cast<unsigned>(seq.return_value),seq.fade_out,seq.fade_end};
    std::copy(std::begin(data),std::end(data),o); o[30]=fp.exception_flags(); o[31]=calls.trace.size();
    std::copy(calls.trace.begin(),calls.trace.end(),o+32);
}
