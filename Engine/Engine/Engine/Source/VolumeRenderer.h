#pragma once

#include <string>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// Forward declarations
class ShaderProgram;

class VolumeRenderer
{
public:
    VolumeRenderer();
    ~VolumeRenderer();

    // Initialize OpenGL resources
    bool init();

    // Load volume from .vdb file (OpenVDB format)
    // Returns false if file cannot be loaded
    bool loadVDBFile(const std::string& filepath);

    // Create a procedural fog sphere (fallback/demo)
    void createProceduralFogSphere(float radius, const glm::vec3& center);

    // Set volume transform
    void setPosition(const glm::vec3& pos) { mPosition = pos; }
    void setScale(const glm::vec3& scale) { mScale = scale; }
    void setRotation(const glm::vec3& eulerDegrees) { mRotation = eulerDegrees; }

    // Get volume transform
    glm::vec3 getPosition() const { return mPosition; }
    glm::vec3 getScale() const { return mScale; }
    glm::vec3 getRotation() const { return mRotation; }

    // Volume rendering parameters
    void setDensityMultiplier(float density) { mDensityMultiplier = density; }
    void setAbsorption(float absorption) { mAbsorption = absorption; }
    void setSmokeColor(const glm::vec3& color) { mSmokeColor = color; }
    void setStepSize(float stepSize) { mStepSize = stepSize; }
    void setMaxSteps(int steps) { mMaxSteps = steps; }

    float getDensityMultiplier() const { return mDensityMultiplier; }
    float getAbsorption() const { return mAbsorption; }
    glm::vec3 getSmokeColor() const { return mSmokeColor; }
    float getStepSize() const { return mStepSize; }
    int getMaxSteps() const { return mMaxSteps; }

    // Shadow parameters
    void setShadowDensityMultiplier(float density) { mShadowDensityMultiplier = density; }
    float getShadowDensityMultiplier() const { return mShadowDensityMultiplier; }

    // Render the volume
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, const glm::vec3& lightDir,
                const glm::vec3& lightColor, unsigned int shadowMap,
                const glm::mat4& lightSpaceMatrix);

    // Render volume to shadow map (for casting shadows)
    void renderToShadowMap(const glm::mat4& lightSpaceMatrix, ShaderProgram& shadowShader);

    // Check if volume is loaded/valid
    bool isValid() const { return mVolumeTexture != 0; }

    // Get volume bounds (for shadow calculations)
    glm::vec3 getBoundsMin() const { return mBoundsMin; }
    glm::vec3 getBoundsMax() const { return mBoundsMax; }

    // Cleanup resources
    void shutdown();

private:
    // Create 3D texture from volume data
    void createVolumeTexture(const std::vector<float>& data, int resX, int resY, int resZ);

    // Create bounding box mesh for raymarching
    void createBoundingBoxMesh();

    // Get model matrix
    glm::mat4 getModelMatrix() const;

    // OpenGL resources
    unsigned int mVolumeTexture;
    unsigned int mBoundingBoxVAO;
    unsigned int mBoundingBoxVBO;
    unsigned int mBoundingBoxEBO;
    ShaderProgram* mShader;

    // Volume dimensions
    int mResolutionX, mResolutionY, mResolutionZ;
    glm::vec3 mBoundsMin;
    glm::vec3 mBoundsMax;

    // Transform
    glm::vec3 mPosition;
    glm::vec3 mScale;
    glm::vec3 mRotation; // Euler angles in degrees

    // Rendering parameters
    float mDensityMultiplier;
    float mAbsorption;
    glm::vec3 mSmokeColor;
    float mStepSize;
    int mMaxSteps;

    // Shadow parameters
    float mShadowDensityMultiplier;
};
