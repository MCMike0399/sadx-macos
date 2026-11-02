#include "player/sonic_model.hpp"

namespace sadx::sonic_model {
void initialize_prefix(State &state, character_pack::ObjectState &objects, ObjectId attach,
                       ObjectId root, Player player, bool alternate, Calls &calls) {
    if (!alternate) {
        auto &object = objects.object(root);
        if (object.sibling != attach) {
            object.sibling = attach;
            player.joins = &state.joins;
            // Reread the live object after publication, just as the source does.
            state.joins.rows.at(11).destination_object =
                &objects.object(objects.object(root).sibling);
            player.join_mode = 0;
            calls.join_vertices();
            player.joins = &state.joins;
        }
    }
    // Metal skips only the splice above. Both paths read the same live slot six.
    state.model_sibling = objects.object(root).sibling;
    calls.initialize_collision();
    // CCL_Init's return is ignored. Its failed allocation must still fault here.
    calls.store_collision_height();
    player.mode = 1;
    player.idle_timer = 0;
}
} // namespace sadx::sonic_model
