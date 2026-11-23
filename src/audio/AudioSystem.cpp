// miniaudio-backed audio system for FOWE.
// Tries the platform's native backend first, then falls back to the null
// backend so the engine still runs on headless/CI machines without a mixer.
#include "AudioSystem.h"

#include "miniaudio.h"

#include <cstdio>

namespace fowe {
namespace {

ma_engine  gEngine;
ma_context gOwnContext;
bool       gInitialized     = false;
bool       gOwnsContext     = false;
char       gBackendName[32] = "none";
uint32_t   gSampleRate      = 0;

} // namespace

bool AudioInit()
{
    if (gInitialized)
        return true;

    ma_result result = ma_engine_init(nullptr, &gEngine);
    if (result != MA_SUCCESS)
    {
        ma_backend backends[] = { ma_backend_null };
        ma_context_config contextConfig = ma_context_config_init();
        if (ma_context_init(backends, 1, &contextConfig, &gOwnContext) == MA_SUCCESS)
        {
            ma_engine_config engineConfig = ma_engine_config_init();
            engineConfig.pContext = &gOwnContext;
            result = ma_engine_init(&engineConfig, &gEngine);
            gOwnsContext = (result == MA_SUCCESS);
        }
    }

    if (result != MA_SUCCESS)
    {
        std::snprintf(gBackendName, sizeof(gBackendName), "unavailable");
        return false;
    }

    gInitialized = true;

    ma_device* pDevice = ma_engine_get_device(&gEngine);
    if (pDevice && pDevice->pContext)
    {
        const char* pName = ma_get_backend_name(pDevice->pContext->backend);
        std::snprintf(gBackendName, sizeof(gBackendName), "%s", pName ? pName : "unknown");
        gSampleRate = pDevice->sampleRate;
    }
    return true;
}

void AudioShutdown()
{
    if (!gInitialized)
        return;

    ma_engine_uninit(&gEngine);
    if (gOwnsContext)
    {
        ma_context_uninit(&gOwnContext);
        gOwnsContext = false;
    }
    gInitialized = false;
}

const char* AudioBackendName() { return gBackendName; }
uint32_t    AudioSampleRate() { return gSampleRate; }

} // namespace fowe
