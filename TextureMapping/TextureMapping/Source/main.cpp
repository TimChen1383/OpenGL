// This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <sstream>
#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "ShaderProgram.h"
#include "Texture2D.h"

//Global variables
const char* APP_Title = "OpenGL Application";
const int gWindowWidth = 800;
const int gWindowHeight = 600;
GLFWwindow* gwindow = nullptr;
const std::string texture1 = "MyGame.jpg"; 

//Custom Functions
void glfw_OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
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
		-0.7f, 0.6f, 0.0f, 0.0f, 1.0f, // Top-left corner
		 0.7f, 0.6f, 0.0f, 1.0f, 1.0f, // Top-right corner
		 0.7f, -0.6f, 0.0f, 1.0f, 0.0f, // Bottom-right corner
		-0.7f, -0.6f, 0.0f, 0.0f, 0.0f // Bottom-left corner
	};

	GLuint indices[] =
	{
		0, 1, 2, // First triangle
		0, 2, 3  // Second triangle
	};

	GLuint VBO, VAO, IBO;

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

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//Create our custom class
	ShaderProgram shaderProgram;
	// Load shaders from files. Text file
	shaderProgram.loadShaders("Basic.vert", "Basic.frag"); 

	// Load texture from file
	Texture2D texture;
	texture.loadTexture(texture1, true); 
	
	
	//Main Loop
	while (!glfwWindowShouldClose(gwindow))
	{
		showFPS(gwindow); // Show FPS in the console and window title
		glClearColor(0.25f, 0.38f, 0.47f, 1.0f); // Set the clear shaderProgram.setUniform("vertColor", glm::vec4(0.0f, 0.0f, blueColor, 1.0f));shaderProgram.setUniform("vertColor", glm::vec4(0.0f, 0.0f, blueColor, 1.0f));color (background color)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen

		texture.bindTexture();
		
		shaderProgram.use();
		
		glBindVertexArray(VAO); // Bind the VAO to use the vertex attributes
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0); // Unbind the VAO

		// Swap buffers. The order is very important
		glfwSwapBuffers(gwindow); // Swap buffers to display the rendered content
		glfwPollEvents(); // Poll for events (like keyboard and mouse input)

	}

	// Delete the shader program after use
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &IBO);

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
	glewExperimental = GL_TRUE; // Enable experimental features for GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		glfwDestroyWindow(gwindow);
		glfwTerminate(); //Shutdown GLFW properly
		return false;
	}
	return true; // Return true if OpenGL initialization is successful
}

void glfw_OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true); // Close the window when Escape is pressed
	}

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