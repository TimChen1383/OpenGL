// This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <sstream>

#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "ShaderProgram.h"
#include "Texture2D.h"
#include "Camera.h"
#include "Mesh.h"
#include "PhysicsManager.h"

//Global variables
const char* APP_Title = "OpenGL Engine";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gwindow = NULL;
FPSCamera fpsCamera(glm::vec3(0.0f, 5.0f, 15.0f)); // Initial position and orientation
const double ZOOM_SENSITIVITY = -3.0f;
const float MOVE_SPEED = 5.0f;
const float MOUSE_SENSITIVITY = 0.25f; // Mouse sensitivity for camera rotation
glm::vec2 groundUVScale = glm::vec2(20.0f, 20.0f); // Texture UV scale

// Directional light settings (controllable via ImGui)
glm::vec3 gDirLightDirection = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
glm::vec3 gDirLightColor = glm::vec3(0.8f, 0.8f, 0.7f);
float gDirLightIntensity = 1.0f;
float gDirLightYaw = -30.0f;    // Horizontal rotation in degrees
float gDirLightPitch = -60.0f;  // Vertical rotation in degrees (negative = pointing down)

// UI state
bool gShowUI = true;           // Toggle UI visibility with Tab key
bool gCameraEnabled = true;    // Toggle camera control

// Physics UI controls
bool gShowCollisionMesh = false;  // Toggle collision mesh wireframe
glm::vec3 gGravity = glm::vec3(0.0f, -9.81f, 0.0f);  // Gravity vector

// Debug wireframe rendering
unsigned int gWireframeCubeVAO = 0;
unsigned int gWireframeCubeVBO = 0;

// Shadow mapping variables
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// Directional light shadow mapping
unsigned int dirDepthMapFBO;
unsigned int dirDepthMap;

// Physics
PhysicsManager gPhysicsManager;
bool gSimulationStarted = false;

// Teapot spawning
bool gSpawningEnabled = false;           // True while P key is held
double gLastSpawnTime = 0.0;             // Time of last spawn
const double SPAWN_INTERVAL = 0.3;       // Spawn every 0.3 seconds
const glm::vec3 SPAWN_POSITION(0.0f, 5.0f, 0.0f);
const glm::vec3 TEAPOT_HALF_EXTENTS(0.8f, 0.5f, 0.5f);

//Custom Functions
void glfw_OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
void glfw_OnFrameBufferSize(GLFWwindow* window, int width, int height); //update the viewport when the window is resized
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY);
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY);
void update(double elapsedTime);
void showFPS(GLFWwindow* window);
bool InitOpenGL();
void InitImGui();
void ShutdownImGui();
void UpdateLightDirection();
void RenderImGuiUI();
void InitWireframeCube();
void DrawWireframeCube(ShaderProgram& shader, const glm::mat4& model, const glm::vec3& halfExtents);
void CleanupWireframeCube();

