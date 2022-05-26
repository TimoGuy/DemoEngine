#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D cloudEffectDepthBuffer;

const vec2 sampleFilter[5] = vec2[](
	vec2( 0.0, 41.0/107.0),		// @NOTE: the first one is the center pixel, and if it's the exit-early bit (0), then we'll only have sampled a single spot on the texture!
	vec2(-2.0,  7.0/107.0),
	vec2(-1.0, 26.0/107.0),
	vec2( 1.0, 26.0/107.0),
	vec2( 2.0,  7.0/107.0)
);

void main()
{
	vec4 color = vec4(0.0);

	float scale = 1.0 / textureSize(cloudEffectDepthBuffer, 0).x;

	bool first = true;
	float minDepth = 0.0;
	float maxDepth = 0.0;
	float avgDepth = 0.0;

	for (int i = 0; i < 5; i++)		// @NOTE: I spent over an hour trying to figure out why the shader program wasn't compiling. It was written i<9 when sampleFilter.size was 5. Apparently there are compile time checks like this, but I wish they'd let you see the error you were producing. Upon linking, it'd just crash, and wouldn't even let me look at the error! Thanks Nvidia.  -Timo
	{

		vec2 coord = vec2(texCoord.x, texCoord.y + sampleFilter[i].x * scale);
		const float sampledDepth = textureLod(cloudEffectDepthBuffer, coord, 0).r;
		if (sampledDepth < 0.01)
		{
			if (first)
			{
				fragmentColor = vec4(sampledDepth, 0, 0, 1);		// @NOTE: exit if first (aka, current texel is a bail texel (0.0 value)
				return;
			}

			// Or else, don't consider this texel in the min/max or avg calculations
			continue;
		}

		avgDepth += sampledDepth * sampleFilter[i].y;
		
		if (first)
		{
			minDepth = sampledDepth;
			maxDepth = sampledDepth;
			first = false;
			continue;
		}
		minDepth = min(sampledDepth, minDepth);
		maxDepth = max(sampledDepth, maxDepth);
	}

	fragmentColor = vec4(vec3(minDepth), 1.0);		// Idk what's up, but this seems to be a-okay??? Like, I'm wanting to do minDepth for now, but there could be a more complex algorithm for this for sure, so we'll see for now eh.
}
