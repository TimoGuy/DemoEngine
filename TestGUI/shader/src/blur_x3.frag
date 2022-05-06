// https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook/blob/master/data/shaders/chapter08/GL02_BlurX.frag
#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D textureMap;

//const vec2 gaussFilter[5] = vec2[](
//	vec2(-2.0,  7.0/107.0),
//	vec2(-1.0, 26.0/107.0),
//	vec2( 0.0, 41.0/107.0),
//	vec2( 1.0, 26.0/107.0),
//	vec2( 2.0,  7.0/107.0)
//);

//const vec2 gaussFilter[5] = vec2[](
//	vec2(-2.0, 1.0/5.0),
//	vec2(-1.0, 1.0/5.0),
//	vec2( 0.0, 1.0/5.0),
//	vec2( 1.0, 1.0/5.0),
//	vec2( 2.0, 1.0/5.0)
//);

//const vec2 gaussFilter[7] = vec2[](
//	vec2(-3.0,   2.0/401.0),
//	vec2(-2.0,  22.0/401.0),
//	vec2(-1.0,  97.0/401.0),
//	vec2( 0.0, 159.0/401.0),
//	vec2( 1.0,  97.0/401.0),
//	vec2( 2.0,  22.0/401.0),
//	vec2( 3.0,   2.0/401.0)
//);

const vec2 gaussFilter[9] = vec2[](		// https://www.desmos.com/calculator/35drzjks21
	vec2(-4.0, 0.01100),
	vec2(-3.0, 0.04318),
	vec2(-2.0, 0.11464),
	vec2(-1.0, 0.20598),
	vec2( 0.0, 0.25040),
	vec2( 1.0, 0.20598),
	vec2( 2.0, 0.11464),
	vec2( 3.0, 0.04318),
	vec2( 4.0, 0.01100)
);

void main()
{
	vec4 color = vec4(0.0);

	float scale = 1.0 / textureSize(textureMap, 0).x;

	for (int i = 0; i < 9; i++)
	{
		vec2 coord = vec2(texCoord.x + gaussFilter[i].x * scale, texCoord.y);
		color += textureLod(textureMap, coord, 0).rgba * gaussFilter[i].y;
	}

	fragColor = color;
}
