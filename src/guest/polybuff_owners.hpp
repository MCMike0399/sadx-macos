#pragma once
#include "cpu/source_bytes.hpp"
#include "engine/polybuff_initialization.hpp"
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace sadx::polybuff_reference {
// NOTSA: a reference fact has availability, but its value always lives in the canonical owner.
// Pointer conversions here represent guest identities only; no host object is dereferenced.
struct Word {
    using Target = std::variant<std::uint32_t *, void **, polybuff::DeviceBuffer **,
                                polybuff::BufferMemory **, std::uint32_t **, const char **>;
    Target target;
    bool available = false;
    explicit Word(std::uint32_t &v) : target(&v) {}
    explicit Word(void *&v) : target(&v) {}
    explicit Word(polybuff::DeviceBuffer *&v) : target(&v) {}
    explicit Word(polybuff::BufferMemory *&v) : target(&v) {}
    explicit Word(std::uint32_t *&v) : target(&v) {}
    explicit Word(const char *&v) : target(&v) {}
    Word(const Word &) = default;
    std::optional<std::uint32_t> get() const {
        if (!available) return {};
        return std::visit([](auto p) -> std::uint32_t {
            using T = std::remove_reference_t<decltype(*p)>;
            if constexpr (std::is_pointer_v<T>)
                return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(*p));
            else return *p;
        }, target);
    }
    operator std::optional<std::uint32_t>() const { return get(); }
    explicit operator bool() const { return available; }
    std::uint32_t operator*() const { return *get(); }
    std::uint32_t value_or(std::uint32_t fallback) const { return get().value_or(fallback); }
    Word &operator=(std::optional<std::uint32_t> value) {
        available = value.has_value();
        if (value) std::visit([&](auto p) {
            using T = std::remove_reference_t<decltype(*p)>;
            if constexpr (std::is_pointer_v<T>) *p = reinterpret_cast<T>(std::uintptr_t(*value));
            else *p = *value;
        }, target);
        return *this;
    }
    Word &operator=(std::uint32_t v) { return *this = std::optional<std::uint32_t>(v); }
    Word &operator=(const Word &v) { return *this = v.get(); }
    bool operator==(std::uint32_t v) const { return get() == v; }
};
struct Record {
    std::array<Word, 9> words;
    explicit Record(polybuff::BufferState &s)
        : words{{Word(s.resource), Word(s.capacity_bytes), Word(s.remaining_bytes), Word(s.stride),
                  Word(s.vertex_format), Word(s.render_arguments.words.data), Word(s.lock_count),
                  Word(s.debug_name), Word(s.unused)}} {}
    Word &operator[](std::size_t n) { return words[n]; }
    const Word &operator[](std::size_t n) const { return words[n]; }
    auto begin() { return words.begin(); }
    auto end() { return words.begin() + 4; }
    Record &operator=(std::array<std::optional<std::uint32_t>, 4> values) {
        for (std::size_t n = 0; n < 4; ++n) words[n] = values[n];
        return *this;
    }
};
struct FrameFacts {
    Word vertices;
    std::array<Record, 9> records;
    explicit FrameFacts(polybuff::Buffers &s) : vertices(s.vertices), records{{
        Record(s.records[0]), Record(s.records[1]), Record(s.records[2]),
        Record(s.records[3]), Record(s.records[4]), Record(s.records[5]),
        Record(s.records[6]), Record(s.records[7]), Record(s.records[8])}} {
        for (auto &record : s.records)
            record.render_arguments.words.kind = polybuff::DataKind::RenderArguments;
    }
};
// NOTSA: polygon consumers borrow either a live canonical-identity view or their existing
// local guest word. There is no cached pointer value, so callback mutations remain visible.
struct VertexScratchRead {
    std::variant<const std::optional<std::uint32_t> *, const Word *> target;
    VertexScratchRead(const std::optional<std::uint32_t> &v) : target(&v) {}
    VertexScratchRead(const Word &v) : target(&v) {}
    std::optional<std::uint32_t> get() const {
        return std::visit([](auto p) -> std::optional<std::uint32_t> { return *p; }, target);
    }
    explicit operator bool() const { return get().has_value(); }
    std::uint32_t operator*() const { return *get(); }
};
} // namespace sadx::polybuff_reference

namespace sadx {
// NOTSA: this specialization preserves the frozen fixtures' SourceBytes interface.
// In production it borrows NinjaDeviceState's Scratch; only standalone fixtures own a
// private Scratch. Availability remains per byte; the words themselves are never copied.
template<> struct SourceBytes<0x3d0fae0, 0x200> {
    static constexpr std::uint32_t base = 0x3d0fae0;
    static constexpr std::size_t size = 0x200;
    std::unique_ptr<polybuff::Scratch> isolated;
    polybuff::Scratch &owner;
    struct Byte {
        polybuff::Scratch *owner = nullptr;
        std::size_t offset = 0;
        bool available = false;
        Byte &operator=(std::optional<std::uint8_t> value) {
            available = value.has_value();
            if (value) {
                auto &word = owner->words[offset / 4];
                const auto shift = (offset % 4) * 8;
                word = (word & ~(255U << shift)) | (std::uint32_t(*value) << shift);
            }
            return *this;
        }
        Byte &operator=(std::uint8_t value) { return *this = std::optional<std::uint8_t>(value); }
        std::uint8_t value_or(std::uint8_t fallback) const {
            return available ? static_cast<std::uint8_t>(owner->words[offset / 4] >> ((offset % 4) * 8))
                             : fallback;
        }
        bool operator==(std::uint8_t value) const { return available && value_or(0) == value; }
    };
    std::array<Byte, size> bytes{};
    SourceBytes() : isolated(std::make_unique<polybuff::Scratch>()), owner(*isolated) { bind(); }
    explicit SourceBytes(polybuff::Scratch &s) : owner(s) { bind(); }
    void bind() {
        for (std::size_t n = 0; n < size; ++n) bytes[n] = Byte{&owner, n, false};
    }
    static bool contains(std::uint32_t address, std::size_t width) {
        return address >= base && width <= size && address - base <= size - width;
    }
    bool write(std::uint32_t address, std::uint64_t value, std::size_t width) {
        if (!contains(address, width)) return false;
        for (std::size_t n = 0; n < width; ++n)
            bytes[address - base + n] = static_cast<std::uint8_t>(value >> (8 * n));
        return true;
    }
    std::optional<std::uint64_t> read(std::uint32_t address, std::size_t width) const {
        if (!contains(address, width)) return {};
        std::uint64_t value = 0;
        for (std::size_t n = 0; n < width; ++n) {
            const auto &byte = bytes[address - base + n];
            if (!byte.available) return {};
            value |= std::uint64_t(byte.value_or(0)) << (8 * n);
        }
        return value;
    }
    bool fill(std::uint32_t address, std::size_t count, std::uint8_t value) {
        if (!contains(address, count)) return false;
        for (std::size_t n = 0; n < count; ++n) bytes[address - base + n] = value;
        return true;
    }
};
} // namespace sadx