int main()
{
	// Initialize OpenGL
	if (!InitOpenGL())
	{
		std::cerr << "OpenGL initialization failed." << std::endl;
		return -1;
	}

	// Initialize ImGui
	InitImGui();

	// Disable VSync for uncapped FPS
	glfwSwapInterval(0);

	//for lighting objects
	ShaderProgram LightingShader;
	LightingShader.loadShaders("Lighting.vert", "Lighting.frag");

	//for ground plane
	ShaderProgram GroundShader;
	GroundShader.loadShaders("Ground.vert", "Ground.frag");

	//for shadow mapping
	ShaderProgram ShadowMapShader;
	ShadowMapShader.loadShaders("ShadowMap.vert", "ShadowMap.frag");

	//for wireframe collision mesh
	ShaderProgram WireframeShader;
	WireframeShader.loadShaders("Wireframe.vert", "Wireframe.frag");

	// Initialize wireframe cube mesh
	InitWireframeCube();

	std::cout << "Shadow map resolution: " << SHADOW_WIDTH << "x" << SHADOW_HEIGHT << std::endl;

	// Ground plane - physics plane is at Y=0
	glm::vec3 GroundPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 GroundScale = glm::vec3(5.0f, 5.0f, 5.0f);

	// Load teapot mesh and texture (for spawning)
	Mesh teapotMesh;
	teapotMesh.loadOBJ("Teapot.obj");
	Texture2D teapotTexture;
	teapotTexture.loadTexture("Pattern3.jpg", true);

	Mesh groundMesh;
	groundMesh.loadOBJ("GroundPlane.obj");
	Texture2D textureGround;
	textureGround.loadTexture("Brick.jpg", true);

	// Configure directional light depth map FBO
	glGenFramebuffers(1, &dirDepthMapFBO);

	// Create directional light depth texture
	glGenTextures(1, &dirDepthMap);
	glBindTexture(GL_TEXTURE_2D, dirDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Attach directional light depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, dirDepthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dirDepthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Shadow map projection settings (light space matrix calculated per-frame)
	float dirNear = 1.0f, dirFar = 50.0f;
	float dirSize = 20.0f;
	glm::mat4 dirLightProjection = glm::ortho(-dirSize, dirSize, -dirSize, dirSize, dirNear, dirFar);
	glm::mat4 dirLightSpaceMatrix;

	// Initialize PhysX
	if (!gPhysicsManager.init())
	{
		std::cerr << "Failed to initialize PhysX!" << std::endl;
		return -1;
	}

	// Create physics ground plane
	gPhysicsManager.createGroundPlane();

	std::cout << "Hold 'P' to spawn teapots!" << std::endl;

	double lastFrameTime = glfwGetTime();
	
	//Main Loop
	while (!glfwWindowShouldClose(gwindow))
	{
		showFPS(gwindow);

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastFrameTime;

		glfwPollEvents();
		update(deltaTime);

		// Step physics simulation if started
		if (gSimulationStarted)
		{
			gPhysicsManager.stepSimulation((float)deltaTime);
		}

		// Spawn teapots while P is held (every 0.3 seconds)
		if (gSpawningEnabled && (currentTime - gLastSpawnTime >= SPAWN_INTERVAL))
		{
			gPhysicsManager.createDynamicBox(SPAWN_POSITION, TEAPOT_HALF_EXTENTS, 1.0f);
			gLastSpawnTime = currentTime;
			std::cout << "Spawned teapot! Total: " << gPhysicsManager.getDynamicActors().size() << std::endl;
		}

		glm::mat4 model, view, projection;

		view = fpsCamera.getViewMatrix();
		projection = glm::perspective(glm::radians(fpsCamera.getFOV()), (float)gWindowWidth / (float)gWindowHeight, 0.1f, 100.0f);
		glm::vec3 viewPos = fpsCamera.getPosition();

		// Update light direction from yaw/pitch and recalculate light space matrix
		UpdateLightDirection();
		glm::vec3 dirLightPos = -gDirLightDirection * 20.0f;
		glm::mat4 dirLightView = glm::lookAt(dirLightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		dirLightSpaceMatrix = dirLightProjection * dirLightView;

		// Calculate effective light color with intensity
		glm::vec3 effectiveLightColor = gDirLightColor * gDirLightIntensity;

		// 1. SHADOW PASS: Render depth from directional light perspective
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, dirDepthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		ShadowMapShader.use();
		ShadowMapShader.setUniform("lightSpaceMatrix", dirLightSpaceMatrix);

		// Render physics teapots to shadow map
		const auto& dynamicActors = gPhysicsManager.getDynamicActors();
		for (size_t i = 0; i < dynamicActors.size(); i++)
		{
			model = gPhysicsManager.getActorTransform(dynamicActors[i]);
			ShadowMapShader.setUniform("model", model);
			teapotMesh.draw();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. RENDER PASS: Render scene with lighting and shadows
		glViewport(0, 0, gWindowWidth, gWindowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Setup lighting shader for physics teapots
		LightingShader.use();
		LightingShader.setUniform("view", view);
		LightingShader.setUniform("projection", projection);
		LightingShader.setUniform("viewPos", viewPos);
		LightingShader.setUniform("dirLightDirection", gDirLightDirection);
		LightingShader.setUniform("dirLightColor", effectiveLightColor);
		LightingShader.setUniform("dirLightSpaceMatrix", dirLightSpaceMatrix);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, dirDepthMap);
		LightingShader.setUniform("shadowMap", 1);

		// Render physics teapots
		for (size_t i = 0; i < dynamicActors.size(); i++)
		{
			model = gPhysicsManager.getActorTransform(dynamicActors[i]);
			LightingShader.setUniform("model", model);

			teapotTexture.bindTexture(0);
			teapotMesh.draw();
			teapotTexture.unbindTexture(0);
		}

		// Render ground plane (TRS order: translate then scale, applied right-to-left)
		model = glm::translate(glm::mat4(1.0f), GroundPos) * glm::scale(glm::mat4(1.0f), GroundScale);
		GroundShader.use();
		GroundShader.setUniform("model", model);
		GroundShader.setUniform("view", view);
		GroundShader.setUniform("projection", projection);
		GroundShader.setUniform("viewPos", viewPos);
		GroundShader.setUniform("groundUVScale", groundUVScale);
		GroundShader.setUniform("dirLightDirection", gDirLightDirection);
		GroundShader.setUniform("dirLightColor", effectiveLightColor);
		GroundShader.setUniform("dirLightSpaceMatrix", dirLightSpaceMatrix);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, dirDepthMap);
		GroundShader.setUniform("shadowMap", 1);

		textureGround.bindTexture(0);
		groundMesh.draw();
		textureGround.unbindTexture(0);

		// Render collision mesh wireframes if enabled
		if (gShowCollisionMesh)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_CULL_FACE);

			WireframeShader.use();
			WireframeShader.setUniform("view", view);
			WireframeShader.setUniform("projection", projection);
			WireframeShader.setUniform("wireColor", glm::vec3(0.0f, 1.0f, 0.0f)); // Green wireframe

			for (size_t i = 0; i < dynamicActors.size(); i++)
			{
				glm::vec3 halfExtents;
				if (gPhysicsManager.getActorBoxHalfExtents(dynamicActors[i], halfExtents))
				{
					model = gPhysicsManager.getActorTransform(dynamicActors[i]);
					DrawWireframeCube(WireframeShader, model, halfExtents);
				}
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_CULL_FACE);
		}

		// Render ImGui UI
		RenderImGuiUI();

		glfwSwapBuffers(gwindow);
		lastFrameTime = currentTime;
	}

	// Cleanup wireframe resources
	CleanupWireframeCube();

	// Cleanup physics
	gPhysicsManager.shutdown();

	// Cleanup ImGui
	ShutdownImGui();

	glfwTerminate();
	return 0;
}

