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

    // Load volume from .nvdb file (NanoVDB format)
    // Returns false if file cannot be loaded
    bool loadVDBFile(const std::string& filepath);

    // Load NVDB animation sequence from folder
    // Expects files named: baseName_0000.nvdb, baseName_0001.nvdb, etc.
    bool loadNVDBSequence(const std::string& folderPath, const std::string& baseName);

    // Update animation (call every frame with deltaTime)
    void update(float deltaTime);

    // Animation playback controls
    void play() { mIsPlaying = true; }
    void pause() { mIsPlaying = false; }
    void stop() { mIsPlaying = false; mCurrentFrame = 0; mFrameTime = 0.0f; }
    void setFrame(int frame);
    void setFPS(float fps) { mFPS = fps; }
    void setLooping(bool loop) { mLoop = loop; }

    // Animation state getters
    bool isPlaying() const { return mIsPlaying; }
    bool isLooping() const { return mLoop; }
    int getCurrentFrame() const { return mCurrentFrame; }
    int getTotalFrames() const { return mTotalFrames; }
    float getFPS() const { return mFPS; }
    bool isAnimationLoaded() const { return !mFramePaths.empty(); }
    bool isPreloaded() const { return !mFrameTextures.empty(); }

    // Preload all animation frames into GPU memory for smooth playback
    // targetResolution: cubic resolution for each frame (e.g., 192 for 192³)
    bool preloadAllFrames(int targetResolution = 192);

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
    // Load NanoVDB file (.nvdb format)
    bool loadNanoVDBFile(const std::string& filepath);

    // Load a single frame into the specified texture slot (for animation)
    // targetResolution: if > 0, clamp to this cubic resolution; if 0, use source resolution
    bool loadFrameToTexture(int frameIndex, unsigned int& textureOut, int targetResolution = 0);

    // Create 3D texture from volume data
    void createVolumeTexture(const std::vector<float>& data, int resX, int resY, int resZ);

    // Create 3D texture and return the texture ID (for animation frames)
    unsigned int createVolumeTextureReturn(const std::vector<float>& data, int resX, int resY, int resZ);

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

    // Animation state
    std::vector<std::string> mFramePaths;  // Paths to all frame files
    int mCurrentFrame;                      // Current frame index
    int mTotalFrames;                       // Total number of frames
    float mFrameTime;                       // Time accumulator for frame timing
    float mFPS;                             // Playback frames per second
    bool mIsPlaying;                        // Is animation playing
    bool mLoop;                             // Loop animation
    int mLastLoadedFrame;                   // Last frame that was loaded to texture

    // Preloaded frame textures (for smooth playback)
    std::vector<unsigned int> mFrameTextures;  // All frames loaded into GPU
    int mAnimationResolution;                   // Resolution used for preloaded frames
};
