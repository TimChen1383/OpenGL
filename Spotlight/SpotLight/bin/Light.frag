#version 330 core

in vec2 TexCoord;

//We modify the value and pass it out
out vec4 frag_color;

uniform vec3 lightColor;

void main()
{
	//map the color from loaded image to the texture
	frag_color = vec4(lightColor, 1.0f);
}