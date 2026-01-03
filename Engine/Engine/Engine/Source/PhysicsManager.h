#ifndef PHYSICS_MANAGER_H
#define PHYSICS_MANAGER_H

#include "PxPhysicsAPI.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

// Forward declaration
struct Vertex;

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

    // Create a convex mesh from vertices (cooks and caches the mesh)
    PxConvexMesh* createConvexMesh(const std::vector<Vertex>& vertices);

    // Create a dynamic body with convex mesh collision
    PxRigidDynamic* createDynamicConvex(const glm::vec3& position, PxConvexMesh* convexMesh, float mass = 1.0f);

    // Get convex mesh vertices for debug rendering
    bool getConvexMeshData(PxRigidActor* actor, std::vector<glm::vec3>& outVertices, std::vector<unsigned int>& outIndices) const;

    // Step the simulation
    void stepSimulation(float deltaTime);

    // Get transform matrix from physics actor
    glm::mat4 getActorTransform(PxRigidActor* actor);

    // Get all dynamic actors
    const std::vector<PxRigidDynamic*>& getDynamicActors() const { return mDynamicActors; }

    // Check if physics is initialized
    bool isInitialized() const { return mIsInitialized; }

    // Clear all dynamic actors (reset scene)
    void clearDynamicActors();

    // Set gravity
    void setGravity(const glm::vec3& gravity);

    // Get current gravity
    glm::vec3 getGravity() const;

    // Get collision box half-extents for an actor (for debug rendering)
    bool getActorBoxHalfExtents(PxRigidActor* actor, glm::vec3& outHalfExtents) const;

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
    std::vector<PxConvexMesh*> mConvexMeshes;  // Cache cooked convex meshes
    bool mIsInitialized;
};

#endif
