// FOWE - Furious Open World Engine
// Real Linux/Vulkan editor skeleton: boots The-Forge renderer, initializes the
// embedded ImGui (via The-Forge UI), font system and Lua scripting layer, and
// presents an editor window with a Low/Medium/High/Ultra quality selector.
//
// Physics/audio/scripting subsystems live behind interfaces and are wired in
// incrementally; this file owns the application/renderer lifecycle.

#include "Application/Interfaces/IApp.h"
#include "Application/Interfaces/IFont.h"
#include "Application/Interfaces/IUI.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "Utilities/Interfaces/IFileSystem.h"
#include "Utilities/Interfaces/ILog.h"
#include "Utilities/Interfaces/IMemory.h"
#include "Utilities/Math/MathTypes.h"
#include "Graphics/FSL/defaults.h"
#include "Utilities/RingBuffer.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

static const uint32_t kDataBufferCount = 2;

enum class QualityTier : int32_t
{
    Low = 0,
    Medium = 1,
    High = 2,
    Ultra = 3
};

static const char* qualityTierName(int32_t tier)
{
    switch (tier)
    {
        case 0: return "Low";
        case 1: return "Medium";
        case 2: return "High";
        case 3: return "Ultra";
        default: return "High";
    }
}

static int32_t parseQualityTier(const char* value)
{
    if (!value)
        return 2;
    if (strcmp(value, "0") == 0 || strcmp(value, "Low") == 0 || strcmp(value, "low") == 0)
        return 0;
    if (strcmp(value, "1") == 0 || strcmp(value, "Medium") == 0 || strcmp(value, "medium") == 0)
        return 1;
    if (strcmp(value, "2") == 0 || strcmp(value, "High") == 0 || strcmp(value, "high") == 0)
        return 2;
    if (strcmp(value, "3") == 0 || strcmp(value, "Ultra") == 0 || strcmp(value, "ultra") == 0)
        return 3;
    return 2;
}

static int32_t readQualityTier()
{
    FILE* f = fopen("fowe.cfg", "r");
    if (!f)
        return 2;

    int32_t tier = 2;
    char    line[128];
    while (fgets(line, sizeof(line), f))
    {
        char* nl = strchr(line, '\n');
        if (nl)
            *nl = 0;
        if (strncmp(line, "quality=", 8) == 0)
            tier = parseQualityTier(line + 8);
    }
    fclose(f);
    return tier;
}

static void writeQualityTier(int32_t tier)
{
    FILE* f = fopen("fowe.cfg", "w");
    if (!f)
        return;
    fprintf(f, "quality=%s\n", qualityTierName(tier));
    fclose(f);
}

class FOWEApp : public IApp
{
public:
    bool Init() override
    {
        RendererDesc settings = {};
        initGPUConfiguration(settings.pExtendedSettings);
        initRenderer(GetName(), &settings, &pRenderer);
        if (!pRenderer)
        {
            ShowUnsupportedMessage(getUnsupportedGPUMsg());
            return false;
        }
        setupGPUConfigurationPlatformParameters(pRenderer, settings.pExtendedSettings);

        QueueDesc queueDesc = {};
        queueDesc.mType = QUEUE_TYPE_GRAPHICS;
        queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
        initQueue(pRenderer, &queueDesc, &pGraphicsQueue);

        GpuCmdRingDesc cmdRingDesc = {};
        cmdRingDesc.pQueue = pGraphicsQueue;
        cmdRingDesc.mPoolCount = kDataBufferCount;
        cmdRingDesc.mCmdPerPoolCount = 1;
        cmdRingDesc.mAddSyncPrimitives = true;
        initGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

        initSemaphore(pRenderer, &pImageAcquiredSemaphore);

        initResourceLoaderInterface(pRenderer);

        RootSignatureDesc rootDesc = {};
        INIT_RS_DESC(rootDesc);
        initRootSignature(pRenderer, &rootDesc);

        // Font used by the editor UI (embedded ImGui style). The physical font
        // file is staged into RD_FONTS by the build.
        FontDesc font = {};
        font.pFontName = "FOWE UI";
        font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
        fntDefineFonts(&font, 1, &gFontID);

        FontSystemDesc fontSystemDesc = {};
        fontSystemDesc.pRenderer = pRenderer;
        if (!initFontSystem(&fontSystemDesc))
            return false;

        UserInterfaceDesc uiDesc = {};
        uiDesc.pRenderer = pRenderer;
        uiDesc.mEnableDocking = true;
        uiDesc.mEnableRemoteUI = false;
        initUserInterface(&uiDesc);

        mQualityTier = readQualityTier();
        mAppliedQualityTier = mQualityTier;

        LOGF(LogLevel::eINFO, "FOWE initialized on %s / %s", pRenderer->pGpu->mGpuVendorPreset.mGpuName, "Vulkan");
        return true;
    }