bool InitOpenGL()
{
	// Initialize GLFW. Neccessary
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);


	gwindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_Title, NULL, NULL);
	if (gwindow == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate(); //Shutdown GLFW properly
		return false;
	}

	glfwMakeContextCurrent(gwindow);
	glfwSetKeyCallback(gwindow, glfw_OnKey);
	glfwSetFramebufferSizeCallback(gwindow, glfw_OnFrameBufferSize);
	glfwSetCursorPosCallback(gwindow, glfw_onMouseMove);//Every time the mouse moves, this function is called
	glfwSetScrollCallback(gwindow, glfw_onMouseScroll);

	glfwSetInputMode(gwindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);// Hide the cursor and capture it within the window
	glfwSetCursorPos(gwindow, gWindowWidth/2.0f, gWindowHeight/2.0f);// Center the cursor in the window
	
	
	glewExperimental = GL_TRUE; // Enable experimental features for GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		glfwDestroyWindow(gwindow);
		glfwTerminate(); //Shutdown GLFW properly
		return false;
	}
	
	glClearColor(0.25f, 0.38f, 0.47f, 1.0f); // Set the clear shaderProgram.setUniform("vertColor", glm::vec4(0.0f, 0.0f, blueColor, 1.0f));shaderProgram.setUniform("vertColor", glm::vec4(0.0f, 0.0f, blueColor, 1.0f));color (background color)
	glViewport(0, 0, gWindowWidth, gWindowHeight); // Set the viewport to the window size
	glEnable(GL_DEPTH_TEST);
	
	return true; // Return true if OpenGL initialization is successful
}

void glfw_OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	// Toggle UI visibility and camera control with Tab
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		gShowUI = !gShowUI;
		gCameraEnabled = !gShowUI;

		if (gShowUI)
		{
			// Show cursor when UI is visible
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else
		{
			// Hide and capture cursor when UI is hidden
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPos(window, gWindowWidth / 2.0, gWindowHeight / 2.0);
		}
	}

	// Spawn teapots while P key is held
	if (key == GLFW_KEY_P)
	{
		if (action == GLFW_PRESS)
		{
			gSpawningEnabled = true;
			// Start physics simulation if not already started
			if (!gSimulationStarted)
			{
				gSimulationStarted = true;
				std::cout << "Physics simulation started!" << std::endl;
			}
		}
		else if (action == GLFW_RELEASE)
		{
			gSpawningEnabled = false;
		}
	}
}

