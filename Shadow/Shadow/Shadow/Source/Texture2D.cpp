#include "Texture2D.h"
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include "stb_image/stb_image.h"

Texture2D::Texture2D()
    :mTexture(0) // Constructor. Initialize mTexture to 0
{
}

Texture2D::~Texture2D()
{
}

bool Texture2D::loadTexture(const string& filename, bool generateMipMaps)
{
    //Loading image using custom library
    int width, height, components;
    unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &components, STBI_rgb_alpha);
    if (imageData == NULL)
    {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return false;
    }

    //invert image
    int widthInBytes = width * 4;
    unsigned char* top = NULL;
    unsigned char* bottom = NULL;
    unsigned char temp = 0;
    int halfHeight = height / 2;
    for (int row = 0; row < halfHeight; row++)
    {
        top = imageData + row * widthInBytes; // Pointer to the current row from the top
        bottom = imageData + (height - row - 1) * widthInBytes; // Pointer to the current row from the bottom

        // Swap the rows
        for (int col = 0; col < widthInBytes; col++)
        {
            temp = *top;
            *top = *bottom;
            *bottom = temp;
            top++;
            bottom++;
        }
    }

    //Create OpenGL texture
    glGenTextures(1,&mTexture);
    glBindTexture(GL_TEXTURE_2D,mTexture); //We need to bind the texture we are using before setting parameters
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); //left right direction
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); //up down direction
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //if texture is larger than the mapping area
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //if texture is smaller than the mapping area

    //Mapping the loaded image data to the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    //Setup mipmapping if requested
    if (generateMipMaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    //Free the image data and Unbind the texture after loaded into OpenGL
    stbi_image_free(imageData);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return true;
}

void Texture2D::bindTexture(GLuint textureUnit)
{
    glActiveTexture(GL_TEXTURE0 + textureUnit); 
    glBindTexture(GL_TEXTURE_2D,mTexture);
}

void Texture2D::unbindTexture(GLuint textureUnit)
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D,0);
}