#pragma once
#include <cstdint>

namespace fowe {

struct Vec3 { float x, y, z; };

class IPhysicsWorld {
public:
    virtual ~IPhysicsWorld() = default;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void Step(float dt) = 0;
    virtual uint64_t CreateRigidBody(const Vec3& pos) = 0;
};

IPhysicsWorld* CreateBulletPhysicsWorld();

} // namespace fowe