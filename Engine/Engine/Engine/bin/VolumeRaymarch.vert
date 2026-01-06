#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 fragPos;
out vec3 localPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Local position for volume UV calculation
    localPos = aPos;

    // World position for raymarching
    vec4 worldPos = model * vec4(aPos, 1.0);
    fragPos = worldPos.xyz;

    gl_Position = projection * view * worldPos;
}
