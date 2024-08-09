#version 460 core
#define PI acos(-1)


out vec4 FragColor;

in  vec3 customColor;
in vec2 TexCoord;
uniform sampler2D redraw_texture1;
uniform sampler2D redraw_texture1_d;

uniform sampler2D HDRI;

//uniform sampler2D redraw_texture2;
uniform sampler2D shadowMap;
uniform sampler3D noiseTexture;
uniform sampler3D worleyNoiseTexture;
uniform sampler3D perlinWorleyTexture;
uniform sampler3D detailWorleyTexture; //3 channels, different frequencies

uniform mat4 holeMat;
uniform vec3 cameraPos;

uniform vec3 u_sunDir;
uniform vec3 u_sunDiff;
uniform vec3 u_sunAmbi;

uniform mat4 lightSpaceMat;


uniform vec2 screensize;
uniform float fov;
uniform mat3 rotMat;
uniform mat4 viewMat;
uniform vec3 offset;
uniform int iFrame;

uniform bool test_bool;



uniform float u_cloudDensityMultiplier;
uniform float u_lightAbsorptionMultiplier;
uniform float u_lightScatterMultiplier;
uniform float u_cloudCover;
uniform float u_scatterFactor;

uniform float animation_state;
struct ray{
	vec3 origin;
	vec3 direction;
};

float ray2AABBIntersect(vec3 high, vec3 low, inout ray lray, inout vec3 r_near, inout vec3 r_far_dir);
float linearDepth(float depthSample);
float snoise4D(vec4 v);
float cnoise(vec3 P);
float minimum_example(vec3 high, vec3 low, vec3 origin, inout vec3 r_far_dir);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir);
float densityFunction(vec3 loc, float weightFactor);
float phaseFunction(float angle, float scatterFactor);
float pushOut(float x, float strength);
float testfunc();
vec4 skyColor(vec3 viewDir);

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0; }

float mod289(float x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0; }

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+1.0)*x);
}

float permute(float x) {
     return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}
//vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}

