// This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <sstream>
#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"

//Global variables
const char* APP_Title = "OpenGL Application";
const int gWindowWidth = 800;
const int gWindowHeight = 600;
GLFWwindow* gwindow = nullptr; 

//Create simple shader
const GLchar* vertexShaderSrc =
"#version 330 core\n"
"layout(location = 0) in vec3 pos;"
"layout (location = 1) in vec3 color;"
"out vec3 vert_color;"
"void main()"
"{"
"	vert_color = color;"
"   gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);"
"}";

const GLchar* fragmentShaderSrc =
"#version 330 core\n"
"in vec3 vert_color;"
"out vec4 frag_color;"
"void main()"
"{"
"    frag_color = vec4(vert_color, 1.0f); "
"}";

//Custom Functions
void glfw_OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
void showFPS(GLFWwindow* window);
bool InitOpenGL();


int main()
{	
	// Initialize OpenGL
	if(!InitOpenGL())
	{
		std::cerr << "OpenGL initialization failed." << std::endl;
		return -1; 
	}

	//Vertices for a simple triangle
	GLfloat vertices[] = 
	{
		// Positions          //Color
		-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,// Bottom left
		 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,// Bottom right
		 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f// Top
	};

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, NULL);
	glEnableVertexAttribArray(0);

	//Color vertex attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (GLvoid*)(sizeof(GLfloat)*3));
	glEnableVertexAttribArray(1);

	/*
	////////////////////////////////////////////////////////////////////////
	Basic Steps for creating shaders
	1.Create Vertex Shader
	2.Create Fragment Shader
	3.Compile Shaders
	4.Create Shader Program
	5.Attach Shaders to Program
	6.Link Shader Program
	7.Use the Program
	////////////////////////////////////////////////////////////////////////
	*/

	// Create and compile the vertex shader
	GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER); 
	glShaderSource(VertexShader, 1, &vertexShaderSrc, NULL); // Set the source code for the vertex shader
	glCompileShader(VertexShader);
	GLint result;
	GLchar infoLog[512];
	glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(VertexShader, sizeof(infoLog), NULL, infoLog);
		std::cerr << "Vertex Shader Compilation Failed: " << infoLog << std::endl;
		return -1; 
	}

	// Create and compile the fragment shader
	GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &fragmentShaderSrc, NULL);
	glCompileShader(FragmentShader);
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(FragmentShader, sizeof(infoLog), NULL, infoLog);
		std::cerr << "Fragment Shader Compilation Failed: " << infoLog << std::endl;
		return -1; 
	}

	//Create, Attach and Link Shader Program
	GLuint shaderProgram = glCreateProgram(); 
	glAttachShader(shaderProgram, VertexShader);
	glAttachShader(shaderProgram, FragmentShader);
	glLinkProgram(shaderProgram); 
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result); 
	if (!result)
	{
		glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
		std::cerr << "Shader Program Linking Failed: " << infoLog << std::endl;
		return -1;
	}

	//Delete the shaders after linking
	//Once the shaders are linked into a shader program , their source code and compiled objects are embedded into the program
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);

	//Main Loop
    while (!glfwWindowShouldClose(gwindow))
	{
		showFPS(gwindow); // Show FPS in the console and window title
		glClearColor(0.25f, 0.38f, 0.47f, 1.0f); // Set the clear color (background color)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen
	
		
		glUseProgram(shaderProgram); // Use the shader program for rendering
		glBindVertexArray(VAO); // Bind the VAO to use the vertex attributes
		glDrawArrays(GL_TRIANGLES, 0, 3); // Draw the triangle using the vertex data in the VBO
		glBindVertexArray(0); // Unbind the VAO

		// Swap buffers. The order is very important
		glfwSwapBuffers(gwindow); // Swap buffers to display the rendered content
		glfwPollEvents(); // Poll for events (like keyboard and mouse input)
	
	}

	// Delete the shader program after use
	glDeleteProgram(shaderProgram); 
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
		outs <<std::fixed
			<< APP_Title << " "	
			<< "FPS: " << fps <<" "
			<< "Frame Time" << msPerframe << " (ms)";
		glfwSetWindowTitle(window, outs.str().c_str());

		frameCount = 0;
	}
	frameCount++;
}