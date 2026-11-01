#include "apps/native_gamma_actor_assets.hpp"
#include "player/player_initialization.hpp"
#include <sstream>
#include <iomanip>
extern "C" const char* final_gamma(const char* imports,const std::uint32_t* in) {
    @autoreleasepool {
        using namespace sadx;
        static std::string result;
        static const auto assets=native::app::load_gamma_actor_assets(std::filesystem::path(imports),character_pack::CharacterPack::load(std::filesystem::path(imports)/"characters/sonic-selection.actions"));
        auto first=std::make_shared<player_initialization::Player>();auto second=std::make_shared<player_initialization::Player>();
        auto init=[&](auto p){p->physics=std::make_shared<PlayerPhysicsState>();auto& f=*p->physics;f.timer_at_80=0;f.source_flags_at_6=0;f.requested_animation_at_124=0;f.current_animation_at_126=0;f.held_task_at_68=TaskHandle{};
#ifdef CURRENT_CONTRACT
            f.animation_tail.table_at_140=std::shared_ptr<const animation::TableStorage>{};
#else
            f.animation_tail.table_at_140=std::shared_ptr<const PlayerAnimationTable>{};
#endif
            f.animation_tail.join_table=static_cast<PlayerJoinTable*>(nullptr);};
        init(first);init(second);
        auto a=player_initialization::bind_initialized_player(first),b=player_initialization::bind_initialized_player(second);
        a.animation_table=assets.animation_table;b.animation_table=assets.animation_table;
        a.requested_animation=static_cast<std::uint16_t>(in[0]%91);b.current_animation=static_cast<std::uint16_t>(in[1]%91);
        std::ostringstream out;out<<"{\"identity\":["<<(a.animation_table.get()==assets.animation_table.get())<<','<<(b.animation_table.get()==assets.animation_table.get())<<','<<(a.animation_table==b.animation_table)<<"],\"requests\":["<<*first->physics->requested_animation_at_124<<','<<*second->physics->current_animation_at_126<<"],\"rows\":[";
        for(unsigned i=0;i<91;++i) {
            if(i)out<<',';const auto& row=assets.animation_table->records.at(i);const auto& recipe=assets.recipes[i];
            auto bits=[](float x){std::uint32_t y;std::memcpy(&y,&x,4);return y;};
            out<<'['<<i<<','<<static_cast<unsigned>(recipe.resource.status)<<',';
#ifdef CURRENT_CONTRACT
            out<<(row.action.status==animation::ResourceStatus::Loaded?2:row.action.status==animation::ResourceStatus::KnownNull?1:0)<<','<<(row.action.status==animation::ResourceStatus::Loaded?row.action.identity:0)<<','<<+row.object_count<<','<<static_cast<int>(row.mode)<<','<<row.next<<','<<bits(row.transition_step)<<','<<bits(row.frame_step)<<','<<std::quoted(row.action.unavailable_reason);
#else
            out<<(row.action?(row.action->id?2:1):0)<<','<<(row.action&&row.action->id?*row.action->id:0)<<','<<+*row.object_count<<','<<static_cast<int>(*row.mode)<<','<<*row.next<<','<<bits(*row.transition_step)<<','<<bits(*row.frame_step)<<','<<std::quoted(recipe.resource.boundary);
#endif
            out<<']';
        }
        out<<"],\"PSetMotion\":\"UNRECOVERED\"}";result=out.str();return result.c_str();
    }
}
