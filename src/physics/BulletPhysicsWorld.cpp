// Bullet Physics backend for FOWE behind the IPhysicsWorld abstraction.
#include "IPhysicsWorld.h"

#include <btBulletDynamicsCommon.h>

#include <vector>

namespace fowe {
namespace {

struct BodyRecord
{
    btRigidBody*           pBody   = nullptr;
    btCollisionShape*      pShape  = nullptr;
    btDefaultMotionState*  pMotion = nullptr;
};

class BulletPhysicsWorld final : public IPhysicsWorld
{
public:
    bool Init() override
    {
        if (mWorld)
            return true;

        mCollisionConfig = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher(mCollisionConfig);
        mBroadphase = new btDbvtBroadphase();
        mSolver = new btSequentialImpulseConstraintSolver();
        mWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfig);
        if (!mWorld)
            return false;

        mWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));
        return true;
    }

    void Shutdown() override
    {
        if (!mWorld)
            return;

        for (BodyRecord& record : mBodies)
        {
            if (record.pBody)
                mWorld->removeRigidBody(record.pBody);

            delete record.pMotion;
            delete record.pBody;
            delete record.pShape;
        }
        mBodies.clear();

        delete mWorld;
        mWorld = nullptr;
        delete mSolver;
        mSolver = nullptr;
        delete mBroadphase;
        mBroadphase = nullptr;
        delete mDispatcher;
        mDispatcher = nullptr;
        delete mCollisionConfig;
        mCollisionConfig = nullptr;
    }

    void Step(float deltaTime) override
    {
        if (mWorld)
            mWorld->stepSimulation(deltaTime, 4, 1.0f / 60.0f);
    }

    void SetGravity(const Vec3& gravity) override
    {
        if (mWorld)
            mWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    }

    uint64_t CreateBox(const RigidBodyDesc& desc) override
    {
        if (!mWorld)
            return 0;

        btBoxShape* pShape = new btBoxShape(btVector3(desc.halfExtents.x, desc.halfExtents.y, desc.halfExtents.z));

        btVector3 inertia(0.0f, 0.0f, 0.0f);
        if (desc.mass > 0.0f)
            pShape->calculateLocalInertia(desc.mass, inertia);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(desc.position.x, desc.position.y, desc.position.z));

        btDefaultMotionState* pMotion = new btDefaultMotionState(transform);

        btRigidBody::btRigidBodyConstructionInfo constructionInfo(desc.mass, pMotion, pShape, inertia);
        btRigidBody* pBody = new btRigidBody(constructionInfo);
        pBody->setFriction(0.7f);
        pBody->setRestitution(0.1f);

        mWorld->addRigidBody(pBody);
        mBodies.push_back({ pBody, pShape, pMotion });

        return static_cast<uint64_t>(mBodies.size());
    }

    bool GetBodyTransform(uint64_t body, Vec3& outPosition, Quat& outRotation) const override
    {
        if (body == 0 || body > mBodies.size())
            return false;

        const btTransform& transform = mBodies[body - 1].pBody->getWorldTransform();
        const btVector3&   origin    = transform.getOrigin();
        const btQuaternion rotation  = transform.getRotation();

        outPosition = { origin.x(), origin.y(), origin.z() };
        outRotation = { rotation.x(), rotation.y(), rotation.z(), rotation.w() };
        return true;
    }

private:
    btDefaultCollisionConfiguration*     mCollisionConfig = nullptr;
    btCollisionDispatcher*               mDispatcher      = nullptr;
    btBroadphaseInterface*               mBroadphase      = nullptr;
    btSequentialImpulseConstraintSolver* mSolver          = nullptr;
    btDiscreteDynamicsWorld*             mWorld           = nullptr;

    std::vector<BodyRecord> mBodies;
};

} // namespace

IPhysicsWorld* CreateBulletPhysicsWorld()
{
    return new BulletPhysicsWorld();
}

void DestroyPhysicsWorld(IPhysicsWorld* world)
{
    delete world;
}

} // namespace fowe