vec3 fade(vec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}
float taylorInvSqrt(float r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}
vec4 grad4(float j, vec4 ip)
{
  const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
  vec4 p,s;

  p.xyz = floor( fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;
  p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
  s = vec4(lessThan(p, vec4(0.0)));
  p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www;

  return p;
}

float zNear = 0.1;
float zFar = 50000;

vec3 pixel_pos;


float top_height = 2000;
float top_transition_range = 900;
float bot_height = 900;
float bot_transition_range = 200;
float atmos_const = 0.99995;

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

void main()
{

	float atmospheric_weight = 1.0;

	vec3 box_high = vec3(20.0, 4.0, 20);
	vec3 box_low = vec3(-20, -4.0, -20);


	//vec3 sunDir = normalize(vec3(1, 1, 0.5));
	vec3 sunDir = normalize(vec3(1, 1, 0.5));
	//vec3 sunDir = normalize(vec3(1, 0.1, 0.5));



	sunDir = normalize(u_sunDir);

	vec3 sunDiffuse = u_sunDiff * 1.5;
	vec3 sunAmbient = u_sunAmbi * 1.5;

	ray lray;
	lray.origin = cameraPos;

	float focal_length = (screensize.y / 2) / tan(radians(45 / 2));

	pixel_pos = rotMat * vec3(-(gl_FragCoord.x - (screensize.x / 2 -0.5)), (gl_FragCoord.y - (screensize.y / 2 - 0.5)), focal_length) / focal_length * linearDepth(texture(redraw_texture1_d, TexCoord).r);
	float pixelDepth = linearDepth(texture(redraw_texture1_d, TexCoord).r);
	pixelDepth = length(pixel_pos);

	lray.direction =  normalize(rotMat * normalize(vec3(-(gl_FragCoord.x - (screensize.x / 2 -0.5)), (gl_FragCoord.y - (screensize.y / 2 - 0.5)), focal_length)));


	FragColor =	texture(redraw_texture1, TexCoord);
	
	
	vec3 near = vec3(0);
	vec3 far_dir = vec3(0);
	if(length(pixel_pos) < length(near - lray.origin))
	{
		near = vec3(0.0);
		far_dir = vec3(0.0);
	}
	if(length(pixel_pos) < length(near - lray.origin + far_dir) && far_dir != vec3(0))
	{
		far_dir = pixel_pos + lray.origin - near;
	}

	float total_blockage = 1.0;
	float final_alpha = 1.0;
	

	vec3 smokeColor = vec3(0.999, 0.999, 0.999);
	smokeColor = vec3(1.0);

	float densityCounter = 0.0;


	vec3 resultColor = vec3(0);
	float unscattered_left = 1.1; //1.1 for a little trick later on //NEEDS FIXING, BRIGHTNESS NOT ADJUSTED
	int accuracy = 5000;
	float step_size = 1.f / accuracy;

	int light_accuracy = 10;
	float max_distance = 50000;
	int inaccurate_ratio = 10;
	float base_sample_length = max_distance / accuracy;
	float sample_length = base_sample_length;
	int fine_sample_count = 0;

	bool raymarch = true;

	float semi_random_offset = random(gl_FragCoord.xy);

	float total_distance = sample_length + semi_random_offset * sample_length;


	float previous_dist = 0;
	bool rough = true;
	while(raymarch)
	{
		if(total_distance > pixelDepth)
		{
			total_distance -= sample_length;
			sample_length -= total_distance - pixelDepth;
			total_distance += sample_length;
			raymarch = false;
		}

		vec3 loc = lray.origin + lray.direction * total_distance;
		//total_distance += sample_length;
		float local_density = 1 - densityFunction(loc, sample_length * u_lightScatterMultiplier * u_cloudDensityMultiplier);
		vec3 lightlevel = sunAmbient;
		float total_brightness = 1.0;

		if(local_density > 0)
		{
			float light_sample_length = 500.f / light_accuracy;

			for(int j = 1; j <= light_accuracy; j++)
			{
				float light_transmittance;
				light_transmittance = densityFunction(loc + sunDir * light_sample_length * j,  light_sample_length * u_lightAbsorptionMultiplier * u_cloudDensityMultiplier);

				if(light_transmittance < 1)
					total_brightness *= (light_transmittance);
			}

			//lightlevel += sunDiffuse * total_brightness * vec3(phaseFunction(acos(dot(lray.direction, sunDir)), u_scatterFactor));
			
			lightlevel += sunDiffuse * total_brightness * vec3(phaseFunction(acos(dot(lray.direction, sunDir)), pow(u_scatterFactor, 1 / total_brightness)));  

			unscattered_left *=  (1 - local_density);
			vec3 prelimColor = smokeColor * lightlevel * unscattered_left * local_density * 0.5; //  * (1 - pow(unscattered_left, 2));
			resultColor += prelimColor; //mix(prelimColor, vec3(skyColor(lray.direction)) * unscattered_left, pow(0.99998, total_distance));
			

			final_alpha *= ( 1 - pow(1 - unscattered_left, 2));


			atmospheric_weight *= 1 - (1 - pow(atmos_const, total_distance - previous_dist)) * unscattered_left;
			previous_dist = total_distance;
		}
//		if(local_density  == -1)
//		{
//			//rough = true;
//			//total_distance += sample_length * 9;
//		}
		
		//sample_length = base_sample_length / unscattered_left; //this slows down the code by 20% despite dynamically changing the sample depth to get bigger once total transmission goes down :upside_down:
		total_distance += sample_length;
		if(unscattered_left <= 0.1)
		{
			raymarch = false;
			unscattered_left = 0.0;
		}
		//if(total_distance > max_distance)
			//raymarch = false;

		if(loc.y > top_height && lray.direction.y > 0 || loc.y < bot_height && lray.direction.y < 0 || total_distance > max_distance)
			raymarch = false;
		
	}
	atmospheric_weight *= 1 - (1 - pow(atmos_const, pixelDepth - previous_dist)) * unscattered_left;
	if(unscattered_left >= 0.1)
	{
		unscattered_left -= 0.1;
	}
	total_blockage = unscattered_left;

	resultColor *= 1;
	resultColor = clamp(resultColor, vec3(0.0), vec3(1.0));

	
	//total_blockage = 1 - total_blockage;

	vec2 TexCoordinates = vec2((atan(lray.direction.z, lray.direction.x) + PI) / (2 * PI), (-sin(lray.direction.y) + PI / 2) / PI);

	//FragColor = texture(HDRI, TexCoordinates) * 0.8;
	//FragColor.rgb = resultColor / (1 - pow(unscattered_left, 2));


	vec4 fog_rgba = vec4(vec3(skyColor(lray.direction)), 1 - pow(0.99998, pixelDepth));

	//total_blockage = (pow(total_blockage, 2));





	FragColor.rgb = clamp(resultColor / (1 - pow(unscattered_left, 2)), vec3(0.0), vec3(1.0));
	FragColor.a = (1 - total_blockage);
	//FragColor.a = 0;
	
	//FragColor.rgb = mix(vec3(skyColor(lray.direction)), FragColor.rgb, atmospheric_weight);

	//FragColor.a = 1 - (atmospheric_weight * total_blockage);

	//FragColor.rgb = skyColor(lray.direction)).rgb;

	//FragColor.rgb = resultColor;
	//FragColor.a = 1 - total_blockage;



}

float densityFunction(vec3 loc, float weightFactor)
{
		//return pow(clamp(length(loc) - 10, 0.05, 1.0), weightFactor);
	float lacunarity = 2;
	float persistance = 0.55;
	float worleylacunarity = 2.0;
	float worleypersistance = 0.5;
	float result;

	 vec3 noiseLoc = loc * 0.002;


	 if(loc.x > 38400 || loc.x < -38400 || loc.z > 38400 || loc.z < -38400)
		return 1.0;

	float altitude_gradient = 1.0 - clamp(loc.y - bot_height, 0, bot_transition_range) / bot_transition_range + clamp(loc.y - top_height + top_transition_range, 0, top_transition_range) / top_transition_range;
	altitude_gradient = pow(altitude_gradient, 2);


	if(altitude_gradient >= 1.0)
		return 2.0;
	//return pow(clamp(1 - altitude_gradient, 0.2, 1.0), weightFactor);

	float Noise0 = texture(noiseTexture, noiseLoc / 32).r;
	float Noise1 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.7)) * pow(lacunarity, 1)).r * pow(persistance, 1);
	float Noise2 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.9)) * pow(lacunarity, 2)).r * pow(persistance, 2);
	float Noise3 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 3)).r * pow(persistance, 3);
	float Noise4 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 4)).r * pow(persistance, 4);
	float Noise5 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 5)).r * pow(persistance, 5);
	float Noise6 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 6)).r * pow(persistance, 6);
	
	float combinedRoughNoise = texture(perlinWorleyTexture, noiseLoc / 32 ).r;

	float preliminary_density = clamp( (combinedRoughNoise + (u_cloudCover - 0.5) * 2) / (1 + (u_cloudCover - 0.5) * 2), 0.0, 1.0);

	if(preliminary_density <= 0.0)
	{
		return 2.0;
	}

	float invWorleyNoise = (texture(detailWorleyTexture, noiseLoc / 2.5).r - 1);
	float invWorleyNoise1 = (texture(detailWorleyTexture, noiseLoc / 2.5).g - 1);
	float invWorleyNoise2 = (texture(detailWorleyTexture, noiseLoc / 2.5).b - 1);
	bool testbool2 = true;

	result = clamp(pushOut(preliminary_density, 1.0)  + float(testbool2) * 0.1 * (0.6 * invWorleyNoise + 0.3 * invWorleyNoise1 +  0.9 * invWorleyNoise2 /*+ (Noise3 - 0.0625 * 2) * 4*/ ) - altitude_gradient, 0.0, 0.8);
	return pow(1 - result, weightFactor);
}
float phaseFunction(float angle, float scatterFactor)
{
	float result = 1.0;
	return result;
	result = (1 - pow(scatterFactor, 2)) / (4* PI * pow(1 + pow(scatterFactor, 2) - 2 * scatterFactor * cos(angle), 1.5));
	result += 0.8;
	//result += ( (3.f / 16.f) * PI ) * ( 1 + pow(cos(angle), 2));
	return result;
}
float densityFunctionDebug(vec3 loc, float weightFactor)
{
	//return pow(clamp(length(loc) - 10, 0.05, 1.0), weightFactor);
	float lacunarity = 2;
	float persistance = 0.55;
	float worleylacunarity = 2.0;
	float worleypersistance = 0.5;
	float result;

	

	 vec3 noiseLoc = loc * 0.002;

	float altitude_gradient = 1.0 - clamp(loc.y - bot_height, 0, bot_transition_range) / bot_transition_range + clamp(loc.y - top_height + top_transition_range, 0, top_transition_range) / top_transition_range;
	altitude_gradient = pow(altitude_gradient, 2);


	if(altitude_gradient >= 1.0)
		return 2.0;

//	float Noise0 = cnoise(noiseLoc.xyz * pow(lacunarity, 0)) * pow(persistance, 0);
//	//float Noise05 = cnoise(noiseLoc * pow(lacunarity, 0.5)) * pow(persistance, 0.5);
//	float Noise1 = cnoise(noiseLoc * pow(lacunarity, 1)) * pow(persistance, 1);
//	float Noise2 = cnoise(noiseLoc * pow(lacunarity, 2)) * pow(persistance, 2);
//	float Noise3 = cnoise(noiseLoc * pow(lacunarity, 3)) * pow(persistance, 3);
//	float Noise4 = cnoise(noiseLoc * pow(lacunarity, 4)) * pow(persistance, 4);
//	float Noise5 = cnoise(noiseLoc * pow(lacunarity, 5)) * pow(persistance, 5);
//	float Noise6 = cnoise(noiseLoc * pow(lacunarity, 6)) * pow(persistance, 6);
	//float Noise7 = cnoise(noiseLoc * pow(lacunarity, 7)) * pow(persistance, 7);
	//float Noise8 = cnoise(noiseLoc * pow(lacunarity, 8)) * pow(persistance, 8);

	float Noise0 = texture(noiseTexture, noiseLoc / 32).r;
	float Noise1 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.7)) * pow(lacunarity, 1)).r * pow(persistance, 1);
	float Noise2 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.9)) * pow(lacunarity, 2)).r * pow(persistance, 2);
	float Noise3 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 3)).r * pow(persistance, 3);
	float Noise4 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 4)).r * pow(persistance, 4);
	float Noise5 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 5)).r * pow(persistance, 5);
	float Noise6 = texture(noiseTexture, (noiseLoc / 32 + vec3(0.2)) * pow(lacunarity, 6)).r * pow(persistance, 6);
	

