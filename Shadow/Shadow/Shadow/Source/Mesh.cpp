#include "Mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>


std::vector<std::string> split(std::string& s, std::string t)
{
    std::vector<std::string> result;
    size_t pos = 0;
    while ((pos = s.find(t)) != std::string::npos)
    {
        result.push_back(s.substr(0, pos));
        s = s.substr(pos + t.length()); // Move past the delimiter
    }
    result.push_back(s); // Add the remaining string after the last delimiter
    return result;
}


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
        
        std::string lineBuffer; // temporary string to hold "each line" read from the file
        while (std::getline(fin, lineBuffer)) // reading line by line in the file
        {
            std::stringstream ss(lineBuffer); //Each line will create a string stream to manipulate strings
            std::string cmd; // temporary string for processing
            ss >> cmd; // The "first word" (divided by empty space) in the lineBuffer will be extracted into the cmd variable
            
            //checking every line and see if it includes the vertex position, texture coordinate or face
            if (cmd == "v") //Vertex
            {
                glm::vec3 vertex;
                int dim = 0;
                // extract next 3 float value in ss and store them into vector
                //The value in ss will be gone after being extracted
                while (dim < 3 && ss >> vertex[dim]) 
                    dim++;

                tempVertices.push_back(vertex);//assign back to vertex value
            }
            else if (cmd == "vt") //UV
            {
                glm::vec2 uv;
                int dim = 0;
                while (dim < 2 && ss >> uv[dim])
                    dim++;
                
                tempUVs.push_back(uv);//assign back to uv value
            }
            else if (cmd == "vn") //Normal
            {
                glm::vec3 normal;
                int dim = 0;
                while (dim < 3 && ss >> normal[dim])
                    dim++;

                normal = glm::normalize(normal);
                tempNormals.push_back(normal);//assign back to normal value
            }
            else if (cmd == "f") // Face
            {
                std::string faceData;
                int vertexIndex, uvIndex, normalIndex; // v/vt/vn

                while (ss >> faceData)
                {
                    std::vector<std::string> data = split(faceData, "/");

                    //vertex index
                    if (data[0].size() > 0)
                    {
                        sscanf_s(data[0].c_str(), "%d", &vertexIndex);
                        vertexIndices.push_back(vertexIndex);
                    }

                    //if the face vertex has a texture coordinate index
                    if (data.size() >= 1)
                    {
                        if (data[1].size() > 0)
                        {
                            sscanf_s(data[1].c_str(), "%d", &uvIndex);
                            uvIndices.push_back(uvIndex);
                        }
                    }

                    //does the face vertex have normal index
                    if (data.size() >= 2)
                    {
                        if (data[2].size() > 0)
                        {
                            sscanf_s(data[2].c_str(), "%d", &normalIndex);
                            normalIndices.push_back(normalIndex);
                        } 
                    }
                }
            }
        }
        

        // close the file after reading
        fin.close();
        
        //For each vertex of each triangle, set back the value
        for (unsigned int i = 0; i < vertexIndices.size(); i++)
        {
            Vertex meshVertex;

            if (tempVertices.size() > 0)
            {
                glm::vec3 vertex = tempVertices[vertexIndices[i] - 1];//C++ start from 0
                meshVertex.position = vertex;
            }
            if (tempNormals.size() > 0)
            {
                glm::vec3 normal = tempNormals[normalIndices[i] - 1];
                meshVertex.normal = normal;
            }
            if (tempUVs.size() > 0)
            {
                glm::vec2 uv = tempUVs[uvIndices[i] - 1];
                meshVertex.texCoords = uv;
            }
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
    // Generate and bind Vertex Buffer Object (VBO)
    //A VBO is a memory buffer in the GPU that stores vertex data (e.g., positions, colors, normals)
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), &mVertices[0], GL_STATIC_DRAW);

    // Generate and bind Vertex Array Object (VAO)
    //A VAO is an OpenGL object that stores the configuration of vertex attributes.It simplifies the process of switching between different vertex configurations.Related to VBO
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
    glEnableVertexAttribArray(0);

    //Normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    //Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(6 * sizeof(GLfloat))); 
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // Unbind the VAO. We are done
}
