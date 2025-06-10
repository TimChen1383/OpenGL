// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"

//Global variables
const char* APP_Title = "OpenGL Application";
const int gWindowWidth = 800;
const int gWindowHeight = 600;

int main()
{	
	// Initialize GLFW. Neccessary
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* pwindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_Title, NULL, NULL);
	if (pwindow == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate(); //Shutdown GLFW properly
		return -1;
	}

	glfwMakeContextCurrent(pwindow);
	glewExperimental = GL_TRUE; // Enable experimental features for GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		glfwDestroyWindow(pwindow);
		glfwTerminate(); //Shutdown GLFW properly
		return -1;
	}

	//Main Loop
    while (!glfwWindowShouldClose(pwindow))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen
		// Render your OpenGL content here
		glfwSwapBuffers(pwindow); // Swap buffers to display the rendered content
		glfwPollEvents(); // Poll for events (like keyboard and mouse input)
	}
	glfwTerminate();
    return 0;
}

