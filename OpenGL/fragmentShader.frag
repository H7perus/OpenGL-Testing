#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

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



vec3 calcPointLight(pointLight light)
{
	//ambient lighting
	vec3 ambient = vec3(texture(texture_diffuse1, TexCoords)) * light.ambient;

	//specular lighting
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.position - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = (diff * vec3(texture(texture_diffuse1, TexCoords))) * light.diffuse;

	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = vec3(texture(texture_specular1, TexCoords)) * spec * light.specular;
	//return vec3(1.0);
	return (diffuse + specular + ambient) * (1 / ( 1 + pow((distance(FragPos, light.position)), 2)));
}
vec3 calcSpotLight(spotLight light)
{
	float theta = dot(normalize(light.position - FragPos), normalize(light.direction));
	vec3 ambient = vec3(0.0);
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);
	if(theta > light.cone)
	{
	//ambient lighting
		ambient = vec3(texture(material.diffuse, TexCoords)) * light.ambient;
		//specular lighting
		vec3 norm = normalize(Normal);
		vec3 lightDir = normalize(light.position - FragPos);
		float diff = max(dot(norm, lightDir), 0.0);
		diffuse = (diff * vec3(texture(material.diffuse, TexCoords))) * light.diffuse;

	// specular
		vec3 viewDir = normalize(viewPos - FragPos);
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		specular = vec3(texture(material.specular, TexCoords)) * spec * light.specular;
	}
	return (ambient + diffuse + specular) * (1 / ( 1 + pow((distance(FragPos, light.position)), 2)));
}
vec3 calcDirLight(dirLight light)
{
		//ambient lighting
	vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords));

	//specular lighting
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.direction);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = (diff) * light.diffuse * vec3(texture(texture_diffuse1, TexCoords));

	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = spec * light.specular * vec3(texture(texture_specular1, TexCoords));
	return (ambient + diffuse + specular);
}


void main()
{
	vec3 result;
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
		result += calcPointLight(pointLights[i]);
	}
	result += calcDirLight(sun);
	 
	FragColor = vec4(result, 1.0);
	//FragColor = vec4(1.0);
	//FragColor = mix(texture(albedo, TexCoords), texture(specular, TexCoords), 0.5);
	//FragColor = vec4(ambient, 0);
}