#version 330 core

//Pass the data in. Then we modify it
//The position data is stored inside the first vertex attribute array slot(0)
layout(location = 0) in vec3 pos;

//If we use "uniform" instaed of "in""out"we can pass values directly into fragment shader
//No need to pass into vertex shader, then pass into fragment shader
uniform vec2 posOffset;

void main()
{
   gl_Position = vec4(pos.x + posOffset.x, pos.y + posOffset.y, pos.z, 1.0);
}