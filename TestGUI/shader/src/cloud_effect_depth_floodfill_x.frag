#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D cloudEffectDepthBuffer;

const vec2 sampleFilter[5] = vec2[](
	vec2(-2.0,  7.0/107.0),
	vec2(-1.0, 26.0/107.0),
	vec2( 0.0, 41.0/107.0),
	vec2( 1.0, 26.0/107.0),
	vec2( 2.0,  7.0/107.0)
);

void main()
{
	vec4 color = vec4(0.0);

	float scale = 1.0 / textureSize(cloudEffectDepthBuffer, 0).x;

	float minDepth = 100.0;
	float maxDepth = -100.0;
	float avgDepth = 0.0;

	for (int i = 0; i < 5; i++)		// @NOTE: I spent over an hour trying to figure out why the shader program wasn't compiling. It was written i<9 when sampleFilter.size was 5. Apparently there are compile time checks like this, but I wish they'd let you see the error you were producing. Upon linking, it'd just crash, and wouldn't even let me look at the error! Thanks Nvidia.  -Timo
	{
		vec2 coord = vec2(texCoord.x + sampleFilter[i].x * scale, texCoord.y);
		const float sampledDepth = textureLod(cloudEffectDepthBuffer, coord, 0).r;
		minDepth = min(sampledDepth, minDepth);
		maxDepth = max(sampledDepth, maxDepth);
		avgDepth += sampledDepth * sampleFilter[i].y;
	}

	fragmentColor = vec4(vec3(minDepth), 1.0);		// Idk what's up, but this seems to be a-okay??? Like, I'm wanting to do minDepth for now, but there could be a more complex algorithm for this for sure, so we'll see for now eh.
}

//
// @NOTE: So here are my thoughts about doing a depth buffer flood-fill.
//		It seems like the change from one cloud to another may have some artifacts where the depth values don't line up, but the edges where it's 
//		the boundary between depth=1 (raymarch failed) and a hit on a cloud will not have these artifacts bc we'll have the alpha channel of the 
//		color texture of the cloud to be able to tone back the "fade out" of the cloud. It seems like between-cloud flood fills should still be 
//		done, however, bc it helps with removing depth noise in the face of the cloud. So, it's really similar to a "blur", except it gets the 
//		minimum depth. We'll see how it goes, I guess. ALSO, note that the gaussian blur kernel size is identical to the floodfill kernel size. This
//		is to achieve a 1 to 1 consistent size and not have any missing detail. Hopefully with the same size kernel it'd alleviate the inter-cloud 
//		blending issues that will likely occur. I bet it will!  -Timo
//
