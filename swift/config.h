#pragma once
#define THORVG_VERSION_STRING "1.1.3"
#define THORVG_CPU_ENGINE_SUPPORT 1
#define THORVG_PARTIAL_RENDER_SUPPORT 1
#define THORVG_SFNT_LOADER_SUPPORT 1
#define THORVG_TTF_LOADER_SUPPORT 1
#define THORVG_OTF_LOADER_SUPPORT 1
#define THORVG_LOTTIE_LOADER_SUPPORT 1
#define THORVG_FILE_IO_SUPPORT 1

// The task scheduler needs std::thread. Toolchains whose C++ library has no threads
// (for example single-threaded WASI) leave __STDCPP_THREADS__ undefined.
// Define THORVG_NO_THREAD_SUPPORT to build the single-threaded variant anywhere.
#if defined(__STDCPP_THREADS__) && !defined(THORVG_NO_THREAD_SUPPORT)
#define THORVG_THREAD_SUPPORT 1
#endif

// NEON is part of the AArch64 baseline. AVX2 is not enabled: it needs -mavx2 on
// tvgSwRasterSimd.cpp only, which SwiftPM cannot express per file.
#if defined(__aarch64__) || defined(__arm64__)
#define THORVG_NEON_SUPPORT 1
#endif
