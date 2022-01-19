// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_BlurY.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D ssaoTexture;

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

	float scale = 1.0 / textureSize(ssaoTexture, 0).y;

	for (int i = 0; i < 11; i++)
	{
		vec2 coord = vec2(texCoord.x, texCoord.y + gaussFilter[i].x * scale);
		color += textureLod(ssaoTexture, coord, 0).rgb * gaussFilter[i].y;
	}

	fragColor = vec4(color, 1.0);
}