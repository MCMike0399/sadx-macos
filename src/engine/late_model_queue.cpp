#include "engine/late_model_queue.hpp"

#include <cstring>

namespace sadx {
namespace {
constexpr std::uint32_t kNodeBytes = 0x70;
constexpr std::uint32_t kHeaderOffset = 8;
constexpr std::uint32_t kListWordOffset = 12;
constexpr std::uint32_t kModelOffset = 0x2c;
constexpr std::uint32_t kMatrixOffset = 0x30;
constexpr std::uint32_t kMatrixBytes = 64;
constexpr std::uint8_t kRecordType = 1;
constexpr std::uint32_t kQueuePolicy = 0;
constexpr std::uint32_t kModelPointerFloor = 0x100000;
constexpr unsigned kTranslationZ = 14;

LateModelResult failed(NinjaQueueFailure failure, std::uint32_t site, NinjaQueueRecordHandle record = {}) {
    LateModelResult result;
    result.failure = failure;
    result.source_boundary = site;
    result.record = std::move(record);
    return result;
}

bool write_bytes(const NinjaQueueRecordHandle &record, std::uint32_t offset, const void *data,
                 std::uint32_t size, std::uint32_t site, LateModelResult &result) {
    const auto failure = record->arena->write(record->block, offset, data, size);
    if (failure == NinjaQueueFailure::none)
        return true;
    result = failed(failure, site, record);
    return false;
}

bool read_bytes(const NinjaQueueRecordHandle &record, std::uint32_t offset, void *data,
                std::uint32_t size) {
    return record && record->arena &&
           record->arena->read(record->block, offset, data, size) == NinjaQueueFailure::none;
}
} // namespace

LateModelResult late_draw_model(NinjaSpriteQueue &queue, const NinjaMatrix &matrix,
                                std::uint32_t model_word, std::uint8_t late_flags, float depth_limit,
                                std::function<void()> draw, NinjaSpriteQueueRuntime &runtime,
                                fidelity::Context &arithmetic) {
    auto allocated = queue.allocate_record(kNodeBytes, kRecordType, late_flags, 0, runtime);
    if (!allocated)
        return failed(allocated.failure, allocated.source_boundary, allocated.record);
    if (!allocated.record) {
        LateModelResult result;
        result.route = LateModelRoute::immediate;
        return result;
    }
    auto record = allocated.record;
    record->draw_model = std::move(draw);
    LateModelResult result;
    result.record = record;
    if (!write_bytes(record, kModelOffset, &model_word, sizeof(model_word), kModelOffset, result))
        return result;
    if (!write_bytes(record, kMatrixOffset, matrix.data(), kMatrixBytes, kMatrixOffset, result))
        return result;
    const auto extended = arithmetic.from_float(matrix[kTranslationZ]).negated();
    result.depth = arithmetic.to_float(extended);
    const auto comparison = arithmetic.compare(extended, arithmetic.from_float(depth_limit));
    const bool queue_node = comparison == X87Comparison::greater || (late_flags & 1) == 0;
    if (!queue_node) {
        result.route = LateModelRoute::immediate;
        return result;
    }
    auto inserted = queue.insert(record, result.depth, kQueuePolicy, runtime, arithmetic);
    if (!inserted)
        return failed(inserted.failure, inserted.source_boundary, record);
    result.route = LateModelRoute::queued;
    result.record = inserted.record;
    result.bucket = inserted.bucket;
    return result;
}

bool dispatch_queued_model(const NinjaQueueRecordHandle &record, bool simple_models,
                           std::int32_t lighting_palette,
                           const std::function<void(const NinjaQueueRecordHandle &, const NinjaMatrix &,
                                                    std::uint32_t, bool)> &draw) {
    std::uint8_t header = 0;
    if (!read_bytes(record, kHeaderOffset, &header, sizeof(header)) || (header & 15) != kRecordType)
        return false;
    std::uint32_t list_word = 0;
    std::uint32_t model_word = 0;
    if (!read_bytes(record, kListWordOffset, &list_word, sizeof(list_word)) ||
        !read_bytes(record, kModelOffset, &model_word, sizeof(model_word)))
        return false;
    if (list_word != 0 && model_word >= kModelPointerFloor && draw) {
        NinjaMatrix matrix{};
        if (!read_bytes(record, kMatrixOffset, matrix.data(), kMatrixBytes))
            return false;
        const bool simple = simple_models && lighting_palette <= 0;
        draw(record, matrix, model_word, simple);
    }
    return true;
}
} // namespace sadx
