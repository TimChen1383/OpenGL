#include "PhysicsManager.h"
#include <iostream>

PhysicsManager::PhysicsManager()
    : mFoundation(nullptr)
    , mPhysics(nullptr)
    , mDispatcher(nullptr)
    , mScene(nullptr)
    , mMaterial(nullptr)
    , mPvd(nullptr)
    , mIsInitialized(false)
{
}

PhysicsManager::~PhysicsManager()
{
    shutdown();
}

bool PhysicsManager::init()
{
    // Create foundation
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
    if (!mFoundation)
    {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return false;
    }

    // Create physics (without PVD for simplicity)
    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, nullptr);
    if (!mPhysics)
    {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return false;
    }

    // Initialize extensions
    if (!PxInitExtensions(*mPhysics, nullptr))
    {
        std::cerr << "PxInitExtensions failed!" << std::endl;
        return false;
    }

    // Create scene
    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

    mDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    mScene = mPhysics->createScene(sceneDesc);
    if (!mScene)
    {
        std::cerr << "createScene failed!" << std::endl;
        return false;
    }

    // Create default material (static friction, dynamic friction, restitution)
    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.3f);

    mIsInitialized = true;
    std::cout << "PhysX initialized successfully!" << std::endl;
    return true;
}

void PhysicsManager::shutdown()
{
    if (!mIsInitialized) return;

    mDynamicActors.clear();

    PxCloseExtensions();

    if (mScene) mScene->release();
    if (mDispatcher) mDispatcher->release();
    if (mPhysics) mPhysics->release();
    if (mFoundation) mFoundation->release();

    mScene = nullptr;
    mDispatcher = nullptr;
    mPhysics = nullptr;
    mFoundation = nullptr;
    mIsInitialized = false;

    std::cout << "PhysX shutdown complete." << std::endl;
}

void PhysicsManager::createGroundPlane()
{
    if (!mIsInitialized) return;

    PxRigidStatic* groundPlane = PxCreatePlane(*mPhysics, PxPlane(0, 1, 0, 0), *mMaterial);
    mScene->addActor(*groundPlane);
    std::cout << "Ground plane created." << std::endl;
}

PxRigidDynamic* PhysicsManager::createDynamicBox(const glm::vec3& position, const glm::vec3& halfExtents, float mass)
{
    if (!mIsInitialized) return nullptr;

    PxTransform transform(PxVec3(position.x, position.y, position.z));
    PxBoxGeometry geometry(halfExtents.x, halfExtents.y, halfExtents.z);

    PxRigidDynamic* body = PxCreateDynamic(*mPhysics, transform, geometry, *mMaterial, mass);
    if (body)
    {
        body->setAngularDamping(0.5f);
        mScene->addActor(*body);
        mDynamicActors.push_back(body);
    }
    return body;
}

void PhysicsManager::stepSimulation(float deltaTime)
{
    if (!mIsInitialized || !mScene) return;

    mScene->simulate(deltaTime);
    mScene->fetchResults(true);
}

glm::mat4 PhysicsManager::getActorTransform(PxRigidActor* actor)
{
    if (!actor) return glm::mat4(1.0f);

    PxTransform transform = actor->getGlobalPose();

    // Convert PhysX transform to glm matrix
    PxMat44 pxMat(transform);

    glm::mat4 result;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result[i][j] = pxMat[i][j];
        }
    }

    return result;
}

void PhysicsManager::clearDynamicActors()
{
    if (!mIsInitialized || !mScene) return;

    // Remove all dynamic actors from scene
    for (PxRigidDynamic* actor : mDynamicActors)
    {
        if (actor)
        {
            mScene->removeActor(*actor);
            actor->release();
        }
    }
    mDynamicActors.clear();

    std::cout << "Physics scene cleared." << std::endl;
}

void PhysicsManager::setGravity(const glm::vec3& gravity)
{
    if (!mIsInitialized || !mScene) return;

    mScene->setGravity(PxVec3(gravity.x, gravity.y, gravity.z));
}

glm::vec3 PhysicsManager::getGravity() const
{
    if (!mIsInitialized || !mScene) return glm::vec3(0.0f, -9.81f, 0.0f);

    PxVec3 g = mScene->getGravity();
    return glm::vec3(g.x, g.y, g.z);
}

bool PhysicsManager::getActorBoxHalfExtents(PxRigidActor* actor, glm::vec3& outHalfExtents) const
{
    if (!actor) return false;

    // Get the first shape from the actor
    PxShape* shapes[1];
    if (actor->getShapes(shapes, 1) > 0)
    {
        PxShape* shape = shapes[0];
        PxGeometryHolder geomHolder = shape->getGeometry();

        if (geomHolder.getType() == PxGeometryType::eBOX)
        {
            const PxBoxGeometry& boxGeom = geomHolder.box();
            outHalfExtents = glm::vec3(boxGeom.halfExtents.x, boxGeom.halfExtents.y, boxGeom.halfExtents.z);
            return true;
        }
    }
    return false;
}
