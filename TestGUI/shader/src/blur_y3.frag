// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_BlurY.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D textureMap;

const vec2 gaussFilter[5] = vec2[](
	vec2(-2.0,  7.0/107.0),
	vec2(-1.0, 26.0/107.0),
	vec2( 0.0, 41.0/107.0),
	vec2( 1.0, 26.0/107.0),
	vec2( 2.0,  7.0/107.0)
);

//const vec2 gaussFilter[5] = vec2[](
//	vec2(-2.0, 1.0/5.0),
//	vec2(-1.0, 1.0/5.0),
//	vec2( 0.0, 1.0/5.0),
//	vec2( 1.0, 1.0/5.0),
//	vec2( 2.0, 1.0/5.0)
//);

void main()
{
	vec4 color = vec4(0.0);

	float scale = 1.0 / textureSize(textureMap, 0).y;

	for (int i = 0; i < 5; i++)
	{
		vec2 coord = vec2(texCoord.x, texCoord.y + gaussFilter[i].x * scale);
		color += textureLod(textureMap, coord, 0).rgba * gaussFilter[i].y;
	}

	fragColor = color;
}
