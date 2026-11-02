#include "player/model_motion_source.hpp"
#include <cstring>

namespace sadx::model_matrices {
NativeMotionSource::NativeMotionSource(const animation::Action &action,
    const animation::Resources &resources, const character_pack::ObjectState &objects)
    : imported_(&action), resources_(&resources), objects_(objects) {}
NativeMotionSource::NativeMotionSource(const PlayerAnimationAllocatedAction &action,
    TaskWorkArena &arena, const character_pack::ObjectState &objects)
    : generated_(&action), arena_(&arena), objects_(objects) {}

const animation::Motion &NativeMotionSource::imported_motion() const {
    if (!imported_->motion.id)
        throw Error(Failure::MissingResource);
    const auto found = resources_->motions.find(*imported_->motion.id);
    if (found == resources_->motions.end())
        throw Error(Failure::MissingResource);
    return found->second;
}
const PlayerAnimationAllocatedMotion &NativeMotionSource::generated_motion() const {
    if (!generated_->motion || !*generated_->motion)
        throw Error(Failure::MissingResource);
    return **generated_->motion;
}
std::uint32_t NativeMotionSource::frame_count() {
    if (imported_)
        return imported_motion().frame_count;
    const auto &motion = generated_motion();
    if (!motion.frame_count)
        throw Error(Failure::MissingMotionFrameCount);
    return *motion.frame_count;
}
std::uint16_t NativeMotionSource::type() {
    if (imported_)
        return imported_motion().type;
    const auto &motion = generated_motion();
    if (!motion.data)
        throw Error(Failure::MissingMotionData);
    if (!motion.type)
        throw Error(Failure::MissingMotionType);
    return *motion.type;
}
ObjectId NativeMotionSource::root() {
    if (generated_ && !generated_->object)
        throw Error(Failure::MissingActionObject);
    const auto &reference = imported_ ? imported_->object : *generated_->object;
    if (!reference.id)
        return 0;
    return *reference.id;
}
const character_pack::ObjectResource &NativeMotionSource::object(ObjectId id) {
    return objects_.object(id);
}
const animation::Node &NativeMotionSource::imported_node(unsigned node) const {
    const auto &motion = imported_motion();
    if (!motion.data.id)
        throw Error(Failure::MissingResource);
    const auto found = resources_->node_arrays.find(*motion.data.id);
    if (found == resources_->node_arrays.end() || node >= found->second.size())
        throw Error(Failure::MissingKeys);
    return found->second[node];
}
TaskWorkHandle NativeMotionSource::generated_keys(unsigned node, Channel channel) const {
    const auto &motion = generated_motion();
    if (!motion.data || !*motion.data)
        throw Error(Failure::MissingKeys);
    const unsigned stride = (*motion.type & 4U) ? 24 : 16;
    const auto reference = arena_->reference(*motion.data, node * stride + unsigned(channel) * 4);
    if (!reference)
        throw Error(Failure::MissingKeys);
    return reference.work;
}
bool NativeMotionSource::has_keys(unsigned node, Channel channel) {
    if (imported_)
        return imported_node(node).keys[unsigned(channel)].id.has_value();
    return bool(generated_keys(node, channel));
}
std::uint32_t NativeMotionSource::key_word(unsigned node, Channel channel,
                                         unsigned key, unsigned word) {
    std::uint32_t result;
    const auto offset = std::size_t(key) * 16 + word * 4;
    if (imported_) {
        const auto &reference = imported_node(node).keys[unsigned(channel)];
        if (!reference.id)
            throw Error(Failure::MissingKeys);
        const auto found = resources_->key_arrays.find(*reference.id);
        if (found == resources_->key_arrays.end())
            throw Error(Failure::MissingKeys);
        const auto &keys = found->second;
        if (!keys.bytes || offset + 4 > keys.byte_extent ||
            keys.offset + offset + 4 > keys.bytes->size())
            throw Error(Failure::MissingKeys);
        std::memcpy(&result, keys.bytes->data() + keys.offset + offset, 4);
    } else {
        const auto keys = generated_keys(node, channel);
        if (!keys || !arena_->read(keys, offset, &result, 4))
            throw Error(Failure::MissingKeys);
    }
    return result;
}
} // namespace sadx::model_matrices
