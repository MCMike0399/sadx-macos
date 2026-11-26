#pragma once
#include "player/player_vertex_weld.hpp"
#include "vertex_weld_fixture.hpp"
#include <array>
#include <cstring>
#include <vector>

namespace weld_active {
using namespace sadx;
constexpr unsigned kOutputWords = 1024;
constexpr unsigned kMaxEvents = 64;
constexpr unsigned kEventWords = 8;
enum Input : unsigned {
    kMode = 0,
    kVertexCount = 1,
    kJoinMode = 2,
    kAnimationMode = 3,
    kRequestedAnimation = 4,
    kFrame = 5,
    kActionRoot = 6,
    kRowObject = 7,
    kRowSource = 8,
    kRowDestination = 9,
    kSourceDepth = 10,
    kDestinationDepth = 11,
    kSourceNodeResult = 12,
    kDestinationNodeResult = 13,
    kNodeCount = 14,
    kMatrixCount = 15,
    kGeometry = 32,
    kBackup = 80,
    kPairs = 104,
    kInputWords = 112
};
enum Output : unsigned {
    kResult = 0,
    kRowMode = 1,
    kRowCount = 2,
    kRowSourceDepth = 3,
    kRowDestinationDepth = 4,
    kEventCount = 5,
    kBackupOut = 8,
    kGeometryOut = 32,
    kAttachmentOut = 80,
    kMatrixOut = 116,
    kEventOut = 180
};
struct Event {
    std::array<std::uint32_t, kEventWords> words{};
};
inline float value(std::uint32_t word) {
    float result;
    std::memcpy(&result, &word, sizeof(result));
    return result;
}
inline std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
inline NinjaMatrix initial_matrix() {
    NinjaMatrix matrix{};
    for (unsigned i = 0; i < matrix.size(); ++i)
        matrix[i] = static_cast<float>(i + 1);
    return matrix;
}
inline NinjaMatrix pattern(std::uint32_t index) {
    NinjaMatrix matrix{};
    for (unsigned i = 0; i < matrix.size(); ++i)
        matrix[i] = static_cast<float>((i + 1) * 10 + index);
    matrix[12] = static_cast<float>(1000 + index);
    matrix[13] = static_cast<float>(2000 + index);
    matrix[14] = static_cast<float>(3000 + index);
    return matrix;
}

class ScriptedMotionMatrices final : public vertex_weld::MotionMatrices {
  public:
    ScriptedMotionMatrices(character_pack::SceneState &scene, const std::uint32_t *input,
                           std::vector<Event> &events)
        : scene_(scene), input_(input), events_(events) {}
    void calculate(float frame, std::uint32_t node_count) override {
        log(0, {bits(frame), node_count, input_[kActionRoot]});
        count_ = node_count;
        for (std::uint32_t index = 0; index < count_ && index < instances_.size(); ++index)
            instances_[index] = pattern(index);
    }
    bool get(std::uint32_t index, NinjaMatrix &destination) override {
        const bool found = index < count_;
        log(1, {index, found ? 1U : 0U});
        if (found)
            destination = instances_[index];
        return found;
    }
    int node_index(const character_pack::ObjectResource *root,
                   const character_pack::ObjectResource *target) override {
        const auto root_index = index_of(root);
        const bool source = target == weld_fixture::object(scene_, input_[kRowSource]);
        const auto result = source ? static_cast<std::int32_t>(input_[kSourceNodeResult])
                                   : static_cast<std::int32_t>(input_[kDestinationNodeResult]);
        log(2, {root_index, source ? 1U : 2U, static_cast<std::uint32_t>(result)});
        return result;
    }
    const character_pack::ObjectResource *action_object() override {
        return weld_fixture::object(scene_, input_[kActionRoot]);
    }

