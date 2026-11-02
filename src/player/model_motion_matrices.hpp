#pragma once

#include "engine/character_pack.hpp"
#include "engine/model_matrices.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::model_matrices {
using ObjectId = character_pack::ResourceId;
enum class Channel { Position, Angle, Scale };
enum class Failure {
    MissingResource,
    MissingKeys,
    MissingMotionData,
    MissingMotionType,
    MissingMotionFrameCount,
    MissingActionObject,
    MissingMatrixStorage,
    ZeroFrameCount,
    UnsupportedLayout
};
class Error : public std::runtime_error {
  public:
    explicit Error(Failure failure) : std::runtime_error("model motion matrix boundary"), failure(failure) {}
    Failure failure;
};
// NOTSA: a transient typed view of the existing imported/generated motion and
// scene owners. Implementations resolve data lazily; none owns mutable poses.
struct MotionSource {
    virtual ~MotionSource() = default;
    virtual std::uint32_t frame_count() = 0;
    virtual std::uint16_t type() = 0;
    virtual ObjectId root() = 0;
    virtual const character_pack::ObjectResource &object(ObjectId) = 0;
    virtual bool has_keys(unsigned node, Channel) = 0;
    virtual std::uint32_t key_word(unsigned node, Channel, unsigned key, unsigned word) = 0;
};

// FUNCTION: SONIC 0x4b81f0 CalcMMMatrix
bool calculate(State &, NinjaMatrixPrimitives &, MotionSource &, const NinjaMatrix *base,
               float frame, std::uint32_t requested_node, NinjaMatrix *destination,
               fidelity::Context &);
} // namespace sadx::model_matrices
