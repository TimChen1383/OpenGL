#include "VolumeRenderer.h"
#include "ShaderProgram.h"

#define GLEW_STATIC
#include "GL/glew.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cctype>

// NanoVDB includes
#include <nanovdb/NanoVDB.h>
#include <nanovdb/io/IO.h>
#include <nanovdb/GridHandle.h>

VolumeRenderer::VolumeRenderer()
    : mVolumeTexture(0)
    , mBoundingBoxVAO(0)
    , mBoundingBoxVBO(0)
    , mBoundingBoxEBO(0)
    , mShader(nullptr)
    , mResolutionX(128)
    , mResolutionY(128)
    , mResolutionZ(128)
    , mBoundsMin(-1.0f)
    , mBoundsMax(1.0f)
    , mPosition(0.0f, 2.0f, 0.0f)
    , mScale(2.0f)
    , mRotation(0.0f)
    , mDensityMultiplier(1.0f)
    , mAbsorption(1.0f)
    , mSmokeColor(0.8f, 0.8f, 0.85f)
    , mStepSize(0.01f)
    , mMaxSteps(256)
    , mShadowDensityMultiplier(0.5f)
{
}

VolumeRenderer::~VolumeRenderer()
{
    shutdown();
}

bool VolumeRenderer::init()
{
    // Create shader program
    mShader = new ShaderProgram();
    if (!mShader->loadShaders("VolumeRaymarch.vert", "VolumeRaymarch.frag"))
    {
        std::cerr << "Failed to load volume raymarching shaders!" << std::endl;
        return false;
    }

    // Create bounding box mesh
    createBoundingBoxMesh();

    std::cout << "VolumeRenderer initialized successfully." << std::endl;
    return true;
}

