#include "player/player_coordinates.hpp"

#include <utility>

namespace sadx {

CoordinateResult PlayerCoordinates::world_to_player(const PlayerOrientation& task, NinjaVector& vector,
                                                      X87Context& fp) {
    const NinjaVector saved = vector;
    (void)begin_unit_frame();
    if (task.angles[1] != 0) rotate_y(stack_.current(),task.angles[1]&0xffff,fp);
    if (task.angles[0] != 0) rotate_x(stack_.current(),ninja_wrapped_negation(task.angles[0]),fp);
    if (task.angles[2] != 0) rotate_z(stack_.current(),ninja_wrapped_negation(task.angles[2]),fp);
    const auto calculated=calculate_vector(stack_.current(),saved,vector,fp);
    if (!calculated) return calculated; // retained partial stores/stack ownership
    end_frame();
    return {};
}

CoordinateResult PlayerCoordinates::task_to_world(const PlayerOrientation* task, NinjaVector& vector,
                                                  X87Context& fp) {
    if (!task) return {}; // actual null-task branch, no matrix or vector change
    const NinjaVector saved=vector;
    (void)begin_unit_frame();
    if (task->angles[2] != 0) rotate_z(stack_.current(),task->angles[2]&0xffff,fp);
    if (task->angles[0] != 0) rotate_x(stack_.current(),task->angles[0]&0xffff,fp);
    if (task->angles[1] != 0) rotate_y(stack_.current(),ninja_wrapped_negation(task->angles[1]),fp);
    const auto calculated=calculate_vector(stack_.current(),saved,vector,fp);
    if (!calculated) return calculated;
    end_frame();
    return {};
}

CoordinateResult PlayerCoordinates::floor_to_world(const NinjaVector& normal, NinjaVector& vector,
                                                   std::int32_t yaw, const NormalAnglesProvider& provider,
                                                   X87Context& fp) {
    // 43edd8..43ede0/43eded..43edf1 capture the input before 43ee05's
    // angle-helper call, including when its native binding observes live state.
    const NinjaVector saved=vector;
    if (!provider) return {CoordinateFailure::missing_normal_angles,0x43ee05};
    const auto decoded=provider(normal,fp); // occurs BEFORE pushing matrix stack
    if (decoded.failure != CoordinateFailure::none) return {decoded.failure,0x43ee05};
    if (!decoded.angles) return {CoordinateFailure::normal_angles_unavailable,0x43ee05};
    (void)begin_unit_frame();
    if (decoded.angles->roll != 0) rotate_z(stack_.current(),decoded.angles->roll&0xffff,fp);
    if (decoded.angles->inclination != 0) rotate_x(stack_.current(),decoded.angles->inclination&0xffff,fp);
    if (yaw != 0) rotate_y(stack_.current(),ninja_wrapped_negation(yaw),fp);
    const auto calculated=calculate_vector(stack_.current(),saved,vector,fp);
    if (!calculated) return calculated;
    end_frame();
    return {};
}

FloorProjectionResult PlayerCoordinates::project_input_to_floor(const PlayerOrientation& task,
    const NinjaVector& normal,std::int32_t raw_angle,std::int32_t camera_pitch,
    std::int32_t camera_yaw,X87Value retained,const CoordinateAxisConstants& constants,X87Context& fp) {
    FloorProjectionResult result;
    NinjaVector camera_input{fp.to_float(retained),fp.to_float(retained),-1.0F};
    (void)begin_unit_frame();
    if(camera_yaw!=0)rotate_y(stack_.current(),camera_yaw&0xffff,fp);
    if(camera_pitch!=0)rotate_x(stack_.current(),camera_pitch&0xffff,fp);
    NinjaVector camera_output;
    result.coordinate=calculate_vector(stack_.current(),camera_input,camera_output,fp);
    result.camera_direction=camera_output;
    if(!result.coordinate){result.source_boundary=0x44b4a8;return result;}
    end_frame();
    (void)begin_unit_frame();
    rotate_axis(stack_.current(),normal,raw_angle,constants,fp);
    NinjaVector projected;
    result.coordinate=calculate_vector(stack_.current(),camera_output,projected,fp);
    result.projected_direction=projected;
    if(!result.coordinate){result.source_boundary=0x44b4db;return result;}
    end_frame();
    NinjaVector forward{1.0F,0.0F,0.0F};
    result.coordinate=task_to_world(&task,forward,fp);
    result.forward_direction=forward;
    if(!result.coordinate)result.source_boundary=0x44b508;
    return result;
}

} // namespace sadx