////	float worleyNoise = texture(worleyNoiseTexture, noiseLoc / 16).r;
////	float worleyNoise1 = texture(worleyNoiseTexture, noiseLoc / 16 * pow(worleylacunarity, 1) + vec3(0.1, 0.1, 0.1)).r * pow(worleypersistance, 1);
////	float worleyNoise2 = texture(worleyNoiseTexture, noiseLoc / 16 * pow(worleylacunarity, 2) + vec3(0.1, 0.1, 0.1)).r * pow(worleypersistance, 2);
//	float worleyNoise3 = texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 3) + vec3(0.1, 0.1, 0.1)).r * pow(worleypersistance, 3);
//	float worleyNoise4 = texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 4) + vec3(0.2, 0.2, 0.2)).r * pow(worleypersistance, 4);
//	float worleyNoise5 = texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 5) + vec3(0.2, 0.2, 0.2)).r * pow(worleypersistance, 5);
//	float worleyNoise6 = texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 6) + vec3(0.2, 0.2, 0.2)).r * pow(worleypersistance, 6);
//
//	float max_value_multiplier =  1 / (1 + 0.5 + 1.5);
//
//	float amplitude = 1 + 0.5 + (1 + 0.5) * 1.5;
//	float worleyAmplitude = 1 + 0.5 + 0.25 + 0.125 + 0.0625;
//	float worleyTotal = (worleyNoise + worleyNoise1 + worleyNoise2 + worleyNoise3 + worleyNoise4) / worleyAmplitude;
//
//	float perlinAmplitude = 1 + 0.5 + 0.25 + 0.125 + 0.0625;
//	float perlinTotal = (Noise0 + Noise1 + Noise2 + Noise3 + Noise4) / perlinAmplitude;
//
//	//float combinedRoughNoise = ( (Noise0 + Noise1) + ((worleyNoise * 2 - 1) + (worleyNoise1 * 2 - 0.5))* 1.5) / amplitude; 	// * max_value_multiplier;
//	float combinedRoughNoise = (worleyTotal + perlinTotal) / 2; 	// * max_value_multiplier;

	float combinedRoughNoise = texture(perlinWorleyTexture, noiseLoc / 32 ).r;

	float preliminary_density = clamp( (combinedRoughNoise + (u_cloudCover - 0.5) * 2) / (1 + (u_cloudCover - 0.5) * 2), 0.0, 1.0);
	//float preliminary_density = clamp( combinedRoughNoise, 0.0, 1.0);

	if(preliminary_density <= 0.0)
	{
		return 2.0;
	}
	Noise4 = texture(noiseTexture, noiseLoc / 32 * pow(lacunarity, 4)).r * pow(persistance, 4);
	//Noise5 = texture(noiseTexture, noiseLoc / 32 * pow(lacunarity, 5)).r * pow(persistance, 5);
	Noise6 = texture(noiseTexture, noiseLoc / 32 * pow(lacunarity, 6)).r * pow(persistance, 6);
	//float invWorleyNoise = (texture(worleyNoiseTexture, noiseLoc / 5).r - 1);
	//float invWorleyNoise1 = (texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 1) + vec3(0.1, 0.1, 0.1)).r - 1) * pow(worleypersistance, 1) * 2;
	//float invWorleyNoise2 = (texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 2) + vec3(0.1, 0.1, 0.1)).r - 1) * pow(worleypersistance, 2) * 4;
	//float invWorleyNoise3 = (1 - texture(worleyNoiseTexture, noiseLoc / 16 * pow(worleylacunarity, 3) + vec3(0.1, 0.1, 0.1)).r) * pow(worleypersistance, 3);
	//float invWorleyNoise4 = (1 - texture(worleyNoiseTexture, noiseLoc / 16 * pow(worleylacunarity, 4) + vec3(0.2, 0.2, 0.2)).r) * pow(worleypersistance, 4);
	//float invWorleyNoise5 = (1 - texture(worleyNoiseTexture, noiseLoc / 5 * pow(worleylacunarity, 5) + vec3(0.2, 0.2, 0.2)).r) * pow(worleypersistance, 5);

	float invWorleyNoise = (texture(detailWorleyTexture, noiseLoc / 2.5).r - 1);
	float invWorleyNoise1 = (texture(detailWorleyTexture, noiseLoc / 2.5).g - 1);
	float invWorleyNoise2 = (texture(detailWorleyTexture, noiseLoc / 2.5).b - 1);

	//result = clamp(pushOut(preliminary_density, 1.0)  - 0.6 * int(test_bool)  * ((invWorleyNoise3 * 0.167 * 8) + invWorleyNoise4 * 0.133 * 16) - 0 * float(test_bool) * clamp((Noise4 + Noise6), 0.0, 1.0) - altitude_gradient, 0.0, 0.8);
	result = clamp(pushOut(preliminary_density, 1.0)  + float(test_bool) * 0.1 * (0.6 * invWorleyNoise + 0.3 * invWorleyNoise1 +  0.9 * invWorleyNoise2 /*+ (Noise3 - 0.0625 * 2) * 4*/ ) - altitude_gradient, 0.0, 0.8);
	//result = preliminary_density;
	return pow(1 - result, weightFactor);
}
float pushOut(float x, float strength)
{
	return 0.5 + pow(abs(x - 0.5) * 2, 1 / strength) * 0.5 * sign(x - 0.5); 
}

