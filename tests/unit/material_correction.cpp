#include "engine/material_correction.hpp"
#include <cassert>
#include <vector>

namespace sadx::material_correction {
enum class Event { Get, Load, Multiply, Store, Power, Set };
struct Observer final : fidelity::ExactObserver {
    std::vector<Event> &events;
    explicit Observer(std::vector<Event> &values) : events(values) {}
    void before(fidelity::ExactEvent event) override {
        switch (event) {
        case fidelity::ExactEvent::Load:
            events.push_back(Event::Load);
            break;
        case fidelity::ExactEvent::Multiply:
            events.push_back(Event::Multiply);
            break;
        case fidelity::ExactEvent::Store:
            events.push_back(Event::Store);
            break;
        default:
            assert(false);
        }
    }
};
struct Context {
    X87Context fp{0x007f};
    std::vector<Event> events;
    Observer observer{events};
    graphics::Material device_material;
};
graphics::Material get_material(Context &context) {
    context.events.push_back(Event::Get);
    return context.device_material;
}
void set_material(Context &context, const graphics::Material &material) {
    context.events.push_back(Event::Set);
    context.device_material = material;
}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.fp, value, &context.observer};
}
float specular_power(Context &context) {
    context.events.push_back(Event::Power);
    // A callback observes the source device material, before publication of the local copy.
    assert(context.device_material.diffuse[0] == 255.0f);
    assert(context.device_material.ambient[0] == 255.0f);
    context.device_material.emissive[0] = 42.0f;
    return 8.0f;
}
} // namespace sadx::material_correction

int main() {
    using namespace sadx::material_correction;
    Context context;
    context.device_material.diffuse = {255, 255, 255, 0.25f};
    context.device_material.ambient = {255, 255, 255, 0.5f};
    context.device_material.specular = {255, 255, 255, 0.75f};
    context.device_material.emissive = {3, 4, 5, 6};
    correct_material(context);
    std::vector<Event> expected{Event::Get, Event::Load, Event::Multiply, Event::Power,
                                Event::Store};
    for (unsigned channel = 0; channel < 8; ++channel) {
        expected.push_back(Event::Load);
        expected.push_back(Event::Multiply);
        expected.push_back(Event::Store);
    }
    expected.push_back(Event::Set);
    assert(context.events == expected);
    assert(context.device_material.diffuse[3] == 0.25f);
    assert(context.device_material.ambient[3] == 0.5f);
    assert(context.device_material.specular[3] == 0.75f);
    assert((context.device_material.emissive == std::array<float, 4>{3, 4, 5, 6}));
    assert(context.device_material.power == 8.0f);
}
