#pragma once
namespace sadx::save {
// NOTSA: a published filename pointer borrows its actual native allocation.
// The backing allocation may be released independently before the source clears
// or replaces this pointer; native resource ownership stays with its caller.
struct SaveNameState {
    using TextPointer = const char *;
    // GLOBAL: SONIC 0x3b290dc SaveName size=4
    TextPointer value = nullptr;
};
} // namespace sadx::save