float ray2AABBIntersect(vec3 high, vec3 low, inout ray lray, inout vec3 r_near, inout vec3 r_far_dir)
{
	float multiplier;
	high -= lray.origin;
	low -= lray.origin;
	int count = 0;
	vec3 far = vec3(0);
	vec3 near = vec3(0);
	r_near = vec3(0);
	r_far_dir = vec3(0);
	
	for (int i = 0; i < 3; i++)
	{
		if (0 > high[i] && lray.direction[i] > 0)
		{
			r_near = vec3(0);
			r_far_dir = vec3(0);
			return 0.0;
		}
		if (0 < low[i] && lray.direction[i] < 0)
		{
			r_near = vec3(0);
			r_far_dir = vec3(0);
			return 0.0;
		}
		else
		{
			r_near = vec3(0);
			r_far_dir = vec3(0);
			vec3 local;
			multiplier = 1 / lray.direction[i];
			int i1 = (i + 1) % 3;
			int i2 = (i + 2) % 3;
			local[i] = high[i];
			local[i1] = high[i] * multiplier * lray.direction[i1];
			local[i2] = high[i] * multiplier * lray.direction[i2];

			if (local[i1] < high[i1]    &&  local[i2] < high[i2] &&  !(high[i] > 0  && lray.direction[i] < 0)     )
			{
				if (local[i1] > low[i1] && local[i2] > low[i2])
				{
					count++;
					//return 1.0;
					
					
					if(count == 2)
					{
						//return 1.0;
						if(length(far) > length(local))
						{
							near = local;
						}
						else
						{
							near = far;
							far = local;
						}
						float dist = length(near - far);

						r_near = near + lray.origin;
						r_far_dir = far - near;
						dist = length(far - near);

						return clamp(dist, 0.0, 1.0);
					}
					far = local;
				}
			}

			local[i] = low[i];
			local[i1] = low[i] * multiplier * lray.direction[i1];
			local[i2] = low[i] * multiplier * lray.direction[i2];

			if (local[i1] < high[i1] && local[i2] < high[i2] && !(0 > low[i] && lray.direction[i] > 0))
			{
				if (local[i1] > low[i1] && local[i2] > low[i2])
				{
					count++;
					
					if(count == 2)
					{
						//return 1.0;
						if(length(far) > length(local))
						{
							near = local;
						}
						else
						{
							near = far;
							far = local;
						}
						float dist = length(near - far);
						r_near = near + lray.origin;
						r_far_dir = far - near;
						dist = length(far - near);
						//return 1.0;
						return clamp(dist, 0.0, 1.0);
					}
					far = local;
				}
			}
		}
	}
	//return 0.0;
	if(count == 1)
	{
		//return 1.0;
		r_near = lray.origin;
		r_far_dir = far;
		return max(0, min(1.0, length(far)));
	}
	r_near = vec3(0);
	r_far_dir = vec3(0);
	//return  max(0, min(1.0, abs(length(far))));
	return 0.0;
}



