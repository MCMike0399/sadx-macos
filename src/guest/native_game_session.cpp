#include "guest/native_game_session.hpp"
#include "input/input_mod_constants.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
namespace sadx {
namespace {
std::shared_ptr<const ExecutableImage> load(const std::string& path,std::string& error){
    std::ifstream f(path,std::ios::binary);if(!f){error="cannot read "+path;return {};}
    std::string parse_error;auto image=ExecutableImage::parse({std::istreambuf_iterator<char>(f),{}},&parse_error);
    if(!image){error=path+": "+parse_error;return {};}
    return std::make_shared<const ExecutableImage>(std::move(*image));
}
std::string directory_of(const std::string& path){const auto slash=path.find_last_of('/');return slash==std::string::npos?std::string("."):path.substr(0,slash);}
}
BootLoadResult load_boot_config(const std::string& exe_path,const std::string& input_mod_path,std::uint32_t frames,OwnerPolicy policy,const SpeedrunLimiterClock& clock){
    BootLoadResult r;NativeGameConfig config;
    config.executable=load(exe_path,r.error);if(!config.executable)return r;
    config.input_mod_dll=load(input_mod_path,r.error);if(!config.input_mod_dll)return r;
    // Native code tables are maintained source; images still supply unmigrated state/assets.
    config.engine_tables=runtime::engine_code_tables();
    config.control_defaults=runtime::session_control_defaults();
    config.input_defaults=input::input_initial_state();
    config.sine_table=ninja_sine_table();
    config.fog_defaults=ninja_fog_initial_state();
    config.screen_defaults=ninja_screen_initial_state();
    config.sound_defaults=sound_initial_state();
    config.view_defaults=game_view_initial_state();
    config.queue_defaults=ninja_queue_initial_state();
    config.startup_defaults=startup_initial_state();
    config.clock=clock;config.max_frames=frames;config.policy=policy;
    {   // 401ac0's model DLLs from the game's system directory (slot 0: CHRMODELS_orig.dll behind the loader proxy).
        const auto system=directory_of(exe_path)+"/system/";
        const char* names[7]={"CHRMODELS_orig.dll","ADV00MODELS.DLL","ADV01MODELS.DLL","ADV01CMODELS.DLL","ADV02MODELS.DLL","ADV03MODELS.DLL","BOSSCHAOS0MODELS.DLL"};
        for(std::size_t i=0;i<7;++i){std::FILE* f=std::fopen((system+names[i]).c_str(),"rb");if(!f)continue;std::fclose(f);std::string ignored;config.model_dlls[i]=load(system+names[i],ignored);}
    }
    {   // WinMain's .\sonicDX.ini next to the executable (absent: Win32 fallbacks).
        std::ifstream f(directory_of(exe_path)+"/sonicDX.ini",std::ios::binary);if(f)config.sonicdx_ini=std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f),{});
    }
    config.input_constants=input_mod_261_constants();
    for(auto& s:config.pad_settings)s={false,7849,8689,true,false,30}; // profile: deadzones, radial L, threshold 30
    config.pad_settings[0].allow_keyboard=true;                          // keyboardPlayer 0
    r.config=std::move(config);return r;
}
std::string describe_report(const NativeGameReport& r,const D3D8StateOwner& d,OwnerPolicy policy){
    std::string out;char line[512];
    out+=std::string("policy: ")+(policy==OwnerPolicy::survey_unported_as_noop?"survey (unported owners treated as no-ops; not game behaviour)":"stop at first unported owner")+"\n";
    std::snprintf(line,sizeof line,"startup completed: %s\nframes completed: %u\nexited: %s\n",r.startup_completed?"yes":"no",r.frames_completed,r.exited?"yes":"no");out+=line;
    if(r.stopped_at){std::snprintf(line,sizeof line,"native execution stops at: %s (call site %x)\n",r.stopped_at->path.c_str(),r.stopped_at->first_site);out+=line;}
    if(!r.failure.empty())out+="failure: "+r.failure+"\n";
    std::snprintf(line,sizeof line,"D3D8 frame methods issued (recording owner): BeginScene %llu, EndScene %llu, Clear %llu%s, Present %llu\n",
        static_cast<unsigned long long>(d.begin_scenes),static_cast<unsigned long long>(d.end_scenes),static_cast<unsigned long long>(d.clears),
        d.last_clear_flags?(" (last flags "+std::to_string(*d.last_clear_flags)+")").c_str():"",static_cast<unsigned long long>(d.presents));out+=line;
    for(const auto& a:r.assumptions)out+="assumption: "+a+"\n";
    std::snprintf(line,sizeof line,"unported owners reached (%zu, first-reached order):\n",r.unported.size());out+=line;
    for(const auto& u:r.unported){std::snprintf(line,sizeof line,"  %-40s site %x  calls %u\n",u.path.c_str(),u.first_site,u.calls);out+=line;}
    return out;
}
} // namespace sadx
