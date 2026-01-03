#include "PhysicsManager.h"
#include "Mesh.h"
#include "cooking/PxCooking.h"
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

    // Release convex meshes
    for (PxConvexMesh* mesh : mConvexMeshes)
    {
        if (mesh) mesh->release();
    }
    mConvexMeshes.clear();

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

PxConvexMesh* PhysicsManager::createConvexMesh(const std::vector<Vertex>& vertices)
{
    if (!mIsInitialized) return nullptr;

    // Extract positions from vertices
    std::vector<PxVec3> points;
    points.reserve(vertices.size());
    for (const Vertex& v : vertices)
    {
        points.push_back(PxVec3(v.position.x, v.position.y, v.position.z));
    }

    // Create convex mesh descriptor
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count = (PxU32)points.size();
    convexDesc.points.stride = sizeof(PxVec3);
    convexDesc.points.data = points.data();
    convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;  // Let PhysX compute the convex hull

    // Setup cooking parameters
    PxCookingParams cookingParams(mPhysics->getTolerancesScale());

    // Cook the convex mesh using standalone function (PhysX 5.x API)
    PxDefaultMemoryOutputStream writeBuffer;
    PxConvexMeshCookingResult::Enum result;
    if (!PxCookConvexMesh(cookingParams, convexDesc, writeBuffer, &result))
    {
        std::cerr << "Failed to cook convex mesh!" << std::endl;
        return nullptr;
    }

    // Create the convex mesh from cooked data
    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxConvexMesh* convexMesh = mPhysics->createConvexMesh(readBuffer);

    if (convexMesh)
    {
        mConvexMeshes.push_back(convexMesh);
        std::cout << "Convex mesh created with " << convexMesh->getNbVertices() << " vertices." << std::endl;
    }

    return convexMesh;
}

PxRigidDynamic* PhysicsManager::createDynamicConvex(const glm::vec3& position, PxConvexMesh* convexMesh, float mass)
{
    if (!mIsInitialized || !convexMesh) return nullptr;

    PxTransform transform(PxVec3(position.x, position.y, position.z));
    PxConvexMeshGeometry geometry(convexMesh);

    PxRigidDynamic* body = PxCreateDynamic(*mPhysics, transform, geometry, *mMaterial, mass);
    if (body)
    {
        body->setAngularDamping(0.5f);
        mScene->addActor(*body);
        mDynamicActors.push_back(body);
    }
    return body;
}

bool PhysicsManager::getConvexMeshData(PxRigidActor* actor, std::vector<glm::vec3>& outVertices, std::vector<unsigned int>& outIndices) const
{
    if (!actor) return false;

    PxShape* shapes[1];
    if (actor->getShapes(shapes, 1) == 0) return false;

    PxGeometryHolder geomHolder = shapes[0]->getGeometry();
    if (geomHolder.getType() != PxGeometryType::eCONVEXMESH) return false;

    const PxConvexMeshGeometry& convexGeom = geomHolder.convexMesh();
    PxConvexMesh* mesh = convexGeom.convexMesh;

    // Get vertices
    const PxVec3* verts = mesh->getVertices();
    PxU32 numVerts = mesh->getNbVertices();

    outVertices.clear();
    outVertices.reserve(numVerts);
    for (PxU32 i = 0; i < numVerts; i++)
    {
        outVertices.push_back(glm::vec3(verts[i].x, verts[i].y, verts[i].z));
    }

    // Get polygon indices for wireframe rendering
    outIndices.clear();
    const PxU8* indexBuffer = mesh->getIndexBuffer();
    PxU32 numPolygons = mesh->getNbPolygons();

    for (PxU32 p = 0; p < numPolygons; p++)
    {
        PxHullPolygon polygon;
        mesh->getPolygonData(p, polygon);

        // Create edges for this polygon (wireframe)
        for (PxU32 i = 0; i < polygon.mNbVerts; i++)
        {
            PxU32 idx0 = indexBuffer[polygon.mIndexBase + i];
            PxU32 idx1 = indexBuffer[polygon.mIndexBase + ((i + 1) % polygon.mNbVerts)];
            outIndices.push_back(idx0);
            outIndices.push_back(idx1);
        }
    }

    return true;
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
