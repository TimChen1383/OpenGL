#ifndef PHYSICS_MANAGER_H
#define PHYSICS_MANAGER_H

#include "PxPhysicsAPI.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

using namespace physx;

class PhysicsManager
{
public:
    PhysicsManager();
    ~PhysicsManager();

    bool init();
    void shutdown();

    // Create ground plane
    void createGroundPlane();

    // Create a dynamic box at position with given half-extents
    PxRigidDynamic* createDynamicBox(const glm::vec3& position, const glm::vec3& halfExtents, float mass = 1.0f);

    // Step the simulation
    void stepSimulation(float deltaTime);

    // Get transform matrix from physics actor
    glm::mat4 getActorTransform(PxRigidActor* actor);

    // Get all dynamic actors
    const std::vector<PxRigidDynamic*>& getDynamicActors() const { return mDynamicActors; }

    // Check if physics is initialized
    bool isInitialized() const { return mIsInitialized; }

private:
    PxDefaultAllocator mAllocator;
    PxDefaultErrorCallback mErrorCallback;
    PxFoundation* mFoundation;
    PxPhysics* mPhysics;
    PxDefaultCpuDispatcher* mDispatcher;
    PxScene* mScene;
    PxMaterial* mMaterial;
    PxPvd* mPvd;

    std::vector<PxRigidDynamic*> mDynamicActors;
    bool mIsInitialized;
};

#endif