bool VolumeRenderer::loadVDBFile(const std::string& filepath)
{
    std::cout << "Loading volume file: " << filepath << std::endl;

    // Check file extension
    std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "vdb")
    {
        std::cout << "=====================================================================" << std::endl;
        std::cout << "NOTE: .vdb files (OpenVDB format) require conversion to .nvdb format." << std::endl;
        std::cout << "To convert, you can use the nanovdb_convert tool from OpenVDB, or" << std::endl;
        std::cout << "download pre-converted .nvdb files from openvdb.org" << std::endl;
        std::cout << "=====================================================================" << std::endl;
        std::cout << "Creating procedural fog sphere as demonstration..." << std::endl;
        createProceduralFogSphere(1.0f, glm::vec3(0.0f));
        return false;
    }

    try
    {
        // Try to load as NanoVDB file (.nvdb)
        auto handle = nanovdb::io::readGrid(filepath);

        if (!handle)
        {
            std::cerr << "Failed to read NanoVDB grid from: " << filepath << std::endl;
            std::cerr << "Creating procedural fog sphere as fallback..." << std::endl;
            createProceduralFogSphere(1.0f, glm::vec3(0.0f));
            return false;
        }

        // Get grid metadata
        auto* gridMeta = handle.gridMetaData();
        if (!gridMeta)
        {
            std::cerr << "No grid metadata found!" << std::endl;
            createProceduralFogSphere(1.0f, glm::vec3(0.0f));
            return false;
        }

        std::cout << "Grid name: " << gridMeta->shortGridName() << std::endl;
        std::cout << "Grid type: " << (gridMeta->isFogVolume() ? "Fog Volume" :
                                       gridMeta->isLevelSet() ? "Level Set" : "Other") << std::endl;

        // Get the float grid
        auto* grid = handle.grid<float>();
        if (!grid)
        {
            std::cerr << "Grid is not a float grid!" << std::endl;
            createProceduralFogSphere(1.0f, glm::vec3(0.0f));
            return false;
        }

        // Get bounding box
        auto bbox = grid->indexBBox();
        auto worldBBox = grid->worldBBox();

        int minX = bbox.min()[0], minY = bbox.min()[1], minZ = bbox.min()[2];
        int maxX = bbox.max()[0], maxY = bbox.max()[1], maxZ = bbox.max()[2];

        // Calculate resolution (clamp to reasonable size)
        int sizeX = std::min(maxX - minX + 1, 256);
        int sizeY = std::min(maxY - minY + 1, 256);
        int sizeZ = std::min(maxZ - minZ + 1, 256);

        // Use target resolution of 128^3
        mResolutionX = std::min(sizeX, 128);
        mResolutionY = std::min(sizeY, 128);
        mResolutionZ = std::min(sizeZ, 128);

        std::cout << "Volume resolution: " << mResolutionX << "x" << mResolutionY << "x" << mResolutionZ << std::endl;

        // Create accessor for reading values
        auto acc = grid->getAccessor();

        // Sample the grid into a 3D array
        std::vector<float> volumeData(mResolutionX * mResolutionY * mResolutionZ, 0.0f);

        float scaleX = (float)(maxX - minX) / (mResolutionX - 1);
        float scaleY = (float)(maxY - minY) / (mResolutionY - 1);
        float scaleZ = (float)(maxZ - minZ) / (mResolutionZ - 1);

        float maxVal = 0.0f;

        for (int z = 0; z < mResolutionZ; ++z)
        {
            for (int y = 0; y < mResolutionY; ++y)
            {
                for (int x = 0; x < mResolutionX; ++x)
                {
                    // Map to original grid coordinates
                    int gx = minX + (int)(x * scaleX);
                    int gy = minY + (int)(y * scaleY);
                    int gz = minZ + (int)(z * scaleZ);

                    nanovdb::Coord coord(gx, gy, gz);
                    float value = acc.getValue(coord);

                    // For level sets, convert to density (inside = positive)
                    if (gridMeta->isLevelSet())
                    {
                        value = value < 0 ? -value : 0.0f;
                    }

                    int idx = x + y * mResolutionX + z * mResolutionX * mResolutionY;
                    volumeData[idx] = std::max(0.0f, value);
                    maxVal = std::max(maxVal, volumeData[idx]);
                }
            }
        }

        // Normalize values
        if (maxVal > 0.0f)
        {
            for (float& v : volumeData)
            {
                v /= maxVal;
            }
        }

        // Store bounds
        mBoundsMin = glm::vec3(worldBBox.min()[0], worldBBox.min()[1], worldBBox.min()[2]);
        mBoundsMax = glm::vec3(worldBBox.max()[0], worldBBox.max()[1], worldBBox.max()[2]);

        // Create texture
        createVolumeTexture(volumeData, mResolutionX, mResolutionY, mResolutionZ);

        std::cout << "VDB file loaded successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception while loading VDB file: " << e.what() << std::endl;
        std::cerr << "Creating procedural fog sphere as fallback..." << std::endl;
        createProceduralFogSphere(1.0f, glm::vec3(0.0f));
        return false;
    }
}

