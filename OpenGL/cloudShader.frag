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

float linearDepth(float depthSample);
float minimum_example(vec3 high, vec3 low, vec3 origin, inout vec3 r_far_dir);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir);
float calculate_lighting(vec3 loc, vec3 light_dir, float dist, int steps);
float densityFunction(vec3 loc);
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
	vec3 sunDir = normalize(vec3(1, 1, 0.5)); // placeholder, will be replaced by uniforms later
	sunDir = normalize(u_sunDir);
	vec3 sunDiffuse = u_sunDiff * 1;
	vec3 sunAmbient = u_sunAmbi * 4;


	ray lray;
	lray.origin = cameraPos;

	float pixelDepth = linearDepth(texture(redraw_texture1_d, TexCoord).r);
	float focal_length = (screensize.y / 2) / tan(radians(45 / 2)); //focal length relative to resolution, assumes  fixed fov, needs to be a uniform later.
	float inv_focal_len = 1 / focal_length;

	pixel_pos = mat3(inverse(viewMat)) * vec3((gl_FragCoord.x - (screensize.x / 2 -0.5)) * inv_focal_len, (gl_FragCoord.y - (screensize.y / 2 - 0.5)) * inv_focal_len, -1) * pixelDepth;
	// why the heck does this work, why do I need negative focal length?

	lray.direction =  normalize(pixel_pos);


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
	

	vec3 cloudColor = vec3(0.9999, 0.9999, 0.9999);
	cloudColor = vec3(1.0);

	float densityCounter = 0.0;


	vec3 resultColor = vec3(0);
	float unscattered_left = 1.0; //1.1 for a little trick later on //NEEDS FIXING, BRIGHTNESS NOT ADJUSTED

	int fine_grade_divisor = 1;
	int accuracy = 1000;
	int light_accuracy = 10;
	float max_distance = 50000;
	float step_size = max_distance / accuracy;
	

	bool raymarch = true;

	float semi_random_offset = random(gl_FragCoord.xy);

	float total_distance = step_size + semi_random_offset * step_size;

	bool rough = true;

	resultColor;
	float light_incoming;

	while(raymarch)
	{
		total_distance += step_size; 
		if(pixelDepth < total_distance)
		{
			total_distance -= step_size;
			step_size = pixelDepth - total_distance;
			total_distance = pixelDepth;
			raymarch = false;
		}


		vec3 position = lray.origin + lray.direction * total_distance;

		if( (lray.direction.y >= 0 && position.y > top_height) || (lray.direction.y <= 0 && position.y < bot_height) ) // obvious optimization is obvious
			break;

		float local_density = densityFunction(position);




		if(local_density == -2)
		{
			total_distance += step_size * (fine_grade_divisor - 1);
		}
		if(local_density > 0)
		{
			
			light_incoming = calculate_lighting(position, sunDir, 500, 8)  * phaseFunction(acos(dot(sunDir, lray.direction)), 0.15 * pow(unscattered_left, 10));


			resultColor += (vec3(light_incoming * sunDiffuse) + sunAmbient * 2) * unscattered_left * local_density * step_size * 0.052; //make first sample distance-independent?
			unscattered_left *=   pow(1 - local_density, step_size / 10);
		}






		if(unscattered_left <= 0.1/1.1)
		{
			raymarch = false;
		}
		if(max_distance < total_distance)
			raymarch = false;
	}
	unscattered_left = unscattered_left * 1.1 - 0.1;
	total_blockage = unscattered_left;

	FragColor.rgb = resultColor;
	//FragColor.a = pow(0, 1);
	FragColor.a = 1 - total_blockage;


}

float calculate_lighting(vec3 loc, vec3 light_dir, float dist, int steps)
{
	float product = 1.0; //how much will be left
	for(int i = 1; i <= steps; i++)
	{
		float density = clamp(densityFunction(loc + i * (dist / steps) * u_sunDir), 0, 1);
		product *= pow(1 - density, 1);
	}
	return product;
}

float densityFunction(vec3 loc) //don't forget this shit can return negative numbers
{
		//return pow(clamp(length(loc) - 10, 0.05, 1.0), weightFactor);
	float lacunarity = 2;
	float persistance = 0.55;
	float worleylacunarity = 2.0;
	float worleypersistance = 0.5;
	float result;

	 vec3 noiseLoc = loc * 0.002;


	 if(loc.x > 38400 || loc.x < -38400 || loc.z > 38400 || loc.z < -38400)
		return 0.0;

	if( loc.y < bot_height || loc.y > top_height)
		return -2;

	float altitude_gradient = 1.0 - clamp(loc.y - bot_height, 0, bot_transition_range) / bot_transition_range + clamp(loc.y - top_height + top_transition_range, 0, top_transition_range) / top_transition_range;
	altitude_gradient = pow(altitude_gradient, 2);


	if(altitude_gradient >= 1.0)
		return -2.0;
	//return pow(clamp(1 - altitude_gradient, 0.2, 1.0), weightFactor);

	float combinedRoughNoise = texture(perlinWorleyTexture, noiseLoc / 32 ).r;

	float preliminary_density = clamp( (combinedRoughNoise + (u_cloudCover - 0.5) * 2) / (1 + (u_cloudCover - 0.5) * 2), 0.0, 1.0);

	if(preliminary_density <= 0.0)
	{
		return -2.0;
	}

	vec3 sample_loc = noiseLoc * 0.4;

	float invWorleyNoise = (texture(detailWorleyTexture, sample_loc).r - 1);
	float invWorleyNoise1 = (texture(detailWorleyTexture, sample_loc).g - 1);
	float invWorleyNoise2 = (texture(detailWorleyTexture, sample_loc).b - 1); // is slower than using 0.4
	float invWorleyNoise3 = (texture(detailWorleyTexture, noiseLoc).b - 1);
	bool testbool2 = true;

	float extra_detail = float(testbool2) * 0.1 * (0.6 * invWorleyNoise + 0.5 * invWorleyNoise1 +  0.9 * invWorleyNoise2 + 0.4 * invWorleyNoise3);

	result = clamp(pushOut(preliminary_density, 1)  + extra_detail  - altitude_gradient, 0.0, 0.9);
	result = pushOut(result, 1);
	return pow(result, 0.5); //change this value? we will see.
}

float phaseFunction(float angle, float scatterFactor)
{
	float result = 1.0;
	//return result;
	result = (1 - pow(scatterFactor, 2)) / (4* PI * pow(1 + pow(scatterFactor, 2) - 2 * scatterFactor * cos(angle), 1.5));
	//result += ( (3.f / 16.f) * PI ) * ( 1 + pow(cos(angle), 2));
	return result * 12.8; //very much non pbr, trying to get it close to 1;
}

float phaseFunction2(float angle, float scatterFactor) //Doesn't look well atm. might look well with HDR and such, but right now? NOPE
{
	float result = 1.0;
	//return result;
	result = 3 * scatterFactor * pow(cos(angle), 2); // two lobes
	result += 1; //for circular shape
	result += scatterFactor * angle * 1.02 / acos(-1); //generates a pointy spike
	result /= 1 + 1.97531 * scatterFactor;	//approx. normalization of volume and hence brightness. (this feels really stupid, I hate this, don't make me do this)

	return result;
}

float pushOut(float x, float strength)
{
	return 0.5 + pow(abs(x - 0.5) * 2, 1 / strength) * 0.5 * sign(x - 0.5); 
}

float linearDepth(float depthSample)
{
    depthSample = 2.0 * depthSample - 1.0;
    float zLinear = 2.0 * zNear * zFar / (zFar + zNear - depthSample * (zFar - zNear));
    return zLinear;
}
vec4 skyColor(vec3 viewDir)
{
    vec4 Color = vec4(0.73 - 0.2 * pow(max(viewDir.y, 0), 0.5), 0.9 - 0.1 * pow(max(viewDir.y, 0), 0.5), 0.7 + 0.22 * pow(max(viewDir.y, 0), 0.5), 1.0);
    vec4 sunLight = vec4(u_sunDiff, 1.0) * pow(1 / (acos(dot(viewDir, normalize(u_sunDir))) * 40), 3);
    return Color + sunLight;
}