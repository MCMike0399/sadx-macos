#include "player/gamma_shadow.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}

struct Fake final : sadx::gamma_shadow::Services {
    bool missed = false;
    float height = -1000000.0f;
    std::int32_t normal_z = 0;
    std::int32_t normal_x = 0;
    float asked_y = 0;
    std::vector<std::string> calls;
    float translate_y = 0;
    float scale_x = 0, scale_y = 0, scale_z = 0;
    float clip = 0;
    std::uint16_t yaw_angle = 0;
    sadx::gamma_shadow::Ground sample_ground(float, float y, float) override {
        asked_y = y;
        return {height, normal_z, normal_x};
    }
    bool missed_frames() override { return missed; }
    void prepare() override { calls.push_back("prepare"); }
    void push_matrix() override { calls.push_back("push"); }
    void translate(float, float y, float) override {
        translate_y = y;
        calls.push_back("translate");
    }
    void rotate_z(std::uint16_t) override { calls.push_back("rotz"); }
    void rotate_x(std::uint16_t) override { calls.push_back("rotx"); }
    void rotate_y(std::uint16_t angle) override {
        yaw_angle = angle;
        calls.push_back("roty");
    }
    void scale(float x, float y, float z) override {
        scale_x = x;
        scale_y = y;
        scale_z = z;
        calls.push_back("scale");
    }
    void draw(float value) override {
        clip = value;
        calls.push_back("draw");
    }
    void pop_matrix() override { calls.push_back("pop"); }
};

float run(const sadx::gamma_shadow::Body &body, float scale, Fake &services) {
    sadx::fidelity::Context arithmetic(0x007f);
    return sadx::gamma_shadow::update(body, scale, services, arithmetic).rounded();
}
} // namespace

int main() {
    {
        Fake services;
        services.missed = true;
        const auto height = run({400.0f, 0.0f, -20.0f, 16384}, 1.5f, services);
        require(services.calls.empty(), "missed frames do not draw");
        require(services.asked_y == 4.0f, "the probe is four units above the body");
        require(height == services.height, "the height is returned");
    }
    {
        Fake services;
        services.height = 0.0f;
        const auto height = run({400.0f, 0.0f, -20.0f, 0}, 1.5f, services);
        require(height == 0.0f, "a zero height is returned");
        require(services.calls.front() == "prepare" && services.calls.back() == "pop", "draw order");
        require(services.yaw_angle == 0 && services.scale_x == 1.5f, "a flat body keeps the scale");
        require(services.scale_y == 1.2f && services.clip == 1.5f, "the blob is flattened and the clip is the factor");
    }
    {
        Fake services;
        run({400.0f, 0.0f, -20.0f, 16384}, 1.5f, services);
        require(services.yaw_angle == 49152, "yaw 16384 turns to 49152");
    }
    {
        Fake services;
        services.normal_z = 7;
        services.normal_x = 9;
        run({1.0f, 2.0f, 3.0f, 0}, 1.5f, services);
        const auto z = std::find(services.calls.begin(), services.calls.end(), "rotz");
        const auto x = std::find(services.calls.begin(), services.calls.end(), "rotx");
        require(z != services.calls.end() && x != services.calls.end() && z < x, "normal z turns before normal x");
    }
    std::cout << "Gamma shadow: empty height, missed frames and blob scale passed\n";
    return 0;
}