void VolumeRenderer::createProceduralFogSphere(float radius, const glm::vec3& center)
{
    std::cout << "Creating procedural cloud volume..." << std::endl;

    mResolutionX = 128;
    mResolutionY = 128;
    mResolutionZ = 128;

    std::vector<float> volumeData(mResolutionX * mResolutionY * mResolutionZ, 0.0f);

    // Helper lambda for pseudo-random noise
    auto hash = [](float n) -> float {
        return fmodf(sinf(n) * 43758.5453123f, 1.0f);
    };

    auto noise3D = [&](float x, float y, float z) -> float {
        float fx = floorf(x), fy = floorf(y), fz = floorf(z);
        float dx = x - fx, dy = y - fy, dz = z - fz;

        // Smooth interpolation
        dx = dx * dx * (3.0f - 2.0f * dx);
        dy = dy * dy * (3.0f - 2.0f * dy);
        dz = dz * dz * (3.0f - 2.0f * dz);

        float n = fx + fy * 57.0f + fz * 113.0f;
        float v000 = hash(n);
        float v100 = hash(n + 1.0f);
        float v010 = hash(n + 57.0f);
        float v110 = hash(n + 58.0f);
        float v001 = hash(n + 113.0f);
        float v101 = hash(n + 114.0f);
        float v011 = hash(n + 170.0f);
        float v111 = hash(n + 171.0f);

        float v00 = v000 + dx * (v100 - v000);
        float v10 = v010 + dx * (v110 - v010);
        float v01 = v001 + dx * (v101 - v001);
        float v11 = v011 + dx * (v111 - v011);

        float v0 = v00 + dy * (v10 - v00);
        float v1 = v01 + dy * (v11 - v01);

        return v0 + dz * (v1 - v0);
    };

    // Generate cloud-like volume with multiple overlapping spheres and noise
    for (int z = 0; z < mResolutionZ; ++z)
    {
        for (int y = 0; y < mResolutionY; ++y)
        {
            for (int x = 0; x < mResolutionX; ++x)
            {
                // Normalize coordinates to [-1, 1]
                float nx = (x / (float)(mResolutionX - 1)) * 2.0f - 1.0f;
                float ny = (y / (float)(mResolutionY - 1)) * 2.0f - 1.0f;
                float nz = (z / (float)(mResolutionZ - 1)) * 2.0f - 1.0f;

                float density = 0.0f;

                // Main body sphere
                float dist = sqrtf(nx * nx + ny * ny + nz * nz);
                if (dist < 0.8f)
                {
                    float t = dist / 0.8f;
                    density = 1.0f - t * t;
                }

                // Add smaller spheres for cloud-like appearance
                // Sphere 1 - upper right
                float d1 = sqrtf((nx - 0.4f) * (nx - 0.4f) + (ny - 0.3f) * (ny - 0.3f) + nz * nz);
                if (d1 < 0.5f)
                {
                    float t = d1 / 0.5f;
                    density = std::max(density, (1.0f - t * t) * 0.8f);
                }

                // Sphere 2 - upper left
                float d2 = sqrtf((nx + 0.35f) * (nx + 0.35f) + (ny - 0.25f) * (ny - 0.25f) + (nz - 0.1f) * (nz - 0.1f));
                if (d2 < 0.45f)
                {
                    float t = d2 / 0.45f;
                    density = std::max(density, (1.0f - t * t) * 0.7f);
                }

                // Sphere 3 - lower front
                float d3 = sqrtf((nx + 0.1f) * (nx + 0.1f) + (ny + 0.3f) * (ny + 0.3f) + (nz - 0.3f) * (nz - 0.3f));
                if (d3 < 0.4f)
                {
                    float t = d3 / 0.4f;
                    density = std::max(density, (1.0f - t * t) * 0.6f);
                }

                // Add turbulent noise for cloud detail
                if (density > 0.0f)
                {
                    float noiseScale = 4.0f;
                    float noise = noise3D(nx * noiseScale, ny * noiseScale, nz * noiseScale) * 0.5f +
                                  noise3D(nx * noiseScale * 2.0f, ny * noiseScale * 2.0f, nz * noiseScale * 2.0f) * 0.25f +
                                  noise3D(nx * noiseScale * 4.0f, ny * noiseScale * 4.0f, nz * noiseScale * 4.0f) * 0.125f;

                    density *= (0.7f + noise * 0.6f);
                    density = std::max(0.0f, std::min(1.0f, density));
                }

                int idx = x + y * mResolutionX + z * mResolutionX * mResolutionY;
                volumeData[idx] = density;
            }
        }
    }

    // Set bounds
    mBoundsMin = glm::vec3(-1.0f);
    mBoundsMax = glm::vec3(1.0f);

    // Create texture
    createVolumeTexture(volumeData, mResolutionX, mResolutionY, mResolutionZ);

    std::cout << "Procedural cloud volume created." << std::endl;
}

void VolumeRenderer::createVolumeTexture(const std::vector<float>& data, int resX, int resY, int resZ)
{
    // Delete old texture if exists
    if (mVolumeTexture)
    {
        glDeleteTextures(1, &mVolumeTexture);
        mVolumeTexture = 0;
    }

    glGenTextures(1, &mVolumeTexture);
    glBindTexture(GL_TEXTURE_3D, mVolumeTexture);

    // Upload data
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F, resX, resY, resZ, 0, GL_RED, GL_FLOAT, data.data());

    // Set filtering - linear for smooth interpolation
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to border with zero density outside
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_3D, 0);

    std::cout << "3D volume texture created: " << resX << "x" << resY << "x" << resZ << std::endl;
}

