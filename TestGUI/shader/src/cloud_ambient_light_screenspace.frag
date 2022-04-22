#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform float mainCameraZFar;
uniform sampler2D cloudEffectDepthBuffer;
uniform mat4 invCameraProjectionView;


// ext: PBR daynight cycle
uniform samplerCube irradianceMap;
uniform samplerCube irradianceMap2;
//uniform samplerCube prefilterMap;
//uniform samplerCube prefilterMap2;
//uniform sampler2D brdfLUT;
uniform float mapInterpolationAmt;

uniform mat3 sunSpinAmount;


//vec3 getRayDirection()
//{
//	float x = texCoord.x * 2.0 - 1.0;
//	float y = (1.0 - texCoord.y) * 2.0 - 1.0;
//	vec4 position_s = vec4(x, y, 1.0, 1.0);
//	vec4 position_v = invCameraProjectionView * position_s;
//	return normalize(position_v.xyz / position_v.w);
//}


const int NUM_POSITIONS_TO_SAMPLE = 168;
const vec2 positionsToSample[NUM_POSITIONS_TO_SAMPLE] = {

	vec2(-6, -6), vec2(-5, -6), vec2(-4, -6), vec2(-3, -6), vec2(-2, -6), vec2(-1, -6), vec2( 0, -6), vec2( 1, -6), vec2( 2, -6), vec2( 3, -6), vec2( 4, -6), vec2( 5, -6), vec2( 6, -6),
	vec2(-6, -5), vec2(-5, -5), vec2(-4, -5), vec2(-3, -5), vec2(-2, -5), vec2(-1, -5), vec2( 0, -5), vec2( 1, -5), vec2( 2, -5), vec2( 3, -5), vec2( 4, -5), vec2( 5, -5), vec2( 6, -5),
	vec2(-6, -4), vec2(-5, -4), vec2(-4, -4), vec2(-3, -4), vec2(-2, -4), vec2(-1, -4), vec2( 0, -4), vec2( 1, -4), vec2( 2, -4), vec2( 3, -4), vec2( 4, -4), vec2( 5, -4), vec2( 6, -4),
	vec2(-6, -3), vec2(-5, -3), vec2(-4, -3), vec2(-3, -3), vec2(-2, -3), vec2(-1, -3), vec2( 0, -3), vec2( 1, -3), vec2( 2, -3), vec2( 3, -3), vec2( 4, -3), vec2( 5, -3), vec2( 6, -3),
	vec2(-6, -2), vec2(-5, -2), vec2(-4, -2), vec2(-3, -2), vec2(-2, -2), vec2(-1, -2), vec2( 0, -2), vec2( 1, -2), vec2( 2, -2), vec2( 3, -2), vec2( 4, -2), vec2( 5, -2), vec2( 6, -2),
	vec2(-6, -1), vec2(-5, -1), vec2(-4, -1), vec2(-3, -1), vec2(-2, -1), vec2(-1, -1), vec2( 0, -1), vec2( 1, -1), vec2( 2, -1), vec2( 3, -1), vec2( 4, -1), vec2( 5, -1), vec2( 6, -1),
	vec2(-6,  0), vec2(-5,  0), vec2(-4,  0), vec2(-3,  0), vec2(-2,  0), vec2(-1,  0),/*vec2(0,0),*/ vec2( 1,  0), vec2( 2,  0), vec2( 3,  0), vec2( 4,  0), vec2( 5,  0), vec2( 6,  0),
	vec2(-6,  1), vec2(-5,  1), vec2(-4,  1), vec2(-3,  1), vec2(-2,  1), vec2(-1,  1), vec2( 0,  1), vec2( 1,  1), vec2( 2,  1), vec2( 3,  1), vec2( 4,  1), vec2( 5,  1), vec2( 6,  1),
	vec2(-6,  2), vec2(-5,  2), vec2(-4,  2), vec2(-3,  2), vec2(-2,  2), vec2(-1,  2), vec2( 0,  2), vec2( 1,  2), vec2( 2,  2), vec2( 3,  2), vec2( 4,  2), vec2( 5,  2), vec2( 6,  2),
	vec2(-6,  3), vec2(-5,  3), vec2(-4,  3), vec2(-3,  3), vec2(-2,  3), vec2(-1,  3), vec2( 0,  3), vec2( 1,  3), vec2( 2,  3), vec2( 3,  3), vec2( 4,  3), vec2( 5,  3), vec2( 6,  3),
	vec2(-6,  4), vec2(-5,  4), vec2(-4,  4), vec2(-3,  4), vec2(-2,  4), vec2(-1,  4), vec2( 0,  4), vec2( 1,  4), vec2( 2,  4), vec2( 3,  4), vec2( 4,  4), vec2( 5,  4), vec2( 6,  4),
	vec2(-6,  5), vec2(-5,  5), vec2(-4,  5), vec2(-3,  5), vec2(-2,  5), vec2(-1,  5), vec2( 0,  5), vec2( 1,  5), vec2( 2,  5), vec2( 3,  5), vec2( 4,  5), vec2( 5,  5), vec2( 6,  5),
	vec2(-6,  6), vec2(-5,  6), vec2(-4,  6), vec2(-3,  6), vec2(-2,  6), vec2(-1,  6), vec2( 0,  6), vec2( 1,  6), vec2( 2,  6), vec2( 3,  6), vec2( 4,  6), vec2( 5,  6), vec2( 6,  6)

};


vec3 reconstructPosition(float depth)
{
	float x = texCoord.x * 2.0 - 1.0;
	float y = (1.0 - texCoord.y) * 2.0 - 1.0;
	float z = (depth / mainCameraZFar) * 2.0 - 1.0;
	vec4 position_s = vec4(x, y, z, 1.0);
	vec4 position_v = invCameraProjectionView * position_s;
	return position_v.xyz / position_v.w;
}


void main()
{
	// Construct normal from depth buffer!
	float depth = texture(cloudEffectDepthBuffer, texCoord).r;
	if (depth < 0.01 || depth >= mainCameraZFar)
	{
		fragmentColor = vec4(0.0);
		return;
	}

	// Accumulate all depths in a box-blur (not gaussian) style.
	int totalSamples = 1;	// @NOTE: this includes the first depth sample above.
	const float scale = 1.0 / textureSize(cloudEffectDepthBuffer, 0).x;
	for (int i = 0; i < NUM_POSITIONS_TO_SAMPLE; i++)
	{
		vec2 coord = vec2(texCoord.x + positionsToSample[i].x * scale, texCoord.y + positionsToSample[i].y * scale);
		float sampledDepth = texture(cloudEffectDepthBuffer, coord).r;

		if (sampledDepth < 0.01 || sampledDepth >= mainCameraZFar)
			continue;	// Exit out of this pixel... don't include in the average.

		depth += sampledDepth;
		totalSamples++;
	}

	depth /= float(totalSamples);	// Avg.

	vec3 P = reconstructPosition(depth);
	vec3 N = normalize(cross(dFdx(P), dFdy(P)));
	fragmentColor = vec4(N, 1.0);

	//vec3 N = getRayDirection();

	//vec3 irradiance = mix(texture(irradianceMap, sunSpinAmount * N).rgb, texture(irradianceMap2, sunSpinAmount * N).rgb, mapInterpolationAmt);
	//
	//fragmentColor = vec4(irradiance, 1.0);
}
