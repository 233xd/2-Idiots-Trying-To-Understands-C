#version 330 core

in vec2 position;
uniform vec2 screenSize;

void main()
{
	gl_PointSize = 5;
	gl_Position = vec4((position / screenSize) * 2 - 1, 0, 1);
}
