#ifndef SADX_SOFTFLOAT_PLATFORM_H
#define SADX_SOFTFLOAT_PLATFORM_H

// Native ARM64/Intel macOS integer implementation with x87 specialization.
// All source and clients must include this configuration before softfloat.h.
#if !defined(__BYTE_ORDER__) || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error This pinned SoftFloat build requires a little-endian host.
#endif
#define LITTLEENDIAN 1
#define SOFTFLOAT_FAST_INT64 1
#define INLINE inline
#define INLINE_LEVEL 0
// Match C/C++ linkage on Darwin. C++ thread_local emits wrapper symbols absent
// from C's _Thread_local definitions. Clang's __thread is supported in both
// languages and preserves per-thread storage with identical external symbols.
#define THREAD_LOCAL __thread

#endif
