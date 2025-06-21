#version 330 core

uniform vec4 vertColor;
//We modify the value and pass it out
out vec4 frag_color;


void main()
{
	frag_color = vertColor;
}