float linearDepth(float depthSample)
{
    depthSample = 2.0 * depthSample - 1.0;
    float zLinear = 2.0 * zNear * zFar / (zFar + zNear - depthSample * (zFar - zNear));
    return zLinear;
}

float snoise4D(vec4 v)
{
// (math.sqrt(5) - 1)/4 = F4, used once below
const float F4 = 0.309016994374947451f;
vec4 C = vec4( 0.138196601125011f,  // (5 - math.sqrt(5))/20  G4
    0.276393202250021f,  // 2 * G4
    0.414589803375032f,  // 3 * G4
    -0.447213595499958f); // -1 + 4 * G4

// First corner
vec4 i  = floor(v + dot(v, vec4(F4)) );
vec4 x0 = v -   i + dot(i, C.xxxx);

// Other corners

// Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
vec4 i0 = vec4(0.0f);
vec3 isX = step( x0.yzw, x0.xxx );
vec3 isYZ = step( x0.zww, x0.yyz );
//  i0.x = math.dot( isX, vec3( 1.0 ) );
i0.x = isX.x + isX.y + isX.z;
i0.yzw = 1.0f - isX;
//  i0.y += math.dot( isYZ.xy, vec2( 1.0 ) );
i0.y += isYZ.x + isYZ.y;
i0.zw += 1.0f - isYZ.xy;
i0.z += isYZ.z;
i0.w += 1.0f - isYZ.z;

// i0 now contains the unique values 0,1,2,3 in each channel
vec4 i3 = clamp( i0, 0.0f, 1.0f );
vec4 i2 = clamp( i0-1.0f, 0.0f, 1.0f );
vec4 i1 = clamp( i0-2.0f, 0.0f, 1.0f );

//  x0 = x0 - 0.0 + 0.0 * C.xxxx
//  x1 = x0 - i1  + 1.0 * C.xxxx
//  x2 = x0 - i2  + 2.0 * C.xxxx
//  x3 = x0 - i3  + 3.0 * C.xxxx
//  x4 = x0 - 1.0 + 4.0 * C.xxxx
vec4 x1 = x0 - i1 + C.xxxx;
vec4 x2 = x0 - i2 + C.yyyy;
vec4 x3 = x0 - i3 + C.zzzz;
vec4 x4 = x0 + C.wwww;

// Permutations
i = mod289(i);
float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x);
vec4 j1 = permute( permute( permute( permute (
                                           i.w + vec4(i1.w, i2.w, i3.w, 1.0f ))
                                       + i.z + vec4(i1.z, i2.z, i3.z, 1.0f ))
                              + i.y + vec4(i1.y, i2.y, i3.y, 1.0f ))
                     + i.x + vec4(i1.x, i2.x, i3.x, 1.0f ));

