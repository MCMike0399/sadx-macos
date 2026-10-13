#pragma once
#include <cstdint>
#include <stdexcept>
#include <thread>
namespace sadx {
struct Sse32 { std::uint32_t bits; };
struct Sse64 { std::uint64_t bits; };
enum class SseComparison { less, equal, greater, unordered };
enum class SseFailure { reserved_mxcsr, cross_thread_context, unmasked_exception };
class SseError : public std::runtime_error {
public:
    SseError(SseFailure failure, std::uint8_t exceptions, const char* message)
        : std::runtime_error(message), failure_(failure), exceptions_(exceptions) {}
    SseFailure failure() const noexcept { return failure_; }
    std::uint8_t exceptions() const noexcept { return exceptions_; }
private: SseFailure failure_; std::uint8_t exceptions_;
};
// Caller supplies a captured/selected MXCSR. Low six incoming sticky flags are
// retained. Scalar operations use explicit binary words, never host FP state.
// A thrown exception commits status, but does not produce/store a destination.
class SseContext {
public:
    explicit SseContext(std::uint32_t mxcsr);
    SseContext(const SseContext&) = delete;
    SseContext& operator=(const SseContext&) = delete;
    std::uint32_t mxcsr() const;
    void set_mxcsr(std::uint32_t mxcsr);
    void clear_exceptions();
    Sse32 from_int32(std::int32_t value);
    Sse32 add(Sse32 destination, Sse32 source);
    Sse32 subtract(Sse32 destination, Sse32 source);
    Sse32 multiply(Sse32 destination, Sse32 source);
    Sse32 divide(Sse32 destination, Sse32 source);
    Sse64 to_double(Sse32 source);
    Sse32 to_single(Sse64 source);
    Sse64 sqrt(Sse64 source);
    Sse64 subtract_double(Sse64 destination, Sse64 source); // SUBSD
    bool less_than(Sse64 destination, Sse64 source); // signaling CMPLTPD low lane
    std::int32_t truncate_int32(Sse32 source); // CVTTSS2SI, no #D
    SseComparison comiss(Sse32 destination, Sse32 source);
    SseComparison ucomisd(Sse64 destination, Sse64 source);
    Sse32 minimum(Sse32 destination, Sse32 source); // legacy MINSS
private:
    void check_thread() const;
    void commit(std::uint8_t flags);
    Sse32 binary(Sse32 a, Sse32 b, unsigned operation);
    std::uint32_t mxcsr_;
    std::thread::id owner_;
};
} // namespace sadx
