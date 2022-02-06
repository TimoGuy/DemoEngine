// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_BlurX.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D textureMap;

const vec2 gaussFilter[11] = vec2[](
	vec2(-5.0,  3.0/133.0),
	vec2(-4.0,  6.0/133.0),
	vec2(-3.0, 10.0/133.0),
	vec2(-2.0, 15.0/133.0),
	vec2(-1.0, 20.0/133.0),
	vec2( 0.0, 25.0/133.0),
	vec2( 1.0, 20.0/133.0),
	vec2( 2.0, 15.0/133.0),
	vec2( 3.0, 10.0/133.0),
	vec2( 4.0,  6.0/133.0),
	vec2( 5.0,  3.0/133.0)
);

void main()
{
	vec3 color = vec3(0.0);

	float scale = 1.0 / textureSize(textureMap, 0).x;

	for (int i = 0; i < 11; i++)
	{
		vec2 coord = vec2(texCoord.x + gaussFilter[i].x * scale, texCoord.y);
		color += textureLod(textureMap, coord, 0).rgb * gaussFilter[i].y;
	}

	fragColor = vec4(color, 1.0);
}
