#version 460 core
layout	(location = 0) in vec3 aPos;
layout	(location = 1) in vec3 aNormal;
layout	(location = 2) in vec2 aTexCoord;
layout	(location = 3) in vec3 aTangent;
layout	(location = 4) in vec3 aBitangent;
layout	(location = 5) in mat4 instanceModelMat;
layout	(location = 6) in dvec4 PosRot;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
	vec3 tangent, bitangent;
} vs_out;

uniform dvec3 cameraPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform int Instanced;
//uniform mat4 decal_Mat;

void main()
{
	mat4 modelMat;
	if(Instanced == 1)
	{
		modelMat = instanceModelMat;
	}
	else if(Instanced == 2)
	{
		modelMat = mat4(1.0);
		

		vec2 direction = normalize(vec2(PosRot.xz - cameraPos.xz));

		if(PosRot.w < 0)
		{
			modelMat[0][0] = -direction.x;
			modelMat[0][2] = -direction.y;
			modelMat[2][0] = -direction.y;
			modelMat[2][2] = direction.x;
		}

		modelMat[3].xyz = vec3(PosRot.xyz - cameraPos.xyz); 
		//modelMat[3].xyz = //vec3(PosRot.x - cameraPos.x, 0.0, 0.0);
		
	}
	else
	{
		modelMat = model;
	}

	//vs_out.tangent = vec3(0);
	//vs_out.bitangent = vec3(0);
	vs_out.TexCoords = aTexCoord;
	vs_out.Normal = normalize(mat3(transpose(inverse(modelMat))) * aNormal);
	vs_out.FragPos = vec3(modelMat * vec4(aPos, 1.0));
	vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
	//vs_out.tangent = normalize(mat3(transpose(inverse(modelMat))) * aTangent);
	//vs_out.tangent = vec3(1.0);
	//vs_out.bitangent = normalize(mat3(transpose(inverse(modelMat))) * aBitangent); //is this bullshit necessary? isn't transform -> normalize enough

	vs_out.tangent = normalize(mat3(modelMat) * aTangent);
	vs_out.bitangent = normalize(mat3(modelMat) * aBitangent); //is this bullshit necessary? isn't transform -> normalize enough
	//vs_out.decalSpace = decal_Mat * vec4(vs_out.FragPos, 1.0);
	gl_Position = projection * view * modelMat * vec4(aPos, 1.0);
}