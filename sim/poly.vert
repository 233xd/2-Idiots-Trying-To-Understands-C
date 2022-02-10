#version 330 core

in vec2 position;
uniform vec2 screenSize;
uniform vec3 wallColor;

out vec3 color;

void main()
{
	gl_Position = vec4((position / screenSize) * 2 - 1, 0, 1);
	color = wallColor;
}
