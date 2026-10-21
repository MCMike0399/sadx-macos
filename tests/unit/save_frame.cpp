#include "game/save_frame.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>
namespace {
struct Fixture final : sadx::save::FrameCalls {
    std::uint8_t ready = 0, count = 0, exec = 0, pending = 0;
    std::int32_t phase = 0;
    std::uint32_t pause = 0;
    sadx::menu_caption::Context context;
    std::vector<int> order;
    bool completed = false, fail_message = false, fail_write = false, fail_close = false;
    sadx::save::FrameStateView view() {
        return {ready, count, phase, exec, pending, pause, context};
    }
    void display_saving_message(sadx::menu_caption::Context &value) override {
        assert(&value == &context && phase == 1 && exec == 1);
        order.push_back(1);
        if (fail_message)
            throw std::runtime_error("message boundary");
        pending = 1;
    }
    void write_save_file() override {
        assert(completed && exec == 0);
        order.push_back(3);
        if (fail_write)
            throw std::runtime_error("write boundary");
        phase = 0;
        ready = 0;
        count = 0;
    }
    void close_saving_message(sadx::menu_caption::Context &value) override {
        assert(&value == &context && pending != 0);
        order.push_back(4);
        if (fail_close)
            throw std::runtime_error("close boundary");
        pause = 7; // The caller must reload this actual live owner after the callback.
    }
    void frame_completion() {
        assert(!completed);
        order.push_back(2);
        completed = true;
    }
};
template <class F> void fails(F operation) {
    bool failed = false;
    try {
        operation();
    } catch (const std::runtime_error &) {
        failed = true;
    }
    assert(failed);
}
} // namespace
int main() {
    Fixture normal;
    normal.ready = 1;
    normal.count = 255;
    normal.pause = 2;
    sadx::save::before_frame_completion(normal.view(), normal);
    assert(normal.count == 0 && normal.phase == 1 && normal.exec == 1 && normal.pending == 1 &&
           !normal.completed);
    normal.frame_completion();
    sadx::save::after_frame_completion(normal.view(), normal);
    assert(normal.order == std::vector<int>({1, 2, 3, 4}));
    assert(normal.exec == 0 && normal.pending == 0 && normal.phase == 0 && normal.pause == 7);
    Fixture message;
    message.ready = 1;
    message.count = 4;
    message.fail_message = true;
    fails([&] { sadx::save::before_frame_completion(message.view(), message); });
    assert(message.count == 5 && message.exec == 1 && message.phase == 1 && message.pending == 0);
    Fixture write;
    write.phase = 1;
    write.exec = 1;
    write.pending = 1;
    write.completed = true;
    write.fail_write = true;
    fails([&] { sadx::save::after_frame_completion(write.view(), write); });
    assert(write.order == std::vector<int>({3}) && write.exec == 0 && write.pending == 1 &&
           write.phase == 1);
    Fixture close;
    close.pending = 1;
    close.pause = 2;
    close.fail_close = true;
    fails([&] { sadx::save::after_frame_completion(close.view(), close); });
    assert(close.pending == 1 && close.pause == 2);
    Fixture pause;
    pause.pause = 2;
    sadx::save::after_frame_completion(pause.view(), pause);
    assert(pause.pause == 1 && pause.order.empty());
}
