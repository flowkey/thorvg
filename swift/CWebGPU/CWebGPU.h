// The WebGPU C API (webgpu/webgpu.h) of whichever implementation pkg-config `wgpu_native` points
// at, plus wgpu-native's extensions (wgpuDevicePoll, ...) when they are present.
#pragma once
#include <webgpu/webgpu.h>
#if __has_include(<webgpu/wgpu.h>)
#include <webgpu/wgpu.h>
#endif