void glfw_OnFrameBufferSize(GLFWwindow* window, int width, int height)
{
	gWindowWidth = width;
	gWindowHeight = height;
	glViewport(0, 0, gWindowWidth, gWindowHeight);
}

void glfw_onMouseMove(GLFWwindow* window, double posX, double posY)
{

}
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY)
{
	double fov = fpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;
	fov = glm::clamp(fov, 1.0, 120.0); // Clamp the FOV to a reasonable range
	fpsCamera.setFOV((float)fov); // Update the camera's FOV
}
void update(double elapsedTime)
{
	// Only update camera when camera control is enabled (UI hidden)
	if (!gCameraEnabled)
		return;

	double mouseX, mouseY;

	// Get the current mouse position
	glfwGetCursorPos(gwindow, &mouseX, &mouseY);

	fpsCamera.rotate((float)(gWindowWidth / 2.0 - mouseX) * MOUSE_SENSITIVITY, (float)(gWindowHeight / 2.0 - mouseY) * MOUSE_SENSITIVITY);

	glfwSetCursorPos(gwindow, gWindowWidth / 2.0, gWindowHeight / 2.0);

	//Add WS controls for moving the camera
	if (glfwGetKey(gwindow, GLFW_KEY_W) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getLook());
	else if (glfwGetKey(gwindow, GLFW_KEY_S) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getLook());

	//Add AD controls for moving the camera
	if (glfwGetKey(gwindow, GLFW_KEY_A) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getRight());
	else if (glfwGetKey(gwindow, GLFW_KEY_D) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getRight());

	if (glfwGetKey(gwindow, GLFW_KEY_E) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getUp());
	else if (glfwGetKey(gwindow, GLFW_KEY_R) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getUp());

}
void showFPS(GLFWwindow* window)
{
	static double previousSeconds = 0.0;
	static int frameCount = 0;
	double elapsedSeconds;
	double currentSeconds = glfwGetTime();

	elapsedSeconds = currentSeconds - previousSeconds;

	if (elapsedSeconds > 0.25)
	{
		previousSeconds = currentSeconds;
		double fps = frameCount / elapsedSeconds;
		double msPerframe = 1000.0 / fps;

		std::ostringstream outs;
		outs.precision(3);
		outs << std::fixed
			<< APP_Title << " "
			<< "FPS: " << fps << " "
			<< "Frame Time" << msPerframe << " (ms)";
		glfwSetWindowTitle(window, outs.str().c_str());

		frameCount = 0;
	}
	frameCount++;
}

// Initialize ImGui
void InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(gwindow, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	std::cout << "ImGui initialized. Press TAB to toggle UI." << std::endl;
}

// Shutdown ImGui
void ShutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// Update light direction from yaw and pitch angles
void UpdateLightDirection()
{
	float yawRad = glm::radians(gDirLightYaw);
	float pitchRad = glm::radians(gDirLightPitch);

	// Calculate direction from spherical coordinates
	// Pitch: 0 = horizontal, -90 = straight down, +90 = straight up
	// Yaw: 0 = +X direction, 90 = +Z direction
	gDirLightDirection.x = cos(pitchRad) * cos(yawRad);
	gDirLightDirection.y = sin(pitchRad);
	gDirLightDirection.z = cos(pitchRad) * sin(yawRad);
	gDirLightDirection = glm::normalize(gDirLightDirection);
}

