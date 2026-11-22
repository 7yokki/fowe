cmake_minimum_required(VERSION 3.20)
project(FOWE CXX)

set(CMAKE_CXX_STANDARD 17)

# Bullet (core only)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/bullet3)

# The Forge: compile Common_3 sources (no CMake provided upstream)
file(GLOB_RECURSE FORGE_SOURCES
    "third_party/The-Forge/Common_3/Application/*.cpp"
    "third_party/The-Forge/Common_3/Graphics/*.cpp"
    "third_party/The-Forge/Common_3/OS/*.cpp"
    "third_party/The-Forge/Common_3/Utilities/*.cpp"
    "third_party/The-Forge/Common_3/Resources/*.cpp")
add_library(Forge STATIC ${FORGE_SOURCES})
target_include_directories(Forge PUBLIC third_party/The-Forge/Common_3)

# FOWE
add_executable(FOWE src/core/FOWEApp.cpp)
target_link_libraries(FOWE PRIVATE Forge BulletDynamics BulletCollision LinearMath)
target_include_directories(FOWE PRIVATE
    third_party/The-Forge/Common_3
    third_party/miniaudio
    third_party/sol2/include)
