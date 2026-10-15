/*
 * Copyright (c) 2026 the ThorVG project. All rights reserved.

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

#include <thorvg.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>
#include "config.h"
#include "catch.hpp"

//The tests below drive ThorVG from several application threads at once, so they need std::thread
//even when ThorVG itself is built without its task scheduler.
#if defined(THORVG_CPU_ENGINE_SUPPORT) && defined(__STDCPP_THREADS__)

#include <thread>

using namespace tvg;
using namespace std;

static constexpr uint32_t CANVAS_SIZE = 256;
static constexpr uint32_t APPLICATION_THREADS = 8;
static constexpr uint32_t ROUNDS = 12;

using Pixels = vector<uint32_t>;

//Fills, strokes, dashes, gradients, a mask and a blur: every stage that uses the engine's scratch memory.
static void buildScene(Canvas* canvas, uint32_t variant)
{
    auto scene = Scene::gen();

    for (uint32_t shapeIndex = 0; shapeIndex < 24; ++shapeIndex) {
        auto shape = Shape::gen();
        auto offset = float((shapeIndex * 37 + variant * 11) % 200);
        shape->moveTo(offset, 10.0f);
        shape->cubicTo(offset + 80.0f, 0.0f, 250.0f - offset, 120.0f, 60.0f + offset * 0.5f, 240.0f);
        shape->lineTo(10.0f, 200.0f - offset * 0.3f);
        shape->close();
        shape->appendCircle(128.0f, 128.0f, 20.0f + float(shapeIndex * 3), 30.0f + float(variant * 2));
        shape->fill(uint8_t(shapeIndex * 10), uint8_t(255 - variant * 20), uint8_t(shapeIndex * 5 + variant), 160);
        shape->strokeWidth(1.0f + float((shapeIndex + variant) % 5));
        shape->strokeFill(20, 40, uint8_t(shapeIndex * 9), 255);
        shape->strokeJoin(shapeIndex % 2 ? StrokeJoin::Round : StrokeJoin::Miter);
        if (shapeIndex % 3 == 0) {
            float dashPattern[] = {6.0f, 3.0f + float(variant)};
            shape->strokeDash(dashPattern, 2);
        }
        scene->add(shape);
    }

    auto gradientShape = Shape::gen();
    gradientShape->appendRect(20.0f, 20.0f, 200.0f, 120.0f, 30.0f, 30.0f);
    auto gradient = LinearGradient::gen();
    gradient->linear(20.0f, 20.0f, 220.0f, 140.0f);
    Fill::ColorStop colorStops[] = {{0.0f, 255, 0, 0, 255}, {0.5f, 0, uint8_t(variant * 30), 255, 128}, {1.0f, 0, 255, 0, 255}};
    gradient->colorStops(colorStops, 3);
    gradientShape->fill(gradient);
    auto mask = Shape::gen();
    mask->appendCircle(120.0f, 80.0f, 70.0f + float(variant), 50.0f);
    mask->fill(255, 255, 255, 200);
    gradientShape->mask(mask, MaskMethod::Alpha);
    scene->add(gradientShape);

    auto blurred = Scene::gen();
    auto blurredShape = Shape::gen();
    blurredShape->appendRect(140.0f, 150.0f, 90.0f, 80.0f);
    blurredShape->fill(10, 10, 10, 255);
    blurred->add(blurredShape);
    blurred->add(SceneEffect::GaussianBlur, 3.0 + double(variant % 3), 0, 0, 100);
    scene->add(blurred);

    canvas->add(scene);
}

static bool render(SwCanvas* canvas, uint32_t variant, Pixels& pixels)
{
    pixels.assign(CANVAS_SIZE * CANVAS_SIZE, 0);
    if (canvas->target(pixels.data(), CANVAS_SIZE, CANVAS_SIZE, CANVAS_SIZE, ColorSpace::ARGB8888) != Result::Success) return false;
    buildScene(canvas, variant);
    auto drawn = canvas->draw(true) == Result::Success && canvas->sync() == Result::Success;
    canvas->remove();
    return drawn;
}

static vector<Pixels> renderReferences()
{
    vector<Pixels> references(APPLICATION_THREADS);
    auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
    for (uint32_t variant = 0; variant < APPLICATION_THREADS; ++variant) {
        REQUIRE(render(canvas.get(), variant, references[variant]));
    }
    return references;
}

//Runs body(threadIndex) on every application thread at the same time and counts the failed rounds.
template<typename Body>
static uint32_t runConcurrently(Body body)
{
    atomic<uint32_t> failures{0};
    atomic<uint32_t> waiting{APPLICATION_THREADS};
    vector<thread> threads;
    for (uint32_t threadIndex = 0; threadIndex < APPLICATION_THREADS; ++threadIndex) {
        threads.emplace_back([&, threadIndex] {
            --waiting;
            while (waiting > 0) this_thread::yield();
            for (uint32_t round = 0; round < ROUNDS; ++round) {
                if (!body(threadIndex)) ++failures;
            }
        });
    }
    for (auto& worker : threads) worker.join();
    return failures;
}

TEST_CASE("Concurrent rendering of canvases created on one thread", "[tvgConcurrency]")
{
    for (uint32_t workerThreads : {0u, 4u}) {
        REQUIRE(Initializer::init(workerThreads) == Result::Success);
        {
            auto references = renderReferences();

            //The canvases come from this thread, but each one is drawn by a different thread.
            vector<unique_ptr<SwCanvas>> canvases;
            for (uint32_t threadIndex = 0; threadIndex < APPLICATION_THREADS; ++threadIndex) {
                canvases.emplace_back(SwCanvas::gen());
            }

            auto failures = runConcurrently([&](uint32_t threadIndex) {
                Pixels pixels;
                if (!render(canvases[threadIndex].get(), threadIndex, pixels)) return false;
                return pixels == references[threadIndex];
            });
            REQUIRE(failures == 0);
        }
        REQUIRE(Initializer::term() == Result::Success);
    }
}

TEST_CASE("Concurrent rendering of canvases created on their own threads", "[tvgConcurrency]")
{
    for (uint32_t workerThreads : {0u, 4u}) {
        REQUIRE(Initializer::init(workerThreads) == Result::Success);
        {
            auto references = renderReferences();

            auto failures = runConcurrently([&](uint32_t threadIndex) {
                auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
                Pixels pixels;
                if (!canvas || !render(canvas.get(), threadIndex, pixels)) return false;
                return pixels == references[threadIndex];
            });
            REQUIRE(failures == 0);
        }
        REQUIRE(Initializer::term() == Result::Success);
    }
}

TEST_CASE("Rendering after another thread terminated the engine", "[tvgConcurrency]")
{
    REQUIRE(Initializer::init(0) == Result::Success);
    auto references = renderReferences();

    //The last reference is released by another thread, so the engine terminates there.
    thread([] { Initializer::init(0); }).join();
    REQUIRE(Initializer::term() == Result::Success);
    thread([] { Initializer::term(); }).join();

    //This thread rendered before the restart and renders again after it.
    REQUIRE(Initializer::init(0) == Result::Success);
    {
        auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
        REQUIRE(canvas);
        for (uint32_t variant = 0; variant < APPLICATION_THREADS; ++variant) {
            Pixels pixels;
            REQUIRE(render(canvas.get(), variant, pixels));
            REQUIRE(pixels == references[variant]);
        }
    }
    REQUIRE(Initializer::term() == Result::Success);
}


//Pictures that borrow the same pixels share one cached loader across all canvases.
static void buildPictureScene(Canvas* canvas, const vector<uint32_t>& image, uint32_t imageSize, uint32_t variant)
{
    for (uint32_t pictureIndex = 0; pictureIndex < 6; ++pictureIndex) {
        auto picture = Picture::gen();
        picture->load(image.data(), imageSize, imageSize, ColorSpace::ARGB8888, false);
        picture->translate(float(pictureIndex * 30 + variant * 3), float(pictureIndex * 20));
        picture->scale(0.5f + 0.25f * float((pictureIndex + variant) % 4));
        canvas->add(picture);
    }
}

TEST_CASE("Concurrent loading of a shared image", "[tvgConcurrency]")
{
    constexpr uint32_t imageSize = 64;
    vector<uint32_t> image(imageSize * imageSize);
    for (uint32_t pixelIndex = 0; pixelIndex < image.size(); ++pixelIndex) {
        auto x = pixelIndex % imageSize;
        auto y = pixelIndex / imageSize;
        image[pixelIndex] = 0xff000000 | ((x * 4) << 16) | ((y * 4) << 8) | ((x ^ y) & 0xff);
    }

    auto renderPictures = [&](SwCanvas* canvas, uint32_t variant, Pixels& pixels) {
        pixels.assign(CANVAS_SIZE * CANVAS_SIZE, 0);
        if (canvas->target(pixels.data(), CANVAS_SIZE, CANVAS_SIZE, CANVAS_SIZE, ColorSpace::ARGB8888) != Result::Success) return false;
        buildPictureScene(canvas, image, imageSize, variant);
        auto drawn = canvas->draw(true) == Result::Success && canvas->sync() == Result::Success;
        canvas->remove();
        return drawn;
    };

    for (uint32_t workerThreads : {0u, 4u}) {
        REQUIRE(Initializer::init(workerThreads) == Result::Success);
        {
            vector<Pixels> references(APPLICATION_THREADS);
            {
                auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
                for (uint32_t variant = 0; variant < APPLICATION_THREADS; ++variant) {
                    REQUIRE(renderPictures(canvas.get(), variant, references[variant]));
                }
            }

            auto failures = runConcurrently([&](uint32_t threadIndex) {
                auto canvas = unique_ptr<SwCanvas>(SwCanvas::gen());
                Pixels pixels;
                if (!canvas || !renderPictures(canvas.get(), threadIndex, pixels)) return false;
                return pixels == references[threadIndex];
            });
            REQUIRE(failures == 0);
        }
        REQUIRE(Initializer::term() == Result::Success);
    }
}

#endif