// Gradients: 7x7x6 points over a cube, mapped onto a 4-cross polytope
// 7*7*6 = 294, which is close to the ring size 17*17 = 289.
vec4 ip = vec4(1.0f/294.0f, 1.0f/49.0f, 1.0f/7.0f, 0.0f) ;

vec4 p0 = grad4(j0,   ip);
vec4 p1 = grad4(j1.x, ip);
vec4 p2 = grad4(j1.y, ip);
vec4 p3 = grad4(j1.z, ip);
vec4 p4 = grad4(j1.w, ip);

// Normalise gradients
vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
p0 *= norm.x;
p1 *= norm.y;
p2 *= norm.z;
p3 *= norm.w;
p4 *= taylorInvSqrt(dot(p4,p4));

// Mix contributions from the five corners
vec3 m0 = max(0.6f - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0f);
vec2 m1 = max(0.6f - vec2(dot(x3,x3), dot(x4,x4)            ), 0.0f);
m0 = m0 * m0;
m1 = m1 * m1;
return 49.0f * ( dot(m0*m0, vec3( dot( p0, x0 ), dot( p1, x1 ), dot( p2, x2 )))
                + dot(m1*m1, vec2( dot( p3, x3 ), dot( p4, x4 ) ) ) ) ;
}
float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    //return closestDepth;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    //vec3 normal = normalize(Normal);
    float bias = 0.05;
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    //shadow = currentDepth > closestDepth  ? 1.0 : 0.0;
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}




float cnoise(vec3 P){
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 / 7.0;
  vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 / 7.0;
  vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

vec4 skyColor(vec3 viewDir)
{
    vec4 Color = vec4(0.73 - 0.2 * pow(max(viewDir.y, 0), 0.5), 0.9 - 0.1 * pow(max(viewDir.y, 0), 0.5), 0.7 + 0.22 * pow(max(viewDir.y, 0), 0.5), 1.0);
    vec4 sunLight = vec4(u_sunDiff, 1.0) * pow(1 / (acos(dot(viewDir, normalize(u_sunDir))) * 40), 3);
    return Color + sunLight;
}