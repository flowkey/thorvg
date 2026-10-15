#pragma once
#define THORVG_VERSION_STRING "1.1.3"
#define THORVG_CPU_ENGINE_SUPPORT 1
#define THORVG_PARTIAL_RENDER_SUPPORT 1
#define THORVG_SFNT_LOADER_SUPPORT 1
#define THORVG_TTF_LOADER_SUPPORT 1
#define THORVG_OTF_LOADER_SUPPORT 1
#define THORVG_LOTTIE_LOADER_SUPPORT 1
#define THORVG_CAPI_BINDING_SUPPORT 1
#define THORVG_FILE_IO_SUPPORT 1

// The task scheduler (worker threads inside ThorVG) is off: callers parallelise by drawing
// one canvas per thread. THORVG_ENABLE_THREAD_SUPPORT, set by the TaskScheduler trait, turns
// it on where the C++ library has threads; single-threaded WASI leaves __STDCPP_THREADS__ undefined.
#if defined(THORVG_ENABLE_THREAD_SUPPORT) && defined(__STDCPP_THREADS__)
#define THORVG_THREAD_SUPPORT 1
#endif

// NEON is part of the AArch64 baseline. AVX2 is not enabled: it needs -mavx2 on
// tvgSwRasterSimd.cpp only, which SwiftPM cannot express per file.
#if defined(__aarch64__) || defined(__arm64__)
#define THORVG_NEON_SUPPORT 1
#endif
