#version 330 core
out vec4 FragColor;
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

const int numOfAvailableDecals = 5;
const int numOfMaterials = 5;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D main_albedo;
uniform sampler2D subtex_land;
uniform sampler2D subtex_water;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_diffuse3;
uniform sampler2D texture_decal;
uniform sampler2D splatMap;



uniform sampler2D shadowMap;
uniform int numOfDecals;
struct Decal {
	sampler2D albedo;
	sampler2D specular;
	sampler2D splatMaps[numOfMaterials];
	mat4 decalTransform;
};
uniform Decal decals[numOfAvailableDecals];


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

uniform  int numOfPointLights = 0;
vec3 norm = normalize(fs_in.Normal);
uniform pointLight pointLights[100];
uniform dirLight sun;
uniform float shininess;


vec3 albedo;

vec4 decalCalculation(Decal decal)
{
	vec4 decalSpace = decal.decalTransform * vec4(fs_in.FragPos, 1.0);
    vec3 projCoords = decalSpace.xyz / decalSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    return texture(decal.albedo, projCoords.xy).rgba;
}

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
    //float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;
	float bias = max(0.01 * (1.0 - dot(fs_in.Normal, sun.direction)), 0.005);  
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
	if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}  

vec3 calcPointLight(pointLight light)
{
	vec3 ambient = albedo * light.ambient;

	// diffuse
	vec3 lightDir = normalize(light.position - fs_in.FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = albedo * light.diffuse * diff;

	// specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = spec * light.specular * albedo;
	//return vec3(1.0);
	return (diffuse + specular + ambient) * (1 / ( 1 + pow((distance(fs_in.FragPos, light.position)), 2)));
}
//vec3 calcSpotLight(spotLight light)
//{
//	float theta = dot(normalize(light.position - fs_in.FragPos), normalize(light.direction));
//	vec3 ambient = vec3(0.0);
//	vec3 diffuse = vec3(0.0);
//	vec3 specular = vec3(0.0);
//	if(theta > light.cone)
//	{
//	//ambient lighting
//		ambient = vec3(texture(material.diffuse, fs_in.TexCoords)) * light.ambient;
//		//specular lighting
//		vec3 norm = normalize(fs_in.Normal);
//		vec3 lightDir = normalize(light.position - fs_in.FragPos);
//		float diff = max(dot(norm, lightDir), 0.0);
//		diffuse = (diff * vec3(texture(material.diffuse, fs_in.TexCoords))) * light.diffuse;
//
//	// specular
//		vec3 viewDir = normalize(viewPos - fs_in.FragPos);
//		vec3 reflectDir = reflect(-lightDir, norm);
//		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
//		specular = vec3(texture(material.specular, fs_in.TexCoords)) * spec * light.specular;
//	}
//	return (ambient + diffuse + specular) * (1 / ( 1 + pow((distance(fs_in.FragPos, light.position)), 2)));
//}
vec3 calcDirLight(dirLight light)
{
		//ambient lighting
	vec3 ambient = light.ambient * albedo;

	//specular lighting
	vec3 norm = normalize(fs_in.Normal);
	vec3 lightDir = normalize(light.direction);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = (diff) * light.diffuse * albedo;

	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = spec * light.specular * (vec3(texture(splatMap, fs_in.TexCoords)));
	//vec3 specular = vec3(0);
	return diffuse * (1.0 - shadowCalculation(fs_in.FragPosLightSpace)) + ambient; //(diffuse + specular) * (1.0 - shadowCalculation(fs_in.FragPosLightSpace)) 
}

void main()
{

	vec3 texture_ground = vec3(texture(main_albedo, fs_in.TexCoords * 1))* vec3(texture(subtex_land, fs_in.TexCoords * 1000));
	vec3 texture_water =  vec3(texture(main_albedo, fs_in.TexCoords * 1))* vec3(texture(subtex_water, fs_in.TexCoords * 1000));

	albedo = mix(texture_ground, texture_water, 1 - vec3(texture(splatMap, fs_in.TexCoords)).r);
	for(int i = 0; i < numOfDecals; i++)
	{
		vec4 airfield_decal = decalCalculation(decals[i]);
		if(airfield_decal != vec4(0))
			albedo = mix(albedo, airfield_decal.rgb, airfield_decal.a);
	}
	
	vec3 result = vec3(0);
//	for(int i = 0; i < numOfPointLights; i++)
//	{
//		result += calcPointLight(pointLights[i]);
//	}

	result += calcDirLight(sun);
	
	//result = texture_ground;
	FragColor = vec4(result, 1.0);
}