  private:
    std::uint32_t index_of(const character_pack::ObjectResource *object) {
        for (unsigned index = 0; index < 3; ++index)
            if (object == weld_fixture::object(scene_, index + 1))
                return index + 1;
        return 0;
    }
    void log(std::uint32_t tag, std::initializer_list<std::uint32_t> args) {
        if (events_.size() >= kMaxEvents)
            return;
        Event event;
        event.words[0] = tag;
        unsigned slot = 1;
        for (const auto value : args)
            if (slot < kEventWords)
                event.words[slot++] = value;
        events_.push_back(event);
    }
    character_pack::SceneState &scene_;
    const std::uint32_t *input_;
    std::vector<Event> &events_;
    std::array<NinjaMatrix, 128> instances_{};
    std::uint32_t count_ = 0;
};

class ScriptedOperations final : public vertex_weld::PrimitiveOperations {
  public:
    explicit ScriptedOperations(std::vector<Event> &events) : events_(events) {
        stack_.push_back(initial_matrix());
    }
    void set_matrix(NinjaMatrix &destination, const NinjaMatrix &source) override {
        destination = source;
        log(3);
    }
    void invert_matrix(NinjaMatrix &matrix) override {
        for (auto &element : matrix)
            element = -element;
        log(4);
    }
    void multiply_current(const NinjaMatrix &source) override {
        auto &current = current_matrix();
        for (unsigned i = 0; i < current.size(); ++i)
            current[i] += source[i];
        log(5);
    }
    bool push_matrix(const NinjaMatrix &matrix) override {
        stack_.push_back(matrix);
        log(6, {static_cast<std::uint32_t>(stack_.size())});
        return true;
    }
    void pop_matrix() override {
        if (stack_.size() > 1)
            stack_.pop_back();
        log(7, {static_cast<std::uint32_t>(stack_.size())});
    }
    NinjaMatrix &current_matrix() override { return stack_.back(); }
    void calculate_point(const NinjaMatrix &matrix, const NinjaVector &input,
                         NinjaVector &output) override {
        log(8, {bits(matrix[12]), bits(matrix[13]), bits(matrix[14]), bits(input.x),
                bits(input.y), bits(input.z)});
        const auto step = static_cast<float>(calls_++);
        output = {input.x + matrix[12] + step, input.y + matrix[13] + step,
                  input.z + matrix[14] + step};
    }
    void calculate_vector(const NinjaMatrix &matrix, const NinjaVector &input,
                          NinjaVector &output) override {
        log(9, {bits(matrix[0]), bits(matrix[1]), bits(matrix[2]), bits(input.x), bits(input.y),
                bits(input.z)});
        const auto step = static_cast<float>(calls_++);
        output = {input.x + matrix[0] + step, input.y + matrix[1] + step,
                  input.z + matrix[2] + step};
    }
    void unit_vector(NinjaVector &vector) override {
        log(10, {bits(vector.x), bits(vector.y), bits(vector.z)});
        const auto step = static_cast<float>(calls_);
        vector = {vector.x + 7.0f + step, vector.y + 8.0f + step, vector.z + 9.0f + step};
    }

  private:
    void log(std::uint32_t tag, std::initializer_list<std::uint32_t> args = {}) {
        if (events_.size() >= kMaxEvents)
            return;
        Event event;
        event.words[0] = tag;
        unsigned slot = 1;
        for (const auto value : args)
            if (slot < kEventWords)
                event.words[slot++] = value;
        events_.push_back(event);
    }
    std::vector<Event> &events_;
    std::vector<NinjaMatrix> stack_;
    unsigned calls_ = 0;
};

inline void write_output(std::uint32_t *output,
                         const PlayerJoinVertex &row, const TaskWorkArena &arena,
                         const character_pack::SceneState &scene,
                         const PlayerAttachmentVectors &attachments,
                         const vertex_weld::Matrices &matrices,
                         const std::vector<Event> &events, std::uint32_t result) {
    output[kResult] = result;
    output[kRowMode] = static_cast<std::uint8_t>(row.input_mode);
    output[kRowCount] = row.vertex_count;
    output[kRowSourceDepth] = row.source_depth;
    output[kRowDestinationDepth] = row.destination_depth;
    output[kEventCount] = static_cast<std::uint32_t>(events.size());
    if (row.original_vertices) {
        std::memcpy(output + kBackupOut, arena.source_bytes().data() + row.original_vertices->data_offset(),
                    24 * sizeof(std::uint32_t));
    }
    for (unsigned resource = 0; resource < 4; ++resource) {
        const auto &words = scene.geometry.vector_words(10 + resource);
        std::memcpy(output + kGeometryOut + resource * 12, words.data(),
                    12 * sizeof(std::uint32_t));
    }
    const std::array<const NinjaVector *, 12> attachment_values{
        &attachments.right_hand_position, &attachments.left_hand_position,
        &attachments.right_foot_position, &attachments.left_foot_position,
        &attachments.user0_position,      &attachments.user1_position,
        &attachments.right_hand_direction, &attachments.left_hand_direction,
        &attachments.right_foot_direction, &attachments.left_foot_direction,
        &attachments.user0_direction,      &attachments.user1_direction};
    for (unsigned pair = 0; pair < attachment_values.size(); ++pair) {
        output[kAttachmentOut + pair * 3] = bits(attachment_values[pair]->x);
        output[kAttachmentOut + pair * 3 + 1] = bits(attachment_values[pair]->y);
        output[kAttachmentOut + pair * 3 + 2] = bits(attachment_values[pair]->z);
    }
    const std::array<const NinjaMatrix *, 4> matrix_values{&matrices.source, &matrices.destination,
                                                           &matrices.source_inverse,
                                                           &matrices.destination_inverse};
    for (unsigned matrix = 0; matrix < matrix_values.size(); ++matrix)
        for (unsigned index = 0; index < 16; ++index)
            output[kMatrixOut + matrix * 16 + index] = bits((*matrix_values[matrix])[index]);
    for (unsigned event = 0; event < events.size() && event < kMaxEvents; ++event)
        std::memcpy(output + kEventOut + event * kEventWords, events[event].words.data(),
                    kEventWords * sizeof(std::uint32_t));
}
} // namespace weld_active
