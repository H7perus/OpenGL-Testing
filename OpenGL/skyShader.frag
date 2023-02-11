#version 330 core
in vec4 FragPos;
out vec4 FragColor;

uniform mat4 view;
uniform mat4 projection;

uniform vec3 sunDir;


void main()
{
    vec4 FragDirection = inverse(projection) * FragPos;

    vec3 worldDirection = normalize(inverse(mat3(view)) * vec3(FragDirection));

    FragColor = vec4(0.73 - 0.2 * pow(max(worldDirection.y, 0), 0.5), 0.9 - 0.1 * pow(max(worldDirection.y, 0), 0.5), 0.7 + 0.22 * pow(max(worldDirection.y, 0), 0.5), 1.0);
    vec4 sunLight = vec4(1.0) * pow(1 / (acos(dot(worldDirection, normalize(sunDir))) * 40), 3);
    FragColor = FragColor + sunLight;
}