#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D cloudEffectDepthBuffer;

const vec3 sampleFilter[9] = vec3[](
	vec3(-1.0, -1.0, 0.0625),
	vec3( 0.0, -1.0, 0.125),
	vec3( 1.0, -1.0, 0.0625),
	vec3(-1.0,  0.0, 0.125),
	vec3( 0.0,  0.0, 0.25),
	vec3( 1.0,  0.0, 0.125),
	vec3(-1.0,  1.0, 0.0625),
	vec3( 0.0,  1.0, 0.125),
	vec3( 1.0,  1.0, 0.0625)
);

void main()
{
	vec4 color = vec4(0.0);

	float scale = 1.0 / textureSize(cloudEffectDepthBuffer, 0).x;

	float minDepth = 100.0;
	float maxDepth = -100.0;
	float avgDepth = 0.0;

	for (int i = 0; i < 9; i++)
	{
		vec2 coord = vec2(texCoord.x + sampleFilter[i].x * scale, texCoord.y + sampleFilter[i].y * scale);
		const float sampledDepth = textureLod(cloudEffectDepthBuffer, coord, 0).r;
		minDepth = min(sampledDepth, minDepth);
		maxDepth = max(sampledDepth, maxDepth);
		avgDepth += sampledDepth * sampleFilter[i].z;
	}

	fragmentColor = vec4(vec3(minDepth), 1.0);		// Idk what's up, but this seems to be a-okay??? Like, I'm wanting to do minDepth for now, but there could be a more complex algorithm for this for sure, so we'll see for now eh.
}
