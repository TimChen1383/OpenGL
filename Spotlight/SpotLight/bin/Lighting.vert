#version 330 core

//Pass the data in. Then we modify it
//The position data is stored inside the first vertex attribute array slot(0)
layout(location = 0) in vec3 pos;

//Normal data
layout(location = 1) in vec3 normal;

//The UV data is stored inside the second vertex attribute array slot(2)
layout(location = 2) in vec2 texCoord;

uniform mat4 model; //Model matrix for object
uniform mat4 view;  //View matrix for camera
uniform mat4 projection; //Projection matrix for camera


out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;


void main()
{
   Normal = normal; 
   FragPos = vec3(model * vec4(pos, 1.0)); //Transform position to world space
   gl_Position = projection * view * model * vec4(pos, 1.0); // Transform position to clip space
   TexCoord = texCoord*2;
}