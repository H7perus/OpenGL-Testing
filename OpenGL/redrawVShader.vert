#version 330 core
layout (location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 customColor;
out vec2 TexCoord;

void main()
{
	vec3 inPos = aPos;
	gl_Position = vec4(inPos, 1.0);
	customColor = aColor;
	TexCoord = aTexCoord;
}