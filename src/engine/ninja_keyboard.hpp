#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
namespace sadx {
// Ninja keyboard record (4 at 3ce6e38, 12 bytes): +0 modifier bits, +1 byte,
// +2..+7 held key codes in press order, +8 pointer to a 4-byte word at 3ce6e68+4i.
struct NinjaKeyboardRecord {
    std::optional<std::uint8_t> modifiers,byte1;
    std::array<std::optional<std::uint8_t>,6> keys;
    std::optional<std::uint32_t> pointer;
};
// Native owner of the DirectInput8 keyboard device (3d08820). The source
// keeps a COM pointer; only its presence is a source fact here.
struct NinjaKeyboardDevice {
    // GetDeviceState(256): HRESULT and, when it is DI_OK (0), the DIK state.
    std::function<std::pair<std::int32_t,std::array<std::uint8_t,256>>()> get_device_state;
    std::function<std::int32_t()> acquire;
};
// 0xed entries at 389d918: u16 kind (0 key code, 1 modifier bit, else none) and a byte.
struct NinjaKeyboardTable {std::array<std::uint16_t,0xed> kind;std::array<std::uint8_t,0xed> code;};
std::optional<NinjaKeyboardTable> decode_ninja_keyboard_table(const std::uint8_t*,std::size_t);
// Ninja frame-callback list 3d0fa40 with count 3ce712c (77ffc0 appends, 780030 runs).
struct NinjaCallbackList {std::optional<std::uint32_t> count_at_3ce712c;std::vector<std::optional<std::uint32_t>> slots_at_3d0fa40;};
struct NinjaKeyboardState {
    std::shared_ptr<NinjaKeyboardDevice> device_at_3d08820; // null = 0
    bool device_known=false;                                 // load-image or 791b10 store observed
    std::optional<std::uint32_t> records_at_3d08818,count_at_3d0881c;
    std::array<NinjaKeyboardRecord,4> records_at_3ce6e38;
    std::array<std::array<std::optional<std::uint8_t>,4>,4> words_at_3ce6e68;
    std::optional<std::uint32_t> input_pointer_at_3b0e340; // KeyboardInputPointer
};
enum class NinjaKeyboardFailure {none,unknown_fact,unsupported_records,missing_owner};
struct NinjaKeyboardResult {
    NinjaKeyboardFailure failure=NinjaKeyboardFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::uint32_t acquire_calls=0;
    explicit operator bool()const noexcept{return failure==NinjaKeyboardFailure::none;}
};
// Actual 791bd0 (77f090's tail jump): poll the keyboard into every record.
NinjaKeyboardResult ninja_keyboard_poll_791bd0(NinjaKeyboardState&,const NinjaKeyboardTable&);
// DirectInput8::CreateDevice(GUID_SysKeyboard) then SetDataFormat(c_dfDIKeyboard)
// and SetCooperativeLevel(hwnd 3d0fd30, 0xa): the device or null.
using NinjaKeyboardCreate=std::function<std::shared_ptr<NinjaKeyboardDevice>()>;
// Actual 77f0a0: record init, 791b10(3ce6e38, 4), then 77ffc0(77f090). Under the pinned Input mod
// (input-mod.cpp Init: WriteCall 77f0d7 -> CreateSADXKeyboard, bytes `mov eax,[1000e068]; mov [eax],
// 1000e944`), the 791b10 call stores 3b0e340 = 0x1000e944 (its SADXKeyboard) instead: no device,
// record pointer, count or 3ce6e68 words are written.
enum class NinjaKeyboardProfile {original,input_mod};
constexpr std::uint32_t kInputModSadxKeyboard=0x1000e944;
NinjaKeyboardResult ninja_keyboard_init_77f0a0(NinjaKeyboardState&,NinjaCallbackList&,const NinjaKeyboardCreate&,NinjaKeyboardProfile=NinjaKeyboardProfile::original);
// Actual 77ffc0: append without a bound (the source has none).
NinjaKeyboardResult ninja_callback_register_77ffc0(NinjaCallbackList&,std::uint32_t callback);
} // namespace sadx
