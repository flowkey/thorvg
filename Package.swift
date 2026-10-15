// swift-tools-version: 6.2
import PackageDescription

let packageDirectory = String(#filePath.dropLast("/Package.swift".count))

/// Header search paths of the ThorVG implementation, relative to the package root.
let implementationHeaderDirectories = [
    "swift",
    "inc",
    "src/common",
    "src/renderer",
    "src/renderer/cpu_engine",
    "src/renderer/gpu_engine",
    "src/renderer/gpu_engine/gl",
    "src/loaders/raw",
    "src/loaders/sfnt",
    "src/loaders/lottie",
    "src/loaders/lottie/rapidjson",
    "src/bindings/capi",
]

/// The implementation header search paths, rewritten relative to a target directory.
func headerSearchPaths(fromTargetDirectory targetDirectory: String) -> [CXXSetting] {
    let depth = targetDirectory == "." ? 0 : targetDirectory.split(separator: "/").count
    let pathToRoot = depth == 0 ? "" : String(repeating: "../", count: depth)
    return implementationHeaderDirectories.map { .headerSearchPath(pathToRoot + $0) }
}

/// Settings shared by every target that compiles ThorVG sources.
func implementationSettings(targetDirectory: String) -> [CXXSetting] {
    [
        .define("TVG_STATIC"),
        // ThorVG would otherwise replace operator new/delete for the whole host program.
        .define("THORVG_NO_GLOBAL_OPERATOR_NEW"),
        .define("THORVG_GL_ENGINE_SUPPORT", to: "1", .when(traits: ["OpenGL"])),
        .unsafeFlags(["-fno-exceptions", "-fno-rtti", "-w"]),
    ] + headerSearchPaths(fromTargetDirectory: targetDirectory)
}

let package = Package(
    name: "ThorVGPackage",
    platforms: [.iOS("15.6"), .macOS(.v13)],
    products: [
        // ThorVG's C++ API (namespace `tvg`) for Swift C++ interoperability.
        .library(name: "ThorVGCpp", targets: ["ThorVGCpp"]),
        // ThorVG's C API (`tvg_*` functions), importable without C++ interoperability.
        .library(name: "ThorVGC", targets: ["ThorVGC"]),
    ],
    traits: [
        .trait(
            name: "OpenGL",
            description: "Adds the OpenGL engine (tvg::GlCanvas). libGL is loaded at runtime with dlopen, so there is no link-time dependency."
        ),
    ],
    targets: [
        .target(
            name: "ThorVGCpp",
            dependencies: [
                .target(name: "ThorVGGLEngine", condition: .when(traits: ["OpenGL"])),
            ],
            path: ".",
            sources: [
                "src/common/tvgCompressor.cpp",
                "src/common/tvgMath.cpp",
                "src/common/tvgStr.cpp",
                "src/renderer/tvgAccessor.cpp",
                "src/renderer/tvgAnimation.cpp",
                "src/renderer/tvgCanvas.cpp",
                "src/renderer/tvgFill.cpp",
                "src/renderer/tvgInitializer.cpp",
                "src/renderer/tvgLoaderMgr.cpp",
                "src/renderer/tvgPaint.cpp",
                "src/renderer/tvgPicture.cpp",
                "src/renderer/tvgRender.cpp",
                "src/renderer/tvgSaver.cpp",
                "src/renderer/tvgScene.cpp",
                "src/renderer/tvgShape.cpp",
                "src/renderer/tvgTaskScheduler.cpp",
                "src/renderer/tvgText.cpp",
                "src/renderer/cpu_engine/tvgSwBlendOp.cpp",
                "src/renderer/cpu_engine/tvgSwFill.cpp",
                "src/renderer/cpu_engine/tvgSwImage.cpp",
                "src/renderer/cpu_engine/tvgSwMemPool.cpp",
                "src/renderer/cpu_engine/tvgSwPostEffect.cpp",
                "src/renderer/cpu_engine/tvgSwRaster.cpp",
                "src/renderer/cpu_engine/tvgSwRasterCore.cpp",
                "src/renderer/cpu_engine/tvgSwRasterSimd.cpp",
                "src/renderer/cpu_engine/tvgSwRenderer.cpp",
                "src/renderer/cpu_engine/tvgSwRle.cpp",
                "src/renderer/cpu_engine/tvgSwShape.cpp",
                "src/renderer/cpu_engine/tvgSwStroke.cpp",
                "src/renderer/cpu_engine/tvgSwUtil.cpp",
                "src/loaders/raw/tvgRawLoader.cpp",
                "src/loaders/sfnt/tvgSfntLoader.cpp",
                "src/loaders/sfnt/tvgSfntReader.cpp",
                "src/loaders/sfnt/tvgTtfReader.cpp",
                "src/loaders/sfnt/tvgOtfReader.cpp",
                "src/loaders/lottie/tvgLottieAnimation.cpp",
                "src/loaders/lottie/tvgLottieBuilder.cpp",
                "src/loaders/lottie/tvgLottieExpressions.cpp",
                "src/loaders/lottie/tvgLottieInterpolator.cpp",
                "src/loaders/lottie/tvgLottieLoader.cpp",
                "src/loaders/lottie/tvgLottieModel.cpp",
                "src/loaders/lottie/tvgLottieModifier.cpp",
                "src/loaders/lottie/tvgLottieParser.cpp",
                "src/loaders/lottie/tvgLottieParserHandler.cpp",
                "src/loaders/lottie/tvgLottieProperty.cpp",
            ],
            publicHeadersPath: "swift/include",
            cxxSettings: implementationSettings(targetDirectory: ".")
        ),

        .target(
            name: "ThorVGC",
            dependencies: ["ThorVGCpp"],
            path: "swift/ThorVGC",
            cxxSettings: implementationSettings(targetDirectory: "swift/ThorVGC")
        ),

        .target(
            name: "ThorVGGLEngine",
            path: "src/renderer/gpu_engine",
            exclude: ["meson.build", "gl/meson.build", "wg"],
            publicHeadersPath: "gl",
            cxxSettings: implementationSettings(targetDirectory: "src/renderer/gpu_engine") + [
                .define("THORVG_GL_ENGINE_SUPPORT", to: "1"),
                .define("THORVG_GL_TARGET_GL", to: "1"),
            ]
        ),

        // ThorVG's Catch2 unit tests for the engines and loaders built above:
        // swift run -c release ThorVGTests
        .executableTarget(
            name: "ThorVGTests",
            dependencies: ["ThorVGCpp"],
            path: "test",
            exclude: [
                "meson.build",
                "regression",
                "resources",
                // The GL engine defines global function pointers named after the GL entry points
                // (glGenFramebuffers, ...). Linked statically next to OpenGL they shadow the real
                // functions, so the GL tests cannot run in this package.
                "testGlCanvas.cpp",
                "testGlEngine.cpp",
                "testWgCanvas.cpp",
                "testWgEngine.cpp",
            ],
            cxxSettings: [
                .define("TVG_STATIC"),
                .define("TEST_DIR", to: "\"\(packageDirectory)/test/resources\""),
                .headerSearchPath("../swift"),
                .headerSearchPath("../inc"),
                .headerSearchPath("../src/loaders/lottie"),
                .unsafeFlags(["-w"]),
            ],
            linkerSettings: [
                .linkedLibrary("pthread", .when(platforms: [.linux])),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx17
)
