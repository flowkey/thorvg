#pragma once
#define THORVG_VERSION_STRING "1.0.0"
#define THORVG_CPU_ENGINE_SUPPORT 1
#define THORVG_TTF_LOADER_SUPPORT 1
#define THORVG_FILE_IO_SUPPORT 1
// ARM64 NEON — enabled unconditionally for Apple platforms
#if defined(__aarch64__) || defined(__arm64__)
#define THORVG_NEON_VECTOR_SUPPORT 1
#endif