// Render ImGui UI
void RenderImGuiUI()
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (gShowUI)
	{
		// Directional Light Control Window
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(320, 280), ImGuiCond_FirstUseEver);

		ImGui::Begin("Directional Light Controls");

		ImGui::Text("Light Direction");
		ImGui::Separator();

		// Yaw control (horizontal rotation)
		ImGui::SliderFloat("Yaw", &gDirLightYaw, -180.0f, 180.0f, "%.1f deg");

		// Pitch control (vertical rotation)
		ImGui::SliderFloat("Pitch", &gDirLightPitch, -89.0f, 89.0f, "%.1f deg");

		ImGui::Spacing();
		ImGui::Text("Light Color & Intensity");
		ImGui::Separator();

		// Light color picker
		ImGui::ColorEdit3("Color", &gDirLightColor.x);

		// Light intensity slider
		ImGui::SliderFloat("Intensity", &gDirLightIntensity, 0.0f, 3.0f, "%.2f");

		ImGui::Spacing();
		ImGui::Separator();

		// Display current direction vector
		ImGui::Text("Direction: (%.2f, %.2f, %.2f)",
			gDirLightDirection.x, gDirLightDirection.y, gDirLightDirection.z);

		// Reset button
		if (ImGui::Button("Reset to Default"))
		{
			gDirLightYaw = -30.0f;
			gDirLightPitch = -60.0f;
			gDirLightColor = glm::vec3(0.8f, 0.8f, 0.7f);
			gDirLightIntensity = 1.0f;
		}

		ImGui::End();

		// Physics Control Window
		ImGui::SetNextWindowPos(ImVec2(10, 300), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(320, 220), ImGuiCond_FirstUseEver);

		ImGui::Begin("Physics Controls");

		ImGui::Text("Scene Management");
		ImGui::Separator();

		// Reset Physics Scene button
		if (ImGui::Button("Reset Physics Scene"))
		{
			gPhysicsManager.clearDynamicActors();
		}

		ImGui::SameLine();
		ImGui::Text("Objects: %d", (int)gPhysicsManager.getDynamicActors().size());

		ImGui::Spacing();
		ImGui::Text("Debug Visualization");
		ImGui::Separator();

		// Show Collision Mesh checkbox
		ImGui::Checkbox("Show Collision Mesh", &gShowCollisionMesh);

		ImGui::Spacing();
		ImGui::Text("Gravity");
		ImGui::Separator();

		// Gravity Y control (most common adjustment)
		ImGui::SliderFloat("Gravity Y", &gGravity.y, -30.0f, 30.0f, "%.2f m/s^2");

		// Optional: Full gravity vector control
		if (ImGui::TreeNode("Advanced Gravity"))
		{
			ImGui::SliderFloat("Gravity X", &gGravity.x, -20.0f, 20.0f, "%.2f");
			ImGui::SliderFloat("Gravity Z", &gGravity.z, -20.0f, 20.0f, "%.2f");
			ImGui::TreePop();
		}

		// Apply gravity changes
		gPhysicsManager.setGravity(gGravity);

		// Reset gravity button
		if (ImGui::Button("Reset Gravity"))
		{
			gGravity = glm::vec3(0.0f, -9.81f, 0.0f);
			gPhysicsManager.setGravity(gGravity);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text("Controls:");
		ImGui::BulletText("TAB - Toggle UI / Camera");
		ImGui::BulletText("WASD - Move camera");
		ImGui::BulletText("P - Spawn teapots");

		ImGui::End();
	}

	// Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Initialize wireframe cube mesh (unit cube from -1 to 1)
void InitWireframeCube()
{
	// Unit cube vertices (will be scaled by half-extents in shader)
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

	// Indices for wireframe lines (12 edges)
	unsigned int indices[] = {
		// Front face edges
		0, 1,  1, 2,  2, 3,  3, 0,
		// Back face edges
		4, 5,  5, 6,  6, 7,  7, 4,
		// Connecting edges
		0, 4,  1, 5,  2, 6,  3, 7
	};

	unsigned int EBO;
	glGenVertexArrays(1, &gWireframeCubeVAO);
	glGenBuffers(1, &gWireframeCubeVBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(gWireframeCubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, gWireframeCubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

// Draw wireframe cube with given transform and half-extents
void DrawWireframeCube(ShaderProgram& shader, const glm::mat4& transform, const glm::vec3& halfExtents)
{
	// Scale the unit cube by half-extents
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), halfExtents);
	glm::mat4 model = transform * scaleMatrix;

	shader.setUniform("model", model);

	glBindVertexArray(gWireframeCubeVAO);
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

// Cleanup wireframe cube resources
void CleanupWireframeCube()
{
	if (gWireframeCubeVAO)
	{
		glDeleteVertexArrays(1, &gWireframeCubeVAO);
		gWireframeCubeVAO = 0;
	}
	if (gWireframeCubeVBO)
	{
		glDeleteBuffers(1, &gWireframeCubeVBO);
		gWireframeCubeVBO = 0;
	}
}