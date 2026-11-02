#include "player/model_motion_matrices.hpp"
#include "engine/ninja_matrix_composition.hpp"
#include <algorithm>
#include <cstring>

namespace sadx::model_matrices {
namespace {
using fidelity::ExactFloat;

// NOTSA: reinterpret source words without changing NaNs or signed integer bits.
template <class T> T value(std::uint32_t word) {
    T result;
    std::memcpy(&result, &word, sizeof(result));
    return result;
}
// NOTSA: source unsigned conversion uses a signed FILD followed by correction.
ExactFloat unsigned_value(fidelity::Context &fp, std::uint32_t word) {
    auto result = ExactFloat::integer(fp, value<std::int32_t>(word));
    if (value<std::int32_t>(word) < 0)
        result = result + 4294967296.0f;
    return result;
}
// NOTSA: one scanner invocation borrows persistent output and existing stack.
struct Scan {
    State &state;
    NinjaMatrixPrimitives &coordinates;
    MotionSource &motion;
    fidelity::Context &fp;
    std::uint32_t period;
    float frame;
    unsigned channels;
    unsigned node = 0;

    // NOTSA: the interval scan uses unsigned frame comparisons and leaves its
    // following pointer one key ahead, including the exact-current-key case.
    float coefficient(Channel channel, unsigned &first, unsigned &second) {
        const auto integer_frame = static_cast<std::uint32_t>(ExactFloat(fp, frame).truncated_integer());
        first = 0;
        second = 1;
        auto current = motion.key_word(node, channel, first, 0);
        while (current < integer_frame) {
            const auto next = motion.key_word(node, channel, second, 0);
            if (next > integer_frame)
                break;
            ++first;
            ++second;
            current = next;
        }
        auto delta = ExactFloat(fp, frame) - unsigned_value(fp, current);
        if (current == period - 1U) {
            second = 0;
            return delta.rounded();
        }
        const auto next = motion.key_word(node, channel, second, 0);
        return (delta / unsigned_value(fp, next - current)).rounded();
    }

    // NOTSA: apply one sampled channel, retaining the binary32 coefficient spill.
    void sample(Channel channel) {
        unsigned first, second;
        const float fraction = coefficient(channel, first, second);
        std::array<std::uint32_t, 3> a{}, b{};
        for (unsigned axis = 0; axis < 3; ++axis) {
            a[axis] = motion.key_word(node, channel, first, axis + 1);
            b[axis] = motion.key_word(node, channel, second, axis + 1);
        }
        if (channel == Channel::Angle) {
            std::array<std::int32_t, 3> angles{};
            // The source converts Z, Y, X, then passes all three by value.
            for (int axis = 2; axis >= 0; --axis) {
                const auto low = static_cast<std::uint16_t>(b[axis] - a[axis]);
                std::int16_t difference;
                std::memcpy(&difference, &low, sizeof(difference));
                const auto step = (ExactFloat::integer(fp, difference) * fraction).truncated_integer();
                angles[axis] = value<std::int32_t>(a[axis] + static_cast<std::uint32_t>(step));
            }
            rotate_ninja_xyz(coordinates, coordinates.current_matrix(), angles, fp);
        } else {
            std::array<float, 3> sampled{};
            // Source evaluates X, Z, Y before the translate/scale call.
            for (unsigned axis : {0U, 2U, 1U}) {
                auto initial = ExactFloat(fp, value<float>(a[axis]));
                auto difference = ExactFloat(fp, value<float>(b[axis])) - initial;
                sampled[axis] = (difference * fraction + initial).rounded();
            }
            auto &matrix = coordinates.current_matrix();
            if (channel == Channel::Position)
                translate_ninja_matrix(matrix, sampled[0], sampled[1], sampled[2], fp);
            else
                coordinates.scale_matrix(matrix, sampled[0], sampled[1], sampled[2], fp);
        }
    }

    // FUNCTION: SONIC 0x4b7d00 ScanMotionModel
    ObjectId visit(ObjectId id) {
        ++state.count;
        if (state.count > state.limit) {
            state.count = state.limit;
            return 0;
        }
        coordinates.push_current_frame();
        if (channels != 2 && channels != 3)
            throw Error(Failure::UnsupportedLayout); // source spins here; no fabricated transform
        for (unsigned index = 0; index < channels; ++index) {
            const auto channel = static_cast<Channel>(index);
            if (motion.has_keys(node, channel)) {
                sample(channel);
                continue;
            }
            const auto &object = motion.object(id);
            const auto unit_flag = 1U << index;
            // Original MDATA2 position fallback ignores UnitPosition.
            if ((channels == 2 && channel == Channel::Position) ||
                (object.evaluation_flags & unit_flag) == 0) {
                auto &matrix = coordinates.current_matrix();
                if (channel == Channel::Position)
                    coordinates.translate_matrix(matrix,
                        {object.position[0], object.position[1], object.position[2]}, fp);
                else if (channel == Channel::Angle)
                    rotate_ninja_xyz(coordinates, matrix, object.rotation, fp);
                else
                    coordinates.scale_matrix(matrix, object.scale[0], object.scale[1], object.scale[2], fp);
            }
        }
        if (channels == 2) {
            const auto &object = motion.object(id);
            if ((object.evaluation_flags & 4U) == 0)
                coordinates.scale_matrix(coordinates.current_matrix(), object.scale[0],
                                          object.scale[1], object.scale[2], fp);
        }
        state.matrices.at(node) = coordinates.current_matrix();
        ++node;
        auto child = motion.object(id).child;
        while (child != 0)
            child = visit(child);
        coordinates.end_frame();
        return motion.object(id).sibling;
    }
};
} // namespace

bool calculate(State &state, NinjaMatrixPrimitives &coordinates, MotionSource &motion,
               const NinjaMatrix *base, float frame, std::uint32_t requested_node,
               NinjaMatrix *destination, fidelity::Context &fp) {
    state.count = 0;
    state.limit = std::min(requested_node + 1U, std::uint32_t{128});
    coordinates.push_current_frame();
    if (base)
        coordinates.current_matrix() = *base;
    const auto type = motion.type();
    const auto period = motion.frame_count();
    const auto integer_frame = static_cast<std::uint32_t>(ExactFloat(fp, frame).truncated_integer());
    if (period == 0)
        throw Error(Failure::ZeroFrameCount);
    const auto elapsed = (integer_frame / period) * period;
    const float wrapped = (ExactFloat(fp, frame) - unsigned_value(fp, elapsed)).rounded();
    unsigned channels = 0;
    auto remaining = std::uint32_t(type);
    for (unsigned bit = 0; remaining != 0 && bit < 5; ++bit) {
        ++channels;
        remaining &= ~(1U << bit);
    }
    Scan scanner{state, coordinates, motion, fp, period, wrapped, channels, 0};
    // Root siblings are deliberately ignored. The recursive parent alone loops.
    (void)scanner.visit(motion.root());
    if (destination) {
        // Original quirk: the requested index becomes a 32-bit byte offset.
        // Large unsigned indices can wrap back into the stored matrix array.
        const std::uint32_t byte_offset = requested_node * std::uint32_t(sizeof(NinjaMatrix));
        const auto slot = byte_offset / sizeof(NinjaMatrix);
        if (slot >= state.matrices.size())
            throw Error(Failure::MissingMatrixStorage);
        *destination = state.matrices.at(slot);
    }
    coordinates.end_frame();
    return true;
}
} // namespace sadx::model_matrices