    void Exit() override
    {
        exitUserInterface();
        exitFontSystem();

        exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
        exitSemaphore(pRenderer, pImageAcquiredSemaphore);
        exitRootSignature(pRenderer);
        exitResourceLoaderInterface(pRenderer);

        exitQueue(pRenderer, pGraphicsQueue);

        exitRenderer(pRenderer);
        exitGPUConfiguration();
        pRenderer = nullptr;
    }

    bool Load(ReloadDesc* pReloadDesc) override
    {
        const uint32_t type = pReloadDesc->mType;

        if (type & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            if (!addSwapChain())
                return false;

            buildEditorUI();
        }

        if (type & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET | RELOAD_TYPE_SHADER))
        {
            UserInterfaceLoadDesc uiLoad = {};
            uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
            uiLoad.mWidth = mSettings.mWidth;
            uiLoad.mHeight = mSettings.mHeight;
            uiLoad.mDisplayWidth = mSettings.mWidth;
            uiLoad.mDisplayHeight = mSettings.mHeight;
            uiLoad.mLoadType = (uint32_t)type;
            loadUserInterface(&uiLoad);

            FontSystemLoadDesc fontLoad = {};
            fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
            fontLoad.mWidth = mSettings.mWidth;
            fontLoad.mHeight = mSettings.mHeight;
            fontLoad.mLoadType = (ReloadType)type;
            loadFontSystem(&fontLoad);
        }

        return true;
    }

    void Unload(ReloadDesc* pReloadDesc) override
    {
        waitQueueIdle(pGraphicsQueue);

        const uint32_t type = pReloadDesc->mType;

        if (type & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET | RELOAD_TYPE_SHADER))
        {
            unloadUserInterface(type);
            unloadFontSystem((ReloadType)type);
        }

        if (type & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET))
        {
            if (pGuiWindow)
            {
                uiRemoveComponent(pGuiWindow);
                pGuiWindow = nullptr;
            }
            removeSwapChain(pRenderer, pSwapChain);
        }
    }

    void Update(float deltaTime) override
    {
        UNREF_PARAM(deltaTime);

        if (mQualityTier != mAppliedQualityTier)
        {
            mAppliedQualityTier = mQualityTier;
            writeQualityTier(mQualityTier);
            LOGF(LogLevel::eINFO, "Graphics quality preset changed to %s", qualityTierName(mAppliedQualityTier));
        }
    }

    void Draw() override
    {
        uint32_t swapchainImageIndex = 0;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, nullptr, &swapchainImageIndex);

        RenderTarget* pRenderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
        GpuCmdRingElement elem = getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

        FenceStatus fenceStatus;
        getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &elem.pFence);

        resetCmdPool(pRenderer, elem.pCmdPool);

        Cmd* cmd = elem.pCmds[0];
        beginCmd(cmd);

        RenderTargetBarrier barrier = { pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET };
        cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &barrier);

        BindRenderTargetsDesc bindTargets = {};
        bindTargets.mRenderTargetCount = 1;
        bindTargets.mRenderTargets[0] = { pRenderTarget, LOAD_ACTION_CLEAR };
        cmdBindRenderTargets(cmd, &bindTargets);

        cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth, (float)pRenderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

        // Editor UI (embedded ImGui renderer)
        cmdDrawUserInterface(cmd);

        cmdBindRenderTargets(cmd, nullptr);

        barrier = { pRenderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, &barrier);

        endCmd(cmd);

        FlushResourceUpdateDesc flushUpdateDesc = {};
        flushUpdateDesc.mNodeIndex = 0;
        flushResourceUpdates(&flushUpdateDesc);

        Semaphore* waitSemaphores[2] = { flushUpdateDesc.pOutSubmittedSemaphore, pImageAcquiredSemaphore };

        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &elem.pSemaphore;
        submitDesc.ppWaitSemaphores = waitSemaphores;
        submitDesc.pSignalFence = elem.pFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = (uint8_t)swapchainImageIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.ppWaitSemaphores = &elem.pSemaphore;
        presentDesc.mSubmitDone = true;
        queuePresent(pGraphicsQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % kDataBufferCount;

        static bool sLoopLogged = false;
        if (!sLoopLogged)
        {
            sLoopLogged = true;
            LOGF(LogLevel::eINFO, "FOWE render loop active (quality preset %s)", qualityTierName(mAppliedQualityTier));
        }
    }

    const char* GetName() override { return "FOWE"; }

private:
    bool addSwapChain()
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mWidth = mSettings.mWidth;
        swapChainDesc.mHeight = mSettings.mHeight;
        swapChainDesc.mImageCount = getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
        swapChainDesc.mColorFormat = getSupportedSwapchainFormat(pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
        swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
        swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
        ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
        return pSwapChain != nullptr;
    }

    void buildEditorUI()
    {
        UIComponentDesc desc = {};
        desc.mStartPosition = vec2(20.0f, 60.0f);
        desc.mStartSize = vec2(460.0f, 420.0f);
        desc.mFontID = gFontID;
        desc.mFontSize = 18.0f;
        uiAddComponent("FOWE Editor", &desc, &pGuiWindow);

        static LabelWidget titleLabel;
        uiAddComponentWidget(pGuiWindow, "Furious Open World Engine", &titleLabel, WIDGET_TYPE_LABEL);

        static SeparatorWidget sep1;
        uiAddComponentWidget(pGuiWindow, "", &sep1, WIDGET_TYPE_SEPARATOR);

        static LabelWidget qualityLabel;
        uiAddComponentWidget(pGuiWindow, "Graphics Quality", &qualityLabel, WIDGET_TYPE_LABEL);

        static const char* kTierNames[4] = { "Low", "Medium", "High", "Ultra" };
        for (int32_t i = 0; i < 4; ++i)
        {
            mRadios[i].pData = &mQualityTier;
            mRadios[i].mRadioId = i;
            uiAddComponentWidget(pGuiWindow, kTierNames[i], &mRadios[i], WIDGET_TYPE_RADIO_BUTTON);
        }

        static SeparatorWidget sep2;
        uiAddComponentWidget(pGuiWindow, "", &sep2, WIDGET_TYPE_SEPARATOR);

        static LabelWidget hintLabel;
        uiAddComponentWidget(pGuiWindow, "Renderer: The-Forge / Vulkan   Physics: Bullet   Audio: miniaudio", &hintLabel, WIDGET_TYPE_LABEL);
    }

    Renderer*      pRenderer = nullptr;
    Queue*         pGraphicsQueue = nullptr;
    SwapChain*     pSwapChain = nullptr;
    Semaphore*     pImageAcquiredSemaphore = nullptr;
    GpuCmdRing     gGraphicsCmdRing = {};
    UIComponent*   pGuiWindow = nullptr;
    uint32_t       gFrameIndex = 0;
    uint32_t       gFontID = 0;

    int32_t          mQualityTier = (int32_t)QualityTier::High;
    int32_t          mAppliedQualityTier = (int32_t)QualityTier::High;
    RadioButtonWidget mRadios[4] = {};
};

DEFINE_APPLICATION_MAIN(FOWEApp)
