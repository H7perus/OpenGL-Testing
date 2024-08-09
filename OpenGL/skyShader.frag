#version 330 core
in vec4 FragPos;
out vec4 FragColor;

uniform mat4 view;
uniform mat4 projection;

uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 cameraPos;

vec3 sphere_intersection(float radius, vec3 direction, vec3 position);

void main()
{
    vec4 FragDirection = inverse(projection) * FragPos;

    vec3 worldDirection = normalize(inverse(mat3(view)) * vec3(FragDirection));


    //worldDirection = sphere_intersection(1000., worldDirection, cameraPos);


    FragColor = vec4(0.73 - 0.2 * pow(max(worldDirection.y, 0), 0.5), 0.9 - 0.1 * pow(max(worldDirection.y, 0), 0.5), 0.7 + 0.22 * pow(max(worldDirection.y, 0), 0.5), 1.0);
    vec4 sunLight = vec4(sunColor, 1.0) * pow(1 / (acos(dot(worldDirection, normalize(sunDir))) * 40), 3);
    FragColor = FragColor + sunLight;
    //FragColor.rgb = worldDirection;
}



vec3 sphere_intersection(float radius, vec3 direction, vec3 position)
{
    vec3 dir_to_center = normalize(-position);
    float c = length(position);
    float a = sqrt(pow(c, 2) - pow(c * (dot(direction, dir_to_center)), 2));
    float b = sqrt( pow(c, 2) - pow(a, 2) );

    if(dot(direction, dir_to_center) < 0   && c > radius)
    {
        return vec3(0);
    }
    if(a < radius)
    {
        vec3 intermediate_position = direction * b + cameraPos;
        vec3 add = sqrt(pow(radius, 2) - pow(a, 2)) * direction;
        return (intermediate_position + add) / radius;
    }
    return vec3(0);
}