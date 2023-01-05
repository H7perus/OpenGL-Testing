#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D shadowMap;

struct Material {
	vec3 ambient;
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};
struct pointLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
struct dirLight {
	vec3 position;
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
struct spotLight {
	vec3 position;
	vec3 direction;
	float cone;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform Material material;

#define NR_POINT_LIGHTS 2  
uniform pointLight pointLights[NR_POINT_LIGHTS];

uniform dirLight sun;
float shadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = max(0.05 * (1.0 - dot(fs_in.Normal, sun.direction)), 0.005);  
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
	if(projCoords.z > 1.0)
        shadow = 0.0;
	return shadow;
}  
vec3 calcPointLight(pointLight light)
{
	//ambient lighting
	vec3 ambient = light.ambient;

	//specular lighting
	vec3 norm = normalize(fs_in.Normal);
	vec3 lightDir = normalize(light.position - fs_in.FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * light.diffuse;

	// specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = spec * light.specular;
	//return vec3(1.0);
	return (ambient + diffuse + specular) * (1 / ( 1 + pow((distance(fs_in.FragPos, light.position)), 2)));
}
vec3 calcSpotLight(spotLight light)
{
	float theta = dot(normalize(light.position - fs_in.FragPos), normalize(light.direction));
	vec3 ambient = vec3(0.0);
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);
	if(theta > light.cone)
	{
	//ambient lighting
		ambient = vec3(texture(material.diffuse, fs_in.TexCoords)) * light.ambient;
		//specular lighting
		vec3 norm = normalize(fs_in.Normal);
		vec3 lightDir = normalize(light.position - fs_in.FragPos);
		float diff = max(dot(norm, lightDir), 0.0);
		diffuse = (diff * vec3(texture(material.diffuse, fs_in.TexCoords))) * light.diffuse;

	// specular
		vec3 viewDir = normalize(viewPos - fs_in.FragPos);
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		specular = vec3(texture(material.specular, fs_in.TexCoords)) * spec * light.specular;
	}
	return (ambient + diffuse + specular) * (1 / ( 1 + pow((distance(fs_in.FragPos, light.position)), 2)));
}
vec3 calcDirLight(dirLight light)
{
		//ambient lighting
	vec3 ambient = light.ambient;

	//specular lighting
	vec3 norm = normalize(fs_in.Normal);
	vec3 lightDir = normalize(light.direction);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = (diff) * light.diffuse;

	// specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = spec * light.specular;
	return (diffuse + specular) * (1.0 - shadowCalculation(fs_in.FragPosLightSpace)) + ambient;
}

void main()
{
	vec3 result;
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		result += calcPointLight(pointLights[i]);
	}
	result += calcDirLight(sun);
	 
	FragColor = vec4(result, 0.5);
	//FragColor = vec4(1.0);
	//FragColor = mix(texture(albedo, TexCoords), texture(specular, TexCoords), 0.5);
	//FragColor = vec4(ambient, 0);
}