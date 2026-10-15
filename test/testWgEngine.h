/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TVG_TEST_WG_ENGINE_H_
#define _TVG_TEST_WG_ENGINE_H_

#include "config.h"

#if defined(THORVG_WG_TEST_SUPPORT)

#include <thorvg.h>
#include <webgpu/webgpu.h>
#if __has_include(<webgpu/wgpu.h>)
    #include <webgpu/wgpu.h>
    #define TVG_TEST_WGPU_NATIVE 1
#endif
#include <cstring>
#include <vector>

using namespace tvg;

struct TestWgEngine
{
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUTexture texture = nullptr;
    WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm;
    uint32_t width;
    uint32_t height;
    ColorSpace colorSpace;

    TestWgEngine(uint32_t w = 100, uint32_t h = 100, ColorSpace cs = ColorSpace::ABGR8888, WGPUTextureFormat textureFormat = WGPUTextureFormat_BGRA8Unorm) : format(textureFormat), width(w), height(h), colorSpace(cs)
    {
        instance = wgpuCreateInstance(nullptr);

        // request adapter
        auto onAdapterRequestEnded = [](WGPURequestAdapterStatus, WGPUAdapter adapter, WGPUStringView, void* userdata1, void*) {
            *((WGPUAdapter*)userdata1) = adapter;
        };
        WGPURequestAdapterOptions requestAdapterOptions = {};
        requestAdapterOptions.featureLevel = WGPUFeatureLevel_Compatibility;
        requestAdapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;

        WGPURequestAdapterCallbackInfo requestAdapterCallback = {};
        requestAdapterCallback.mode = WGPUCallbackMode_WaitAnyOnly;
        requestAdapterCallback.callback = onAdapterRequestEnded;
        requestAdapterCallback.userdata1 = &adapter;
        wgpuInstanceRequestAdapter(instance, &requestAdapterOptions, requestAdapterCallback);

        // request device
        auto onDeviceError = [](WGPUDevice const*, WGPUErrorType, WGPUStringView, void*, void*) {
        };
        auto onDeviceRequestEnded = [](WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView, void* userdata1, void*) {
            *((WGPUDevice*)userdata1) = device;
        };
        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.label.data = "ThorVG Test Device";
        deviceDesc.label.length = WGPU_STRLEN;
        deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;

        WGPURequestDeviceCallbackInfo requestDeviceCallback = {};
        requestDeviceCallback.mode = WGPUCallbackMode_WaitAnyOnly;
        requestDeviceCallback.callback = onDeviceRequestEnded;
        requestDeviceCallback.userdata1 = &device;
        wgpuAdapterRequestDevice(adapter, &deviceDesc, requestDeviceCallback);

        // create texture
        WGPUTextureDescriptor textureDesc = {};
        textureDesc.usage = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size.width = width;
        textureDesc.size.height = height;
        textureDesc.size.depthOrArrayLayers = 1;
        textureDesc.format = format;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;

        texture = wgpuDeviceCreateTexture(device, &textureDesc);
    }

    ~TestWgEngine()
    {
        wgpuTextureDestroy(texture);
        wgpuTextureRelease(texture);
        wgpuDeviceDestroy(device);
        wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter);
        wgpuInstanceRelease(instance);
    }

    Result target(WgCanvas* canvas)
    {
        return canvas->target({instance, adapter, device}, texture, width, height, colorSpace, 1);
    }

    // Copies the target texture (or another texture of the same size) to the CPU, one uint32_t per
    // pixel in the texture's byte order. Returns an empty vector when the read-back buffer cannot be mapped.
    std::vector<uint32_t> readPixels(WGPUTexture sourceTexture = nullptr)
    {
        auto stride = (width * 4 + 255) & ~255u;
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        bufferDesc.size = uint64_t(stride) * height;
        auto buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

        auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        WGPUTexelCopyTextureInfo source = {};
        source.texture = sourceTexture ? sourceTexture : texture;
        source.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo destination = {};
        destination.buffer = buffer;
        destination.layout.bytesPerRow = stride;
        destination.layout.rowsPerImage = height;
        WGPUExtent3D size = {width, height, 1};
        wgpuCommandEncoderCopyTextureToBuffer(encoder, &source, &destination, &size);
        auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
        auto queue = wgpuDeviceGetQueue(device);
        wgpuQueueSubmit(queue, 1, &commands);

        auto status = 0;
        WGPUBufferMapCallbackInfo mapCallback = {};
        mapCallback.mode = WGPUCallbackMode_AllowProcessEvents;
        mapCallback.callback = [](WGPUMapAsyncStatus mapStatus, WGPUStringView, void* userdata1, void*) {
            *((int*)userdata1) = (mapStatus == WGPUMapAsyncStatus_Success) ? 1 : -1;
        };
        mapCallback.userdata1 = &status;
        wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, bufferDesc.size, mapCallback);
        while (status == 0) {
#ifdef TVG_TEST_WGPU_NATIVE
            wgpuDevicePoll(device, true, nullptr);
#endif
            wgpuInstanceProcessEvents(instance);
        }

        std::vector<uint32_t> pixels;
        if (status > 0) {
            pixels.resize(width * height);
            auto data = (const uint8_t*)wgpuBufferGetConstMappedRange(buffer, 0, bufferDesc.size);
            for (uint32_t y = 0; y < height; ++y) memcpy(pixels.data() + y * width, data + y * stride, width * 4);
            wgpuBufferUnmap(buffer);
        }
        wgpuBufferRelease(buffer);
        wgpuQueueRelease(queue);
        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(encoder);
        return pixels;
    }
};

#endif

#endif
