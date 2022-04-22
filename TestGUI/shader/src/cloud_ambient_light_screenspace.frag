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


vec3 getRayDirection()
{
	float x = texCoord.x * 2.0 - 1.0;
	float y = (1.0 - texCoord.y) * 2.0 - 1.0;
	vec4 position_s = vec4(x, y, 1.0, 1.0);
	vec4 position_v = invCameraProjectionView * position_s;
	return normalize(position_v.xyz / position_v.w);
}


void main()
{
	// Construct normal from depth buffer!
	float depth = texture(cloudEffectDepthBuffer, texCoord).r;
	if (depth < 0.01)
	{
		fragmentColor = vec4(0.0);
		return;
	}

	//vec3 P = reconstructPosition(depth);
	//vec3 N = normalize(cross(dFdx(P), dFdy(P)));

	vec3 N = getRayDirection();

	vec3 irradiance = mix(texture(irradianceMap, sunSpinAmount * N).rgb, texture(irradianceMap2, sunSpinAmount * N).rgb, mapInterpolationAmt);

	fragmentColor = vec4(irradiance, 1.0);
}
