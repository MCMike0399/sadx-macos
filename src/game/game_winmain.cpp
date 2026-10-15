#include "game/game_winmain.hpp"
namespace sadx {
namespace {
struct Stop{};
struct Operation {
    GameWinMainRuntime& rt;GameWinMainResult r;
    [[noreturn]]void fail(GameWinMainFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t at,std::uint32_t fact){if(!v)fail(GameWinMainFailure::unknown_state,at,fact);return *v;}
    std::optional<std::uint32_t> call(std::uint32_t site,std::uint32_t callee,std::array<std::optional<std::uint32_t>,4> arguments={}){
        r.pending_call=WinMainCall{site,callee,arguments};
        if(!rt.invoke)fail(GameWinMainFailure::missing_owner,site);
        const auto receipt=rt.invoke(*r.pending_call);
        if(receipt.owner.state!=GameLogicOwnerState::completed)fail(receipt.owner.state==GameLogicOwnerState::cancelled?GameWinMainFailure::cancelled:
            receipt.owner.state==GameLogicOwnerState::failed?GameWinMainFailure::owner_failed:GameWinMainFailure::missing_owner,site);
        r.pending_call.reset();return receipt.eax;}
    std::int32_t profile(std::uint32_t site,const char* section,const char* key,std::int32_t fallback){return profile(site,std::string(section),std::string(key),fallback);}
    std::int32_t profile(std::uint32_t site,const std::string& section,const std::string& key,std::int32_t fallback){
        if(!rt.profile_int)fail(GameWinMainFailure::profile_unavailable,site);
        const auto v=rt.profile_int({site,section,key,fallback,{},0});if(!v)fail(GameWinMainFailure::profile_unavailable,site);return *v;}
    // (v*m)/100 by the compiler's 0x51eb851f reciprocal (truncating toward zero), minus 100, times 100.
    static std::uint32_t volume(std::int32_t v,std::int32_t m){
        const auto product=static_cast<std::int32_t>(static_cast<std::uint32_t>(v)*static_cast<std::uint32_t>(m));
        const auto high=static_cast<std::int32_t>((static_cast<std::int64_t>(product)*0x51eb851fLL)>>32)>>5;
        const auto quotient=high+static_cast<std::int32_t>(static_cast<std::uint32_t>(high)>>31);
        return static_cast<std::uint32_t>(quotient-100)*100U;}
    void run(){
        auto& s=rt.state;auto& b=rt.borrowed;
        const auto module=call(0x4025c9,0x7db040).value_or(0); // LoadLibraryA("imm32.dll")
        r.imm32_module=module;
        if(module!=0){
            const auto proc=call(0x4025e1,0x7db044,{module,0x7dcaf0u}).value_or(0); // GetProcAddress(module,"ImmDisableIME")
            if(proc==0)fail(GameWinMainFailure::owner_failed,0x4025e8); // call eax with a null pointer
            call(0x4025e8,proc,{0u});                                           // ImmDisableIME(0)
        }
        s.voice_language_at_3b0ef38=1;s.text_language_at_3b0f0e8=1;
        // 402614: the loader's SetLanguage (WriteCall over a debug print).
        s.voice_language_at_3b0ef38=static_cast<std::uint32_t>(rt.loader.voice_language);s.text_language_at_3b0f0e8=static_cast<std::uint32_t>(rt.loader.text_language);
        call(0x402621,0x401a20);                                                // CD drive scan into 3a8af78
        // 402626..402633 read 3a8af78 and MessageBoxA's IAT slot; both reach only a debug print.
        if(need(s.media_at_3ab918c,0x40267b,0x3ab918c)==0||need(s.media_at_3ab9190,0x402683,0x3ab9190)==0){
            const auto caption=need(s.message_caption_at_8929c4,0x40268b,0x8929c4),text=need(s.message_text_at_8929b0,0x402691,0x8929b0);
            call(0x40269a,0x7db1e0,{0u,text,caption,0u});} // MessageBoxA(0, text, caption, 0)
        {auto f=profile(0x4026c5,"sonicDX","framerate",2);if(f<=0)f=1;else if(f>3)f=3;b.configured_frame_interval=f;}
        {const auto fog=profile(0x40270d,"sonicDX","fogemulation",0);const auto flags=need(b.special_flags_at_909eb4,0x402715,0x909eb4);
            b.special_flags_at_909eb4=static_cast<std::uint8_t>(fog<=0?(flags&0xfe):(flags|1));}
        {auto sound=profile(0x402747,"sonicDX","sound3d",1);if(sound<0)sound=0;else if(sound>1)sound=1;
            s.sound3d_at_3b0ef28=static_cast<std::uint32_t>(sound<0?0:sound>3?3:sound);} // 40ff20
        {auto clip=profile(0x4027a0,"sonicDX","cliplevel",1);s.clip_level_at_3abdcf0=static_cast<std::uint32_t>(clip);
            if(clip<0)s.clip_level_at_3abdcf0=0;else if(clip>2)s.clip_level_at_3abdcf0=2;}
        {auto se=profile(0x4027e1,"sonicDX","sevoice",1);b.sevoice_at_38f6ec0=static_cast<std::uint32_t>(se);
            if(se<0)b.sevoice_at_38f6ec0=0;else if(se>1)b.sevoice_at_38f6ec0=1;}
        {auto v=profile(0x40281f,"sonicDX","voicev",100);
            if(v<=0){v=0;b.sevoice_at_38f6ec0=0;}else if(v>100)v=100;
            b.voice_volume_at_3abdfac=volume(v,0x61);
            // 40ccf0(0): 3abdf90 = 3abdfac; when 3abdfac > 0 (signed) it is reduced by 0.
            const auto current=*b.voice_volume_at_3abdfac;b.voice_copy_at_3abdf90=current;
            if(static_cast<std::int32_t>(current)>0)b.voice_volume_at_3abdfac=current-0U;}
        {auto bgm=profile(0x402882,"sonicDX","bgm",1);b.bgm_at_3a7cf3c=static_cast<std::uint32_t>(bgm);
            if(bgm<0)b.bgm_at_3a7cf3c=0;else if(bgm>1)b.bgm_at_3a7cf3c=1;}
        {auto v=profile(0x4028c0,"sonicDX","bgmv",0x59);
            if(v<=0){v=0;b.bgm_at_3a7cf3c=0;}else if(v>100)v=100;
            s.bgm_volume_at_909f28=volume(v,0x59);
            if(need(b.bgm_at_3a7cf3c,0x40290d,0x3a7cf3c)==0)s.bgm_volume_at_909f28=0xffffd8f0u;}
        {const auto screen=profile(0x402930,"sonicDX","screen",1);
            if(screen<=0)s.screen_at_892950=1;
            else{s.screen_at_892950=0;for(;;){const auto count=call(0x402961,0x7db1dc,{0u});if(!count)fail(GameWinMainFailure::owner_failed,0x402963);
                if(static_cast<std::int32_t>(*count)<=0)break;}}} // ShowCursor(FALSE) until the display count is <= 0
        // 40297a: the loader jumps to 402a90 (no screensize read).
        const std::array<std::int32_t,9> defaults{2,1,0xd,0xa,9,0xc,0x11,0x10,3};
        if(!rt.profile_string)fail(GameWinMainFailure::profile_unavailable,0x402aff);
        const auto config=rt.profile_string({0x402aff,"sonicDX","padconfig",0,"pad_default",0x20});
        if(!config)fail(GameWinMainFailure::profile_unavailable,0x402aff);
        auto buttons=profile(0x402b16,*config,"buttons",0);
        if(buttons!=0){
            if(buttons>0xd)buttons=0xd;
            for(auto& w:s.pad_buttons_at_389d634)w=0xffff;
            for(std::int32_t i=0;i<buttons;++i){
                auto v=profile(0x402b66,*config,"pad"+std::to_string(i),-2);
                if(v==-2){if(i>=9)continue;v=defaults[static_cast<std::size_t>(i)];}
                if(v<=0||v>0x11)continue;
                const auto bit=need(s.pad_bits_at_8929d8[static_cast<std::size_t>(v-1)],0x402b7f,0x8929d4+4*static_cast<std::uint32_t>(v));
                if(bit<0)continue;
                if(bit>12)fail(GameWinMainFailure::unknown_state,0x402b8a,0x389d634+2*static_cast<std::uint32_t>(bit)); // past the 13 words this owner holds
                s.pad_buttons_at_389d634[static_cast<std::size_t>(bit)]=static_cast<std::uint16_t>(i);
            }
        }
        {auto mode=profile(0x402bba,"sonicDX","mousemode",0);if(mode<0)mode=0;else if(mode>1)mode=1;
            s.mouse_mode_at_3b0eae0=static_cast<std::uint32_t>(mode);} // 40e8a0
        for(std::uint32_t i=0;i<5;++i){
            const auto v=profile(0x402c01,"sonicDX","cmd"+std::to_string(i),0);
            if(v>0)call(0x402c09,0x40e8b0,{i,static_cast<std::uint32_t>(v)});
        }
    }
};
}
CdDriveResult run_cd_drive_scan_401a20(CdDriveState& s,NinjaMemoryState& memory,GuestHeap& heap,const CdDriveHost& host){
    CdDriveResult r;const auto stop=[&](CdDriveFailure f,std::uint32_t at,std::optional<std::uint32_t> fact={}){r.failure=f;r.source_boundary=at;r.required_fact=fact;return r;};
    if(!s.root_at_3a8af78.write(0x3a8af78,0,1))return stop(CdDriveFailure::unknown_state,0x401a2d,0x3a8af78);
    if(!host.logical_drive_strings||!host.drive_type||!host.file_exists)return stop(CdDriveFailure::host_unavailable,0x401a34);
    const auto sized=host.logical_drive_strings(0);if(!sized)return stop(CdDriveFailure::host_unavailable,0x401a34);
    const auto allocation=nj_malloc_780f60(memory,heap,sized->first);if(!allocation)return stop(CdDriveFailure::memory_failed,allocation.source_boundary);
    const auto buffer=allocation.eax;if(buffer==0)return r;
    const auto release=[&]()->CdDriveResult{const auto f=nj_free_780fb0(memory,heap,buffer);if(!f)return stop(CdDriveFailure::memory_failed,f.source_boundary);return r;};
    const auto filled=host.logical_drive_strings(sized->first);if(!filled)return stop(CdDriveFailure::host_unavailable,0x401a49);
    for(std::size_t i=0;i<filled->second.size()&&i<sized->first;++i)heap.write(static_cast<std::uint32_t>(buffer+i),filled->second[i]);
    if(filled->first==0)return release();
    const auto byte=[&](std::uint32_t offset,std::uint32_t at)->std::optional<std::uint8_t>{const auto v=heap.read(buffer+offset);if(!v)stop(CdDriveFailure::unknown_state,at,buffer+offset);return v;};
    auto first=byte(0,0x401a4f);if(!first)return r;if(*first==0)return release();
    std::uint32_t offset=0;
    for(;;){
        std::string drive;for(std::uint32_t k=offset;;++k){const auto v=byte(k,0x401a61);if(!v)return r;if(*v==0)break;drive.push_back(static_cast<char>(*v));}
        r.queries.push_back({'D',drive});
        const auto type=host.drive_type(drive);if(!type)return stop(CdDriveFailure::host_unavailable,0x401a61);
        if(*type==5){ // 401990: wsprintfA then FindFirstFileA
            const auto pattern=drive+"SoundData\\SE\\V_AMY_J_BANK06.dat";r.queries.push_back({'F',pattern});
            const auto found=host.file_exists(pattern);if(!found)return stop(CdDriveFailure::host_unavailable,0x4019d1);
            if(*found){ // 401aa2: lstrcpyA(3a8af78, buffer + offset)
                if(drive.size()+1>s.root_at_3a8af78.size)return stop(CdDriveFailure::buffer_overflow,0x401aaa,0x3a8af78+0x104);
                for(std::size_t k=0;k<=drive.size();++k)s.root_at_3a8af78.write(static_cast<std::uint32_t>(0x3a8af78+k),k<drive.size()?static_cast<std::uint8_t>(drive[k]):0,1);
                return release();}
        }
        // 401a75: skip to this string's NUL, then step past it.
        auto current=byte(offset,0x401a75);if(!current)return r;
        if(*current!=0){for(;;){const auto v=byte(offset+1,0x401a80);if(!v)return r;++offset;if(*v==0)break;}}
        ++offset;const auto next=byte(offset,0x401a8a);if(!next)return r;
        if(*next==0)break;
    }
    return release();
}
GameWinMainResult run_winmain_settings(GameWinMainRuntime& rt){
    Operation op{rt,{}};
    try{op.run();}catch(Stop){}
    return op.r;
}
} // namespace sadx
