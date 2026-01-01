// This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <sstream>

#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"

#include "ShaderProgram.h"
#include "Texture2D.h"
#include "Camera.h"
#include "Mesh.h"
#include "PhysicsManager.h"

//Global variables
const char* APP_Title = "OpenGL PhysX Demo";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gwindow = NULL;
FPSCamera fpsCamera(glm::vec3(0.0f, 5.0f, 15.0f)); // Initial position and orientation
const double ZOOM_SENSITIVITY = -3.0f;
const float MOVE_SPEED = 5.0f;
const float MOUSE_SENSITIVITY = 0.25f; // Mouse sensitivity for camera rotation
glm::vec2 groundUVScale = glm::vec2(20.0f, 20.0f); // Texture UV scale

// Static directional light direction (pointing down and slightly angled)
const glm::vec3 DIRECTIONAL_LIGHT_DIR = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
const glm::vec3 DIRECTIONAL_LIGHT_COLOR = glm::vec3(0.8f, 0.8f, 0.7f);

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

int main()
{
	// Initialize OpenGL
	if (!InitOpenGL())
	{
		std::cerr << "OpenGL initialization failed." << std::endl;
		return -1;
	}

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

	// Calculate static directional light space matrix (only needs to be done once)
	float dirNear = 1.0f, dirFar = 50.0f;
	float dirSize = 20.0f;
	glm::mat4 dirLightProjection = glm::ortho(-dirSize, dirSize, -dirSize, dirSize, dirNear, dirFar);
	glm::vec3 dirLightPos = -DIRECTIONAL_LIGHT_DIR * 20.0f;
	glm::mat4 dirLightView = glm::lookAt(dirLightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 dirLightSpaceMatrix = dirLightProjection * dirLightView;

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
		LightingShader.setUniform("dirLightDirection", DIRECTIONAL_LIGHT_DIR);
		LightingShader.setUniform("dirLightColor", DIRECTIONAL_LIGHT_COLOR);
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
		GroundShader.setUniform("dirLightDirection", DIRECTIONAL_LIGHT_DIR);
		GroundShader.setUniform("dirLightColor", DIRECTIONAL_LIGHT_COLOR);
		GroundShader.setUniform("dirLightSpaceMatrix", dirLightSpaceMatrix);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, dirDepthMap);
		GroundShader.setUniform("shadowMap", 1);

		textureGround.bindTexture(0);
		groundMesh.draw();
		textureGround.unbindTexture(0);

		glfwSwapBuffers(gwindow);
		lastFrameTime = currentTime;
	}

	// Cleanup physics
	gPhysicsManager.shutdown();

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