void VolumeRenderer::createBoundingBoxMesh()
{
    // Unit cube vertices [-1, 1]
    float vertices[] = {
        // Front face
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        // Back face
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f
    };

    // Indices for cube (12 triangles = 6 faces)
    unsigned int indices[] = {
        // Front
        0, 1, 2,  2, 3, 0,
        // Back
        5, 4, 7,  7, 6, 5,
        // Left
        4, 0, 3,  3, 7, 4,
        // Right
        1, 5, 6,  6, 2, 1,
        // Top
        3, 2, 6,  6, 7, 3,
        // Bottom
        4, 5, 1,  1, 0, 4
    };

    glGenVertexArrays(1, &mBoundingBoxVAO);
    glGenBuffers(1, &mBoundingBoxVBO);
    glGenBuffers(1, &mBoundingBoxEBO);

    glBindVertexArray(mBoundingBoxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mBoundingBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBoundingBoxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

glm::mat4 VolumeRenderer::getModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, mPosition);
    model = glm::rotate(model, glm::radians(mRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(mRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(mRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, mScale);
    return model;
}

void VolumeRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                            const glm::vec3& cameraPos, const glm::vec3& lightDir,
                            const glm::vec3& lightColor, unsigned int shadowMap,
                            const glm::mat4& lightSpaceMatrix)
{
    if (!mVolumeTexture || !mShader)
        return;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writing but keep depth testing
    glDepthMask(GL_FALSE);

    // Disable face culling to render both sides
    glDisable(GL_CULL_FACE);

    mShader->use();

    glm::mat4 model = getModelMatrix();
    glm::mat4 invModel = glm::inverse(model);

    mShader->setUniform("model", model);
    mShader->setUniform("view", view);
    mShader->setUniform("projection", projection);
    mShader->setUniform("invModel", invModel);

    mShader->setUniform("cameraPos", cameraPos);
    mShader->setUniform("lightDir", glm::normalize(lightDir));
    mShader->setUniform("lightColor", lightColor);

    mShader->setUniform("densityMultiplier", mDensityMultiplier);
    mShader->setUniform("absorption", mAbsorption);
    mShader->setUniform("smokeColor", mSmokeColor);
    mShader->setUniform("stepSize", mStepSize);
    mShader->setUniform("maxSteps", mMaxSteps);

    // Shadow mapping
    mShader->setUniform("lightSpaceMatrix", lightSpaceMatrix);
    mShader->setUniform("shadowDensityMultiplier", mShadowDensityMultiplier);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, mVolumeTexture);
    mShader->setUniform("volumeTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    mShader->setUniform("shadowMap", 1);

    // Draw bounding box
    glBindVertexArray(mBoundingBoxVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore state
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void VolumeRenderer::renderToShadowMap(const glm::mat4& lightSpaceMatrix, ShaderProgram& shadowShader)
{
    // Volume shadow rendering is complex - for now we skip it
    // A proper implementation would raymarch in light space and accumulate opacity
}

void VolumeRenderer::shutdown()
{
    if (mVolumeTexture)
    {
        glDeleteTextures(1, &mVolumeTexture);
        mVolumeTexture = 0;
    }

    if (mBoundingBoxVAO)
    {
        glDeleteVertexArrays(1, &mBoundingBoxVAO);
        mBoundingBoxVAO = 0;
    }

    if (mBoundingBoxVBO)
    {
        glDeleteBuffers(1, &mBoundingBoxVBO);
        mBoundingBoxVBO = 0;
    }

    if (mBoundingBoxEBO)
    {
        glDeleteBuffers(1, &mBoundingBoxEBO);
        mBoundingBoxEBO = 0;
    }

    if (mShader)
    {
        delete mShader;
        mShader = nullptr;
    }
}
