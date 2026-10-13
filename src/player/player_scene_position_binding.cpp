#include "player/player_scene_position_binding.hpp"

namespace sadx {
namespace {
decltype(PlayerPositionCallees::ordinary_collision) bind_collision(bool alternate,
    PlayerSceneLandscape& landscape,const PlayerScenePositionConstants& constants,
    PlayerScenePositionRuntime& runtime){
    return [alternate,&landscape,&constants,&runtime](PlayerTaskState& task,
        PlayerMotionState* motion,PlayerPhysicsState& player,PlayerParameters& parameters,
        PlayerPositionCollisionScene&,PlayerCoordinates* coordinates,X87Context& fp){
        // No scene invocation can be made without its native matrix owner.
        // This identifies the uninvoked source entry, not an invented successful
        // scene result or an instruction that the source executed.
        if(!coordinates)return PositionCollisionResult{PlayerPositionFailure::missing_matrix_owner,
            {},alternate?0x445d10U:0x449500U,{}};
        auto child=std::make_shared<PlayerScenePositionResult>(alternate?
            position_player_in_scene_alternate(task,motion,player,parameters,landscape,constants,runtime,*coordinates,fp):
            position_player_in_scene(task,motion,player,parameters,landscape,constants,runtime,*coordinates,fp));
        return PositionCollisionResult{*child?PlayerPositionFailure::none:PlayerPositionFailure::collision_owner_failed,
            child->outcome,child->source_boundary,std::move(child)};
    };
}
}
void bind_player_scene_position(PlayerPositionCallees& callees,PlayerSceneLandscape& landscape,
    const PlayerScenePositionConstants& constants,PlayerScenePositionRuntime& runtime){
    callees.ordinary_collision=bind_collision(false,landscape,constants,runtime);
    callees.alternate_collision=bind_collision(true,landscape,constants,runtime);
}
} // namespace sadx
