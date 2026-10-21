#include "runtime/native_display.hpp"

namespace sadx::native {
void Display::update_selection_camera(selection_camera::Pose pose) {
    // NOTSA: borrows the display's single view/matrix owners for the reached task callback.
    struct CameraCalls final : selection_camera::Calls {
        Display &display;
        explicit CameraCalls(Display &value) : display(value) {}
        void set_camera(view::Camera &) override { display.set_camera(); }
    } calls(*this);
    selection_camera::update(pose, view_.camera, *coordinates_, calls, arithmetic_);
}
} // namespace sadx::native
