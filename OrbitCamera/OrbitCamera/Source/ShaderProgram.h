//include guard
#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <GL/glew.h>
#include <string>
#include "glm/glm.hpp"
#include <map>
using std::string;

class ShaderProgram
{
public:
    //Constructor
    ShaderProgram();
    //Destructor
    ~ShaderProgram();

    enum ShaderType
    {
        VERTEX,
        FRAGMENT,
        PROGRAM
    };

    //Load shaders from extra files in this project
    bool loadShaders(const char* VertexShaderFilename, const char* FragmentShaderFilename);

    //Activate the shader program
    void use();
    
    void setUniform(const GLchar* name, const glm::vec2& v);
    void setUniform(const GLchar* name, const glm::vec3& v);
    void setUniform(const GLchar* name, const glm::vec4& v);
    void setUniform(const GLchar* name, const glm::mat4& m);


private:
    string FileToString(const string& filename);
    void CheckCompileErrors(GLuint shader, ShaderType type);
    GLint getUniformLocation(const GLchar* name);
    
    GLuint mHandle;
    std::map<string, GLint> mUniformLocations;

};
#endif// SHADER_PROGRAM_H
