#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirLightSpaceMatrix;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 FragPosDirLightSpace;

void main()
{
   Normal = mat3(transpose(inverse(model))) * normal;
   FragPos = vec3(model * vec4(pos, 1.0));
   FragPosDirLightSpace = dirLightSpaceMatrix * vec4(FragPos, 1.0);
   gl_Position = projection * view * model * vec4(pos, 1.0);
   TexCoord = texCoord * 2;
}
