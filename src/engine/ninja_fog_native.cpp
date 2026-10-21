#include "engine/ninja_fog_internal.hpp"

namespace sadx::fog {
namespace {
// NOTSA: native composition reads owned values and submits typed device commands.
class NativeContext final : public Context {
  public:
    explicit NativeContext(Runtime &runtime)
        : Context(runtime.arithmetic), runtime_(runtime), state_(runtime.state),
          values_(state_.values), display_switch_(state_.display_switch) {}
    NativeContext(Runtime &runtime, const std::int32_t &pending_logic_ticks)
        : Context(runtime.arithmetic), runtime_(runtime), state_(runtime.state),
          values_(state_.values), display_switch_(pending_logic_ticks) {}
    void device_load() override {}
    void require_device() override {
        if (!runtime_.render)
            fail(ErrorKind::MissingDevice);
    }
    void require_table() override {}
    void render(RenderState state, std::uint32_t value) override {
        require_device();
        runtime_.render(state, value);
    }
    std::uint32_t raster_caps() override { return runtime_.raster_caps; }
    std::uint32_t device_flags() override { return runtime_.device_flags; }
    std::uint32_t color() override { return values_.device.color; }
    void color(std::uint32_t value) override { values_.device.color = value; }
    std::uint32_t start_index() override { return values_.device.start_index; }
    void start_index(std::uint32_t value) override { values_.device.start_index = value; }
    std::uint32_t end_index() override { return values_.device.end_index; }
    void end_index(std::uint32_t value) override { values_.device.end_index = value; }
    float start_factor() override { return values_.device.start_value; }
    void start_factor(float value) override { values_.device.start_value = value; }
    float end_factor() override { return values_.device.end_value; }
    void end_factor(float value) override { values_.device.end_value = value; }
    void software_density(std::uint32_t value) override {
        values_.device.software_density_bits = value;
    }
    float depth(unsigned index) override { return values_.device.w_depth.at(index); }
    void depth(unsigned index, float value) override { values_.device.w_depth.at(index) = value; }
    float reciprocal(unsigned index) override {
        if (index >= values_.device.reciprocal.size())
            fail(ErrorKind::TableIndex);
        return values_.device.reciprocal[index];
    }
    void reciprocal(unsigned index, float value) override {
        values_.device.reciprocal.at(index) = value;
    }
    float sample(unsigned index) override { return values_.table.at(index); }
    void sample(unsigned index, float value) override { values_.table.at(index) = value; }
    std::uint32_t enabled() override { return values_.enabled; }
    void enabled(std::uint32_t value) override { values_.enabled = value; }
    std::uint32_t supported() override { return values_.table_supported; }
    void supported(std::uint32_t value) override { values_.table_supported = value; }
    void mode(std::uint32_t value) override { values_.mode = value; }
    std::int32_t display_switch() override { return display_switch_; }
    std::uint8_t profile() override { return state_.profile; }
    std::uint8_t special_flags() override { return state_.special_flags; }
    float cached_start() override { return values_.cached_start; }
    void cached_start(float value) override { values_.cached_start = value; }
    float cached_end() override { return values_.cached_end; }
    void cached_end(float value) override { values_.cached_end = value; }
    void active(std::uint8_t value) override { state_.active = value; }
    float sentinel() override { return -65535.0F; }
    float level_start() override { return values_.level.start_z; }
    float level_end() override { return values_.level.end_z; }
    std::uint32_t level_color() override { return values_.level.color; }

  private:
    Runtime &runtime_;
    State &state_;
    NinjaFogInitialState &values_;
    const std::int32_t &display_switch_;
};
} // namespace
void initialize(Runtime &runtime) {
    NativeContext context(runtime);
    initialize(context);
}
void set_color(Runtime &runtime, std::uint32_t value) {
    NativeContext context(runtime);
    set_color(context, value);
}
void generate_table(Runtime &runtime, float start, float end) {
    NativeContext context(runtime);
    generate_table(context, start, end);
}
void set_table(Runtime &runtime) {
    NativeContext context(runtime);
    set_table(context);
}
void set_enabled(Runtime &runtime, bool enabled) {
    NativeContext context(runtime);
    set_enabled(context, enabled);
}
void set_enabled(Runtime &runtime, bool enabled, const std::int32_t &pending_logic_ticks) {
    NativeContext context(runtime, pending_logic_ticks);
    set_enabled(context, enabled);
}
} // namespace sadx::fog
