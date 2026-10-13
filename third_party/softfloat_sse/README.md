# Scoped SSE arithmetic closure

Berkeley SoftFloat 3e, revision `a0c6494cdc11865811dec815d5c0049fba9d82a8`, BSD 3-clause terms retained in COPYING.txt and source headers. `upstream.json` pins all copied bytes. The SSE arithmetic entry points compile against the upstream 8086-SSE specialization, independently of the original x87 8086 specialization.

This closure supplies only absent arithmetic/NaN symbols. It reuses the sibling softfloat archive's format-only converters, common-NaN conversions, generic rounding/normalization helpers, exception raising and thread-local environment. The F32/F64 common-NaN converter and raiseFlags bytes were compared and are identical in upstream 8086 and 8086-SSE. NaN propagation is deliberately different and lives here. It does not supply duplicate state or x87 propagation.

Build with `sh third_party/softfloat_sse/build.sh OUTPUT_DIRECTORY`. Link the resulting softfloat-sse.a before the existing core/softfloat archive. The build manifest does not include existing helpers. Build integration is separately owned; this script never writes the shared build directory unless explicitly passed that destination.
