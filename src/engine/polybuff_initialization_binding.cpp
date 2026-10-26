#include "engine/polybuff_initialization.hpp"

namespace sadx::polybuff {
std::uint32_t configured_count(Context &context, std::size_t index) {
    return configured_counts(context).values.at(index);
}
BufferDescription buffer_description(Context &, std::size_t index) {
    return buffer_descriptions().at(index);
}
void initialize_record(Context &context, BufferState &buffer, std::size_t, std::uint32_t count,
                       BufferDescription description) {
    initialize(context, &buffer, count, description.stride, description.vertex_format,
               description.debug_name);
}
} // namespace sadx::polybuff
