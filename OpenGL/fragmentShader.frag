#version 330 core
out vec4 FragColor;

in vec3 customColor;
in vec2 TexCoord;

uniform sampler2D albedo0;
uniform sampler2D albedo1;

void main()
{
   FragColor = mix(texture(albedo0, TexCoord), texture(albedo1, TexCoord), 0.2);
}