#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>

#include "GL/glew.h"
#include "glm/glm.hpp"

struct Vertex
{
    glm::vec3 position; // Vertex position
    glm::vec2 texCoords; // Texture coordinates
};


class Mesh
{
public:
    Mesh();
    ~Mesh();

    bool loadOBJ(const std::string& filename);
    void draw();

private:

    void initBuffer();
    
    bool mLoaded;
    std::vector<Vertex> mVertices;// store collections elements(vertex structure) of the same data type
    GLuint mVBO, mVAO;
    
};



#endif
