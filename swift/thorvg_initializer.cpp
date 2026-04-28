// thorvg_initializer.cpp
// Patched copy of thorvg/src/renderer/tvgInitializer.cpp that omits
// the global operator new/delete overrides (which conflict with the host program).

#include "tvgCommon.h"
#include "tvgTaskScheduler.h"
#include "tvgLoaderMgr.h"

#ifdef THORVG_CPU_ENGINE_SUPPORT
    #include "tvgSwRenderer.h"
#endif

namespace tvg {
    int engineInit = 0;
}

static uint16_t _version = 0;

static bool _buildVersionInfo(uint32_t* major, uint32_t* minor, uint32_t* micro)
{
    auto VER = THORVG_VERSION_STRING;
    auto p = VER;
    const char* x;

    if (!(x = strchr(p, '.'))) return false;
    uint32_t majorVal = atoi(p);
    p = x + 1;

    if (!(x = strchr(p, '.'))) return false;
    uint32_t minorVal = atoi(p);
    p = x + 1;

    uint32_t microVal = atoi(p);

    char sum[7];
    snprintf(sum, sizeof(sum), "%d%02d%02d", majorVal, minorVal, microVal);
    _version = atoi(sum);

    if (major) *major = majorVal;
    if (minor) *minor = minorVal;
    if (micro) *micro = microVal;

    return true;
}

Result Initializer::init(uint32_t threads) noexcept
{
    if (engineInit++ > 0) return Result::Success;

    if (!_buildVersionInfo(nullptr, nullptr, nullptr)) return Result::Unknown;

    if (!LoaderMgr::init()) return Result::Unknown;

    TaskScheduler::init(threads);

    return Result::Success;
}

Result Initializer::term() noexcept
{
    if (engineInit == 0) return Result::InsufficientCondition;

    if (--engineInit > 0) return Result::Success;

    #ifdef THORVG_CPU_ENGINE_SUPPORT
        if (!SwRenderer::term()) return Result::InsufficientCondition;
    #endif

    TaskScheduler::term();

    if (!LoaderMgr::term()) return Result::Unknown;

    return Result::Success;
}

const char* Initializer::version(uint32_t* major, uint32_t* minor, uint32_t* micro) noexcept
{
    if ((!major && !minor && !micro) || _buildVersionInfo(major, minor, micro)) return THORVG_VERSION_STRING;
    return nullptr;
}

uint16_t THORVG_VERSION_NUMBER()
{
    return _version;
}
