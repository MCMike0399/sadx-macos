// Native closure: real task pool, shared primary/motion and actual PInitialize.
// The unrecovered lower seams use the same concrete receipts as the source judge.
#include "../probes/native_gamma_selection_core_probe.cpp"
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
}
int main() {
    using namespace sadx;
    namespace gamma = gamma_selection;
    try {
        std::array<std::uint32_t,32> input{};
        input[2] = 2; input[5] = gamma::bits(1); input[16] = gamma::bits(4);
        input[17] = gamma::bits(9); input[18] = gamma::bits(2);
        input[21] = 0xffffffff; input[23] = 0; input[24] = 2;
        input[26] = gamma::bits(15);
        gamma::Context context(input.data(),127);
        context.native_initialize = true;
        context.parameters[6].rad = 5;
        context.parameters[6].height = 15;
        auto primary_owner = context.task->primary_state;
        auto motion_owner = context.task->motion_state;
        motion_owner->target_yaw = 16384;
        gamma::run(context,context.task);
        auto p = player_initialization::bind_primary_view(context.task);
        require(context.task->primary_state == primary_owner &&
                    context.task->motion_state == motion_owner,
                "Gamma must borrow the real preview primary/motion owners");
        require(p.id == 2 && p.character == 6 && p.mode == 1 &&
                    motion_owner->target_yaw == 16384,
                "ID 2 runs initialization without replacing the actor or aim");
        require(context.registry.players[0] == context.live &&
                    context.registry.primary[0] == primary_owner &&
                    context.registry.motion[0] == motion_owner,
                "actual PInitialize publishes the existing canonical owners");
        require(context.live->auxiliary &&
                    context.live->auxiliary != gamma::extra_work(context,context.live),
                "PInitialize auxiliary allocation and Gamma free work are distinct");
        require(context.published_primary == context.task->primary_work &&
                    !context.published_player,
                "current player remains the initial null publication after PInitialize");
        require(context.globals.time_hud_enabled == 0 && context.task->display &&
                    context.task->destructor && context.collision_extent == 15,
                "ID 2 skips only the time HUD and retains source setup");
        require(context.globals.line_points == context.globals.line_vertices.data() &&
                    context.globals.line_colors == context.line_colors.data() &&
                    context.globals.line_texture == nullptr && context.globals.line_point_count == 2,
                "line descriptor borrows imported colors and the one live Gamma endpoint owner");
        require(context.calls >= 28 && context.extra_data[0].timer == 1 &&
                    context.extra_data[0].ground == 2 && context.extra_data[0].distance == 7,
                "mandatory tail continues through display, shadow and child tasks");
        std::cout << "PASS: real Gamma mode 0, ID 2, task/motion/PInitialize ownership and common tail\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "native Gamma closure stopped at a dependency boundary\n";
        return 1;
    }
}
