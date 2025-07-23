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

//Global variables
const char* APP_Title = "OpenGL Application";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gwindow = NULL;
FPSCamera fpsCamera(glm::vec3(0.0f, 5.0f, 5.0f)); // Initial position and orientation
const double ZOOM_SENSITIVITY = -3.0f;
const float MOVE_SPEED = 5.0f;
const float MOUSE_SENSITIVITY = 0.25f; // Mouse sensitivity for camera rotation
float angle = 0.0f;
glm::vec2 groundUVScale =glm::vec2(20.0f,20.0f); // Texture UV scale

// Flashlight controls
bool flashlightEnabled = true; // Toggle for flashlight on/off
bool flashlightKeyPressed = false; // To prevent key repeat

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
	
	//for light bulb
	ShaderProgram LightShader;
	LightShader.loadShaders("Light.vert", "Light.frag"); 
	
	//for lighting objects
	ShaderProgram LightingShader;
	LightingShader.loadShaders("Lighting.vert", "Lighting.frag");

	//for ground plane
	ShaderProgram GroundShader;
	GroundShader.loadShaders("Ground.vert", "Ground.frag");
	
	//Model Positions
	glm::vec3 modelPos[] = {
		glm::vec3(0.0f, 0.0f, 0.0f),  //RubberToy
		glm::vec3(3.0f, 0.0f, 0.0f),  //Suzan
		glm::vec3(-3.0f, 0.0f, 0.0f),  //Teapot
	};

	glm::vec3 modelScale[] = {
		glm::vec3(1.0f, 1.0f, 1.0f),  //RubberToy
		glm::vec3(1.0f, 1.0f, 1.0f),  //Suzan
		glm::vec3(1.0f, 1.0f, 1.0f),  //Teapot
	};

	//Add ground plane
	glm::vec3 GroundPos = glm::vec3(0.0f, 0.0f, 0.0f); // Position for the ground plane
	glm::vec3 GroundScale = glm::vec3(5.0f, 5.0f, 5.0f);

	//Add light
	glm::vec3 lightPos(0.0f, 1.0f, 0.0f);
	glm::vec3 lightColor(1.0f, 0.5f, 0.0f);
	float lightIntensity = 1.0f;
	float lightSpeed = 70.0f;
	lightColor = lightColor * lightIntensity; 

	glm::vec3 lightPos2(0.0f, 1.0f, 0.0f);
	glm::vec3 lightColor2(1.0f, 1.0f, 1.0f);
	float angle2 = 0.0f;
	float lightSpeed2 = 50.0f;
	//lightColor2 = lightColor2 * lightIntensity; 
	
	//Load meshes and textures
	//Use our custom Mesh and Texture class array
	const int numModels = 3;
	Mesh mesh[numModels];
	Texture2D texture[numModels];
	
	mesh[0].loadOBJ("RubberToy.obj");
	mesh[1].loadOBJ("Suzan.obj");
	mesh[2].loadOBJ("Teapot.obj");
	
	texture[0].loadTexture("Pattern1.jpg", true);
	texture[1].loadTexture("Pattern2.jpg", true);
	texture[2].loadTexture("Pattern3.jpg", true);

	Mesh groundMesh;
	groundMesh.loadOBJ("GroundPlane.obj");
	Texture2D textureGround;
	textureGround.loadTexture("Brick.jpg", true); 
	
	Mesh lightMesh;
	lightMesh.loadOBJ("light.obj");
	
	double lastFrameTime = glfwGetTime();
	
	//Main Loop
	while (!glfwWindowShouldClose(gwindow))
	{
		showFPS(gwindow); // Show FPS in the console and window title

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastFrameTime;

		glfwPollEvents(); // Poll for events (like keyboard and mouse input)
		update(deltaTime); // Update the camera based on input
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen
		
		/***
		 *Model Matrix: local space to world space
		 *View Matrix: world space to camera space
		 *Projection Matrix: camera space to clip space
		 ***/
		glm::mat4 model, view, projection;
		
		//View Matrix
		view = fpsCamera.getViewMatrix();
		
		//Projection Matrix
		projection = glm::mat4(1.0f); 
		projection = glm::perspective(glm::radians(fpsCamera.getFOV()), (float)gWindowWidth / (float)gWindowHeight, 0.1f, 100.0f);

		//Camera view position
		glm::vec3 viewPos = fpsCamera.getPosition();
		
		
		// Animate the first light (line-direction)
		angle += (float)deltaTime * lightSpeed;
		lightPos.x = 7.0f * sinf(glm::radians(angle2));
		lightPos.z = 7.0f * cosf(glm::radians(angle2));

		// Animate the second light (circle-direction)
		angle2 += (float)deltaTime * lightSpeed2;
		lightPos2.x = 5.0f * cosf(glm::radians(angle2));
		lightPos2.z = 5.0f * sinf(glm::radians(angle2));
		
		// --- Flashlight (spotlight) parameters ---
		// Attach the flashlight to the camera position and direction
		glm::vec3 spotLightPos = fpsCamera.getPosition();
		glm::vec3 spotLightDir = fpsCamera.getLook();

		// Flashlight properties - Lower spotlight intensity
		float spotLightCutoff = glm::cos(glm::radians(20.0f));       // Wider inner cone (40 degree cone total)
		float spotLightOuterCutoff = glm::cos(glm::radians(35.0f)); // Wider outer cone (70 degree cone total)
		float spotLightRange = 40.0f;     // Much longer range for debugging
		float spotLightIntensity = flashlightEnabled ? 20.0f : 0.0f; // Lower intensity
		glm::vec3 spotLightColor = glm::vec3(1.0f, 0.95f, 0.8f) * spotLightIntensity; // Warm white color

		// Pass spotlight uniforms to lighting shader
		LightingShader.use();
		LightingShader.setUniform("view", view);
		LightingShader.setUniform("projection", projection);
		LightingShader.setUniform("viewPos", viewPos);
		LightingShader.setUniform("spotLightPos", spotLightPos);
		LightingShader.setUniform("spotLightDir", spotLightDir);
		LightingShader.setUniform("spotLightCutoff", spotLightCutoff);
		LightingShader.setUniform("spotLightOuterCutoff", spotLightOuterCutoff);
		LightingShader.setUniform("spotLightRange", spotLightRange);
		LightingShader.setUniform("spotLightColor", spotLightColor);


		
		for (int i = 0; i < numModels; i++)
		{
			//Set the model matrix for each model
			model = glm::mat4(1.0f);

			model = glm::scale(glm::mat4(1.0f), modelScale[i]) * glm::translate(glm::mat4(1.0f), modelPos[i]);
			LightingShader.setUniform("model", model); // Set the model matrix in the shader
			
			texture[i].bindTexture(0); // Bind the texture for this model
			mesh[i].draw(); // Draw the mesh
			texture[i].unbindTexture(0); // Unbind the texture after drawing
		}
		
		//Render the ground plane
		model =  glm::scale(glm::mat4(1.0f), GroundScale) * glm::translate(glm::mat4(1.0f), GroundPos);
		GroundShader.use();
		GroundShader.setUniform("model", model);
		GroundShader.setUniform("view", view);
		GroundShader.setUniform("projection", projection);
		GroundShader.setUniform("viewPos", viewPos);
		GroundShader.setUniform("groundUVScale", groundUVScale);
		
		// Pass spotlight uniforms to ground shader as well
		GroundShader.setUniform("spotLightPos", spotLightPos);
		GroundShader.setUniform("spotLightDir", spotLightDir);
		GroundShader.setUniform("spotLightCutoff", spotLightCutoff);
		GroundShader.setUniform("spotLightOuterCutoff", spotLightOuterCutoff);
		GroundShader.setUniform("spotLightRange", spotLightRange);
		GroundShader.setUniform("spotLightColor", spotLightColor);
		
		// --- Directional light parameters ---
        glm::vec3 dirLightDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.5f)); // Down and to the side
        glm::vec3 dirLightColor = glm::vec3(0.0f, 0.0f, 0.0f); // Lower directional light intensity

        // Pass directional light uniforms to lighting shader
        LightingShader.use();
        LightingShader.setUniform("dirLightDirection", dirLightDirection);
        LightingShader.setUniform("dirLightColor", dirLightColor);


		textureGround.bindTexture(0);
		groundMesh.draw();
		textureGround.unbindTexture(0); 

		
		// --- Debug: Render a sphere at the spotlight position ---
        // model = glm::translate(glm::mat4(1.0f), spotLightPos);
        // LightShader.use();
        // LightShader.setUniform("model", model);
        // LightShader.setUniform("view", view);
        // LightShader.setUniform("projection", projection);
        // LightShader.setUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White color for debug sphere
        // lightMesh.draw();
		
		// Swap buffers. The order is very important
		glfwSwapBuffers(gwindow); // Swap buffers to display the rendered content

		//update the time
		lastFrameTime = currentTime;
	}

	
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
		glfwSetWindowShouldClose(window, true); // Close the window when Escape is pressed
	}
	
	// Toggle flashlight with F key
	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		flashlightEnabled = !flashlightEnabled;
		std::cout << "Flashlight " << (flashlightEnabled ? "ON" : "OFF") << std::endl;
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