#pragma once
#include <cstdint>

namespace fowe {

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Quat
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct RigidBodyDesc
{
    Vec3  position    = {};
    Vec3  halfExtents = { 0.5f, 0.5f, 0.5f };
    float mass        = 1.0f; // 0 = static body
};

// Backend-agnostic physics abstraction. Bullet is the first implementation;
// a Jolt backend can be added later without touching game/editor code.
class IPhysicsWorld
{
public:
    virtual ~IPhysicsWorld() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
    virtual void Step(float deltaTime) = 0;
    virtual void SetGravity(const Vec3& gravity) = 0;

    // Returns a non-zero handle, or 0 on failure.
    virtual uint64_t CreateBox(const RigidBodyDesc& desc) = 0;
    virtual bool     GetBodyTransform(uint64_t body, Vec3& outPosition, Quat& outRotation) const = 0;
};

IPhysicsWorld* CreateBulletPhysicsWorld();
void           DestroyPhysicsWorld(IPhysicsWorld* world);

} // namespace fowe
