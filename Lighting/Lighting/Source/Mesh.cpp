#include "Mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>

Mesh::Mesh()
    :mLoaded(false)
{
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
}

bool Mesh::loadOBJ(const std::string& filename)
{
    //temporary container when we are reading the file
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> tempVertices;
    std::vector<glm::vec2> tempUVs;
    std::vector<glm::vec3> tempNormals;


    if (filename.find(".obj") != std::string::npos)
    {
        //try open the file
        std::ifstream fin(filename, std::ios::in);
        if (!fin)
        {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }
        std::cout << "Loading OBJ file: " << filename << std::endl;

        std::string lineBuffer; // temporary string to hold each line read from the file
        while (std::getline(fin, lineBuffer)) // reading line by line in the file
        {
            //checking every line and see if it includes the vertex position, texture coordinate or face
            if (lineBuffer.substr(0,2) == "v ") 
            {
                std::istringstream v(lineBuffer.substr(2));
                //store the vertex data from the file
                glm::vec3 vertex; 
                v >> vertex.x; v >> vertex.y; v >> vertex.z;
                tempVertices.push_back(vertex);
            }
            else if (lineBuffer.substr(0,2) == "vt") // looking for Texture coordinate
            {
                std::istringstream vt(lineBuffer.substr(3));
                //store the uv data from the file
                glm::vec2 uv;
                vt >> uv.s; vt >> uv.t;
                tempUVs.push_back(uv);
            }
            else if (lineBuffer.substr(0,2) == "f ") // Face
            {
                int p1, p2, p3; //to store mesh index
                int t1, t2, t3; //to store texture index
                int n1, n2, n3;

                const char* face = lineBuffer.c_str();
                //check the format of the face line
                int match = sscanf_s(face, "f %i/%i/%i %i/%i/%i %i/%i/%i",
                    &p1, &t1, &n1,
                    &p2, &t2, &n2,
                    &p3, &t3, &n3);
                if (match != 9)
                    std::cout <<"Failed to parse OBJ file using our simple OBJ loader" << std::endl;

                vertexIndices.push_back(p1);
                vertexIndices.push_back(p2);
                vertexIndices.push_back(p3);
                uvIndices.push_back(t1);
                uvIndices.push_back(t2);
                uvIndices.push_back(t3);
            }
        }

        // close the file after reading
        fin.close();

        //Use stored temporary data to create actual data
        //For each vertex of each triangle 
        for (unsigned int i = 0; i < vertexIndices.size(); i++)
        {
            glm::vec3 vertex = tempVertices[vertexIndices[i] - 1];//C++ start from 0
            glm::vec2 uv = tempUVs[uvIndices[i] - 1];

            //Set back the value
            Vertex meshVertex;
            meshVertex.position = vertex; 
            meshVertex.texCoords = uv;

            mVertices.push_back(meshVertex);
        }

        initBuffer();
        return (mLoaded = true); 
    }
    
    //immediately return if the file is not OBJ
    return false;
}

void Mesh::draw()
{
    if (!mLoaded) return;

    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLES, 0, mVertices.size());
    glBindVertexArray(0); // Unbind the VAO after drawing
}

void Mesh::initBuffer()
{
    // Generate and bind Vertex Array Object (VAO)
    //A VAO is an OpenGL object that stores the configuration of vertex attributes.It simplifies the process of switching between different vertex configurations.Related to VBO
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    // Generate and bind Vertex Buffer Object (VBO)
    //A VBO is a memory buffer in the GPU that stores vertex data (e.g., positions, colors, normals)
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);

    // Position vertex attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(0);

    //Texture coordinate vertex attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); // Offset by 3 floats for position
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind the VAO. We are done
}
