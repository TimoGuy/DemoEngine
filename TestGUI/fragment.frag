#version 430

out vec4 fragmentColor;

in vec2 texCoord;
in vec4 fragPositionLightSpace;

uniform sampler2D tex0;
uniform sampler2D shadowMap;


float shadowCalculation()
{
	vec3 projectionCoords = fragPositionLightSpace.xyz / fragPositionLightSpace.w;		// NOTE: this line is absolutely meaningless in an ortho projection (like directional light), bc W is 1.0, so omit this when able
	projectionCoords = projectionCoords * 0.5 + 0.5;		// Make into NDC coordinates for sampling the depth tex

	float closestDepth = texture(shadowMap, projectionCoords.xy).r;
	float currentDepth = projectionCoords.z;
	float shadow = currentDepth - 0.005 > closestDepth ? 1.0 : 0.0;

	return shadow;
}


void main()
{
	//fragmentColor = vec4(texCoord, 0.0f, 1.0f);
	fragmentColor = texture(tex0, texCoord);
	//fragmentColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);


	//
	// Calculate Shadow
	//
	float shadow = shadowCalculation();
	if (shadow > 0.5) fragmentColor = vec4(1, 0, 0, 1);			// DEBUG
}