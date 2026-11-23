cmake_minimum_required(VERSION 3.20)
project(FOWE C CXX ASM)

# ---------------------------------------------------------------------------
# FOWE - Furious Open World Engine
# Builds The-Forge Common_3 (which ships no CMake) + FOWE editor executable.
# Target: Linux / Vulkan (dev environment). Other platforms add later.
# ---------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(FORGE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/The-Forge)
set(FORGE_COMMON ${FORGE_ROOT}/Common_3)
set(RUN_DIR ${CMAKE_BINARY_DIR}/run)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
find_package(X11 REQUIRED)
find_library(UDERV_LIB udev REQUIRED)
find_library(XRANDR_LIB Xrandr REQUIRED)
find_library(VULKAN_LIB NAMES libvulkan.so.1 vulkan.so.1 PATHS /usr/lib64 /lib64 /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu REQUIRED)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# The Forge Operating System / Application layer (mirrors SteamOS CodeLite OSBase)
# ---------------------------------------------------------------------------
set(OS_SOURCES
    ${FORGE_COMMON}/Application/Screenshot/Screenshot.cpp
    ${FORGE_COMMON}/Tools/ReloadServer/ReloadClient.cpp
    ${FORGE_COMMON}/Tools/Network/Network.c
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/AnimatedObject.cpp
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/Animation.cpp
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/Clip.cpp
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/ClipController.cpp
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/ClipMask.cpp
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/Rig.cpp
    ${FORGE_COMMON}/Resources/AnimationSystem/Animation/SkeletonBatcher.cpp
    ${FORGE_COMMON}/Application/CameraController.cpp
    ${FORGE_COMMON}/Utilities/Threading/ThreadSystem.c
    ${FORGE_COMMON}/Utilities/Math/Algorithms.c
    ${FORGE_COMMON}/Utilities/Math/StbDs.c
    ${FORGE_COMMON}/Utilities/Timer.c
    ${FORGE_COMMON}/OS/CPUConfig.cpp
    ${FORGE_COMMON}/OS/ThirdParty/OpenSource/hidapi/linux/hid.c
    ${FORGE_COMMON}/Utilities/Log/Log.c
    ${FORGE_COMMON}/Utilities/MemoryTracking/MemoryTracking.c
    ${FORGE_COMMON}/OS/Linux/LinuxInput.cpp
    ${FORGE_COMMON}/OS/Linux/LinuxTime.c
    ${FORGE_COMMON}/OS/Linux/LinuxLog.c
    ${FORGE_COMMON}/OS/Linux/LinuxBase.cpp
    ${FORGE_COMMON}/OS/Linux/LinuxFileSystem.c
    ${FORGE_COMMON}/OS/Linux/LinuxThread.c
    ${FORGE_COMMON}/OS/Linux/LinuxWindow.cpp
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/decompress/huf_decompress_amd64.S
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/decompress/zstd_decompress_block.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/decompress/zstd_decompress.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/decompress/zstd_ddict.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/decompress/huf_decompress.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/zstd_common.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/xxhash.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/threading.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/pool.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/fse_decompress.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/error_private.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/entropy_common.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/zstd/common/debug.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/lz4/lz4.c
    ${FORGE_COMMON}/Utilities/ThirdParty/OpenSource/bstrlib/bstrlib.c
    ${FORGE_COMMON}/Application/ThirdParty/OpenSource/imgui/imgui.cpp
    ${FORGE_COMMON}/Application/ThirdParty/OpenSource/imgui/imgui_demo.cpp
    ${FORGE_COMMON}/Application/ThirdParty/OpenSource/imgui/imgui_draw.cpp
    ${FORGE_COMMON}/Application/ThirdParty/OpenSource/imgui/imgui_widgets.cpp
    ${FORGE_COMMON}/Application/ThirdParty/OpenSource/imgui/imgui_tables.cpp
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lapi.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lauxlib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lbaselib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lbitlib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lcode.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lcorolib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lctype.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ldblib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ldebug.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ldo.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ldump.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lfunc.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lgc.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/linit.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/liolib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/llex.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lmathlib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lmem.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/loadlib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lobject.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lopcodes.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/loslib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lparser.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lstate.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lstring.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lstrlib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ltable.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ltablib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/ltm.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lundump.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lutf8lib.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lvm.c
    ${FORGE_COMMON}/Game/ThirdParty/OpenSource/lua-5.3.5/src/lzio.c
    ${FORGE_COMMON}/OS/ThirdParty/OpenSource/cpu_features/src/impl_x86_linux_or_android.c
    ${FORGE_COMMON}/OS/ThirdParty/OpenSource/cpu_features/src/impl_aarch64_linux_or_android.c
    ${FORGE_COMMON}/Utilities/FileSystem/FileSystem.c
    ${FORGE_COMMON}/Utilities/FileSystem/UnixFileSystem.c
    ${FORGE_COMMON}/Application/Profiler/GpuProfiler.cpp
    ${FORGE_COMMON}/Application/Profiler/ProfilerBase.cpp
    ${FORGE_COMMON}/Application/Fonts/FontSystem.cpp
    ${FORGE_COMMON}/Application/Fonts/stbtt.cpp
    ${FORGE_COMMON}/Application/UI/UI.cpp
    ${FORGE_COMMON}/Game/Scripting/LuaManager.cpp
    ${FORGE_COMMON}/Game/Scripting/LuaManagerImpl.cpp
    ${FORGE_COMMON}/Game/Scripting/LuaSystem.cpp
    ${FORGE_COMMON}/OS/WindowSystem/WindowSystem.cpp
)

add_library(ForgeOS STATIC ${OS_SOURCES})
set_target_properties(ForgeOS PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
target_include_directories(ForgeOS PUBLIC
    ${FORGE_COMMON}
    ${FORGE_COMMON}/Application/ThirdParty/OpenSource/imgui
    ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/volk
)
# VULKAN is defined by The-Forge VulkanConfig.h
target_compile_options(ForgeOS PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
    -w
)
target_include_directories(ForgeOS PRIVATE ${GTK3_INCLUDE_DIRS})
target_link_libraries(ForgeOS PUBLIC
    ${GTK3_LIBRARIES}
    ${X11_LIBRARIES}
    ${UDERV_LIB}
    ${XRANDR_LIB}
    ${CMAKE_DL_LIBS}
    Threads::Threads
    m
)

# ---------------------------------------------------------------------------
# The Forge Graphics / Renderer layer (mirrors SteamOS CodeLite Renderer)
# ---------------------------------------------------------------------------
set(RENDERER_SOURCES
    ${FORGE_COMMON}/Renderer/ParticleSystem/ParticleSystem.cpp
    ${FORGE_COMMON}/Renderer/VisibilityBuffer/VisibilityBuffer.cpp
    ${FORGE_COMMON}/Resources/ResourceLoader/ResourceLoader.cpp
    ${FORGE_COMMON}/Graphics/GraphicsConfig.cpp
    ${FORGE_COMMON}/Graphics/Vulkan/Vulkan.c
    ${FORGE_COMMON}/Graphics/Vulkan/Vulkan_cxx.cpp
    ${FORGE_COMMON}/Graphics/Vulkan/VulkanRaytracing.c
)

add_library(ForgeRenderer STATIC ${RENDERER_SOURCES})
set_target_properties(ForgeRenderer PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
target_include_directories(ForgeRenderer PUBLIC
    ${FORGE_COMMON}
    ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/volk
    ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/VulkanMemoryAllocator
    ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/VulkanSDK/include
    ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/renderdoc
)
# VULKAN defined by The-Forge headers
target_compile_options(ForgeRenderer PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions>
    -w
)
target_link_libraries(ForgeRenderer PUBLIC ForgeOS ${VULKAN_LIB})

# ---------------------------------------------------------------------------
# Runtime assets: shaders (FSL), PathStatement, GPU config, fonts
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY ${RUN_DIR}/CompiledShaders ${RUN_DIR}/Shaders
     ${RUN_DIR}/PipelineCaches ${RUN_DIR}/Fonts/TitilliumText ${RUN_DIR}/Scripts)

set(FSL ${FORGE_ROOT}/Common_3/Tools/ForgeShadingLanguage/fsl.py)
set(SHADER_LISTS
    ${FORGE_COMMON}/Application/UI/Shaders/FSL/UIShaders.list
    ${FORGE_COMMON}/Application/Fonts/Shaders/FSL/FontShaders.list
    ${FORGE_COMMON}/Application/Screenshot/Shaders/FSL/ScreenshotShaders.list)

# The-Forge's .list files reference CamelCase names while the files on disk are
# lowercase (they rely on case-insensitive filesystems). Create the missing
# aliases so Linux can compile them, then run the FSL compiler.
add_custom_target(fowe_shaders
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compile_shaders.sh
            ${FORGE_ROOT} ${RUN_DIR} ${Python3_EXECUTABLE}
    COMMENT "Compiling The-Forge FSL shaders for Vulkan"
    VERBATIM
)

# Copy runtime assets next to the executable
add_custom_target(fowe_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/assets/PathStatement.txt ${RUN_DIR}/PathStatement.txt
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/assets/gpu.cfg ${RUN_DIR}/gpu.cfg
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/assets/gpu.data ${RUN_DIR}/gpu.data
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/assets/Fonts/TitilliumText/TitilliumText-Bold.otf ${RUN_DIR}/Fonts/TitilliumText/TitilliumText-Bold.otf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/VulkanSDK/bin/Linux/libVkLayer_khronos_validation.so ${RUN_DIR}/libVkLayer_khronos_validation.so
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FORGE_COMMON}/Graphics/ThirdParty/OpenSource/VulkanSDK/bin/Linux/VkLayer_khronos_validation.json ${RUN_DIR}/VkLayer_khronos_validation.json
    COMMENT "Staging FOWE runtime assets"
    VERBATIM
)

# ---------------------------------------------------------------------------
# FOWE executable
# ---------------------------------------------------------------------------
add_executable(FOWE
    src/core/FOWEApp.cpp
)
target_link_libraries(FOWE PRIVATE ForgeRenderer ForgeOS)
# VULKAN defined by The-Forge headers
set_target_properties(FOWE PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RUN_DIR})
add_dependencies(FOWE fowe_shaders)
add_dependencies(FOWE fowe_assets)
