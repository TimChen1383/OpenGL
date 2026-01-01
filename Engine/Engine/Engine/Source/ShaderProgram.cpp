#include "ShaderProgram.h"
#include <fstream> 
#include <iostream>
#include <sstream>
#include "glm/gtc/type_ptr.hpp"

ShaderProgram::ShaderProgram()
    : mHandle(0)
{
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(mHandle);
}

bool ShaderProgram::loadShaders(const char* VertexShaderFilename, const char* FragmentShaderFilename)
{
    string vsString = FileToString(VertexShaderFilename);
    string fsString = FileToString(FragmentShaderFilename);
    const GLchar* vsSourcePtr = vsString.c_str();
    const GLchar* fsSourcePtr = fsString.c_str();

    
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

    GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(VertexShader, 1, &vsSourcePtr, NULL); // Set the source code for the vertex shader
    glShaderSource(FragmentShader, 1, &fsSourcePtr, NULL);

    glCompileShader(VertexShader);
    CheckCompileErrors(VertexShader, VERTEX);
    glCompileShader(FragmentShader);
    CheckCompileErrors(FragmentShader, FRAGMENT);
    
 

    //Create, Attach and Link Shader Program
    mHandle = glCreateProgram();
    glAttachShader(mHandle, VertexShader);
    glAttachShader(mHandle, FragmentShader);
    glLinkProgram(mHandle);
    
    CheckCompileErrors(mHandle, PROGRAM);
    
    //Delete the shaders after linking
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return true;
}

void ShaderProgram::use()
{
    if (mHandle > 0)
    {
        glUseProgram(mHandle); // Use the shader program for rendering
    }
    
}

void ShaderProgram::setUniform(const GLchar* name, const glm::vec2& v)
{
    GLint loc = getUniformLocation(name);
    glUniform2f(loc, v.x, v.y);
}

void ShaderProgram::setUniform(const GLchar* name, const glm::vec3& v)
{
    GLint loc = getUniformLocation(name);
    glUniform3f(loc, v.x, v.y, v.z);
}

void ShaderProgram::setUniform(const GLchar* name, const glm::vec4& v)
{
    GLint loc = getUniformLocation(name);
    glUniform4f(loc, v.x, v.y, v.z, v.w);
}

void ShaderProgram::setUniform(const GLchar* name, const glm::mat4& m)
{
    GLint loc = getUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderProgram::setUniform(const GLchar* name, const GLfloat f)
{
    GLint loc = getUniformLocation(name);
    glUniform1f(loc, f);
}

void ShaderProgram::setUniform(const GLchar* name, const GLint i)
{
    GLint loc = getUniformLocation(name);
    glUniform1i(loc, i);
}

GLint ShaderProgram::getUniformLocation(const GLchar* name)
{
    std::map<string, GLint>::iterator it = mUniformLocations.find(name);

    if (it == mUniformLocations.end())
    {
        mUniformLocations[name] = glGetUniformLocation(mHandle, name);
    }
    return mUniformLocations[name];
}

string ShaderProgram::FileToString(const string& filename)
{
    std::stringstream ss; //create a string stream to hold the file content
    std::ifstream file; //open and read files

    try
    {
        file.open(filename, std::ios::in);
        if (!file.fail())
        {
            //store the file content in the string stream
            ss << file.rdbuf();
        }
        file.close();
    }
    catch (std::exception ex)
    {
        std::cout << "Error reading shader file"<< std::endl;
    }
    //return stored content 
    return ss.str();
}

void ShaderProgram::CheckCompileErrors(GLuint shader, ShaderType type)
{
    int status = 0;

    //Use functions to check for compile errors and return error messages
    if (type == PROGRAM)
    {
        glGetProgramiv(mHandle, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint length = 0;
            glGetProgramiv(mHandle, GL_INFO_LOG_LENGTH, &length);

            string errorLog(length, ' ');
            glGetProgramInfoLog(mHandle, length, &length, &errorLog[0]);
            std::cout << "SHADER PROGRAM LINK ERROR: " << errorLog << std::endl;
            std::cerr << "SHADER PROGRAM LINK ERROR: " << errorLog << std::endl;
        }
        else
        {
            std::cout << "Shader program linked successfully." << std::endl;
        }
    }
    else //VERTEX or FRAGMENT
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            string errorLog(length, ' ');
            glGetShaderInfoLog(shader, length, &length, &errorLog[0]);
            string shaderTypeStr = (type == VERTEX) ? "VERTEX" : "FRAGMENT";
            std::cout << "SHADER COMPILE ERROR (" << shaderTypeStr << "): " << errorLog << std::endl;
            std::cerr << "SHADER COMPILE ERROR (" << shaderTypeStr << "): " << errorLog << std::endl;
        }
        else
        {
            string shaderTypeStr = (type == VERTEX) ? "VERTEX" : "FRAGMENT";
            std::cout << shaderTypeStr << " shader compiled successfully." << std::endl;
        }
    }
}
