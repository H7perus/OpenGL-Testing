#version 430 core
#define PI acos(-1)


out vec4 FragColor;

in  vec3 customColor;
in vec2 TexCoord;

uniform vec2 screensize;
uniform sampler2D redraw_texture1;
uniform sampler2D redraw_texture2;

void main()
{
	
//	int directions = 16;
//	float size = 5;
//	int Quality = 6;
//
//	vec2 Radius = size / screensize;
//	float total_added = 0;
//	vec4 blurredColor = texture(redraw_texture2, TexCoord); 
//	for(float d = 0; d < 2 * PI; d += 2 * PI / directions)
//	{
//		for(float q = 1.0 / Quality; q < Quality; q += 1.0/Quality)
//		{
//			vec4 local_color = texture(redraw_texture2, TexCoord + vec2(sin(d), cos(d)) * q * Radius) / q;
//			blurredColor.rgba += local_color.rgba * local_color.a;
//			total_added += 1/q;
//			
//		}
//	}
//	
//	blurredColor.rgba /= total_added * 2;
//
//	
//	
	//FragColor.rgb = texture(redraw_texture1, TexCoord).rgb * (1 - blurredColor.a) +  blurredColor.rgb * blurredColor.a;
	FragColor.rgb = texture(redraw_texture1, TexCoord).rgb * (1 - texture(redraw_texture2, TexCoord).a) +  texture(redraw_texture2, TexCoord).rgb * (texture(redraw_texture2, TexCoord).a);

	//FragColor.rgb = pow(FragColor.rgb, vec3(1.0 / 2.2));
	FragColor = pow(FragColor, vec4(1 / 2.2));
	//FragColor = texture(redraw_texture2, TexCoord);
	//FragColor = vec4(TexCoord, 1.0, 1.0);
}