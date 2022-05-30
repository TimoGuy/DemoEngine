#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D textureMap;

const vec2 sampleFilter[9] = vec2[](		// https://www.desmos.com/calculator/35drzjks21
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
    const vec3 luma = vec3(0.299, 0.587, 0.114);

	float scale = 1.0 / textureSize(textureMap, 0).x;

	float maxLuma = -1.0;

	for (int i = 0; i < 9; i++)
	{
		vec2 coord = vec2(texCoord.x + sampleFilter[i].x * scale, texCoord.y);
		vec4 colorSampled = textureLod(textureMap, coord, 0).rgba;

		float sampledLuma = dot(colorSampled.rgb * (1.0 - colorSampled.a), luma);  // * sampleFilter[i].y;
		if (maxLuma < sampledLuma)
		{
			color = colorSampled;
			maxLuma = max(maxLuma, sampledLuma);
		}
	}

	fragColor = color;
}
