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

//Global variables
const char* APP_Title = "OpenGL Application";
int gWindowWidth = 1024;
int gWindowHeight = 768;
GLFWwindow* gwindow = NULL;
const std::string texture1FileName = "MetalBox.jpg";
const std::string texture2FileName = "MetalGround.jpg";

//Create costum orbit camera
OrbitCamera orbitCamera;
float gYaw = 0.0f;
float gPitch = 0.0f;
float gRadius = 10.0f;
const float MOUSE_SENSITIVITY = 0.25f; // Mouse sensitivity for camera rotation

//Custom Functions
void glfw_OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
void glfw_OnFrameBufferSize(GLFWwindow* window, int width, int height); //update the viewport when the window is resized
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY); 
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

	//Vertices for a simple triangle
	GLfloat vertices[] =
	{
		// Position        //Texture Coordinate    
		//Front face
		-1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f, 0.0f,

		//Back face
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
		
		//Left face
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 1.0f, 0.0f,

		//Right face
		1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
		
		//Top face
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
		
		//Bottom face
		-1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
	};

	// Position of the cube
	glm::vec3 cubePos = glm::vec3(0.0f, 0.0f, 0.0f); 
	
	GLuint VBO, VAO;

	// Generate and bind Vertex Array Object (VAO)
	//A VAO is an OpenGL object that stores the configuration of vertex attributes.It simplifies the process of switching between different vertex configurations.Related to VBO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Generate and bind Vertex Buffer Object (VBO)
	//A VBO is a memory buffer in the GPU that stores vertex data (e.g., positions, colors, normals)
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position vertex attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(0);

	//Texture coordinate vertex attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); // Offset by 3 floats for position
	glEnableVertexAttribArray(1);
	
	//Create our custom class
	ShaderProgram shaderProgram;
	// Load shaders from files. Text file
	shaderProgram.loadShaders("Basic.vert", "Basic.frag"); 

	// Load texture from file
	Texture2D texture;
	texture.loadTexture(texture1FileName, true); 
	Texture2D texture2;
	texture2.loadTexture(texture2FileName, true);

	
	//get time for rotating the cube
	float cubeAngle = 0.0f;
	double lastFrameTime = glfwGetTime();
	
	//Main Loop
	while (!glfwWindowShouldClose(gwindow))
	{
		showFPS(gwindow); // Show FPS in the console and window title

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastFrameTime;

		glfwPollEvents(); // Poll for events (like keyboard and mouse input)

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen

		texture.bindTexture(0);
		
		/***
		 *Model Matrix: local space to world space
		 *View Matrix: world space to camera space
		 *Projection Matrix: camera space to clip space
		 ***/
		glm::mat4 model, view, projection;

		orbitCamera.setLookAt(cubePos);
		orbitCamera.rotate(gYaw, gPitch);
		orbitCamera.setRadius(gRadius);
		
		//Model Matrix
		model = glm::mat4(1.0f);
		model = glm::translate(model, cubePos);
		
		//View Matrix
		view = orbitCamera.getViewMatrix();
		
		//Projection Matrix
		projection = glm::mat4(1.0f); 
		projection = glm::perspective(glm::radians(45.0f), (float)gWindowWidth / (float)gWindowHeight, 0.1f, 100.0f);
		
		shaderProgram.use();
		
		//Pass the matrices to the shader files
		shaderProgram.setUniform("model", model);
		shaderProgram.setUniform("view", view);
		shaderProgram.setUniform("projection", projection);
		
		glBindVertexArray(VAO); // Bind the VAO to use the vertex attributes
		glDrawArrays(GL_TRIANGLES, 0,36);

		//Create ground plane
		texture2.bindTexture(0);
		glm::vec3 floorPos = glm::vec3(0.0f, 0.0f, 0.0f); // Position of the floor
		floorPos.y = -10.0f;

		model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 0.1f, 10.0f));
		model = glm::translate(model, floorPos) ;
		shaderProgram.setUniform("model", model);
		glDrawArrays(GL_TRIANGLES, 0,36);


		glBindVertexArray(0); // Unbind the VAO

		// Swap buffers. The order is very important
		glfwSwapBuffers(gwindow); // Swap buffers to display the rendered content

		//update the time
		lastFrameTime = currentTime;
	}

	// Delete the shader program after use
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	
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

}

void glfw_OnFrameBufferSize(GLFWwindow* window, int width, int height)
{
	gWindowWidth = width;
	gWindowHeight = height;
	glViewport(0, 0, gWindowWidth, gWindowHeight);
}

void glfw_onMouseMove(GLFWwindow* window, double posX, double posY)
{
	static glm::vec2 lastMousePos = glm::vec2(0.0f, 0.0f);

	//Update angles based on Left Mouse Button input
	if (glfwGetMouseButton(gwindow, GLFW_MOUSE_BUTTON_LEFT) == 1)
	{
		//each pixel represents a quater of a degree
		gYaw -=((float)posX - lastMousePos.x) * MOUSE_SENSITIVITY;
		gPitch += ((float)posY - lastMousePos.y) * MOUSE_SENSITIVITY;
	}
	//Change orbit camera radius with Right Mouse Button input
	if (glfwGetMouseButton(gwindow, GLFW_MOUSE_BUTTON_RIGHT) == 1)
	{
		float dx = 0.01f * ((float)posX - lastMousePos.x);
		float dy = 0.01f * ((float)posY - lastMousePos.y);
		gRadius += dx - dy; // Adjust radius based on mouse movement
	}

	lastMousePos.x = (float)posX;
	lastMousePos.y = (float)posY;
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