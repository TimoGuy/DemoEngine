#version 430

out vec4 fragmentColor;

in vec2 texCoord;
in vec3 fragPosition;
in vec4 fragPositionLightSpace;
in vec3 normalVector;

uniform sampler2D tex0;
uniform sampler2D shadowMap;

uniform vec3 lightPosition;
uniform vec3 viewPosition;
uniform vec3 lightColor;


float shadowCalculation(vec3 lightDir)
{
	vec3 projectionCoords = fragPositionLightSpace.xyz / fragPositionLightSpace.w;		// NOTE: this line is absolutely meaningless in an ortho projection (like directional light), bc W is 1.0, so omit this when able
	projectionCoords = projectionCoords * 0.5 + 0.5;		// Make into NDC coordinates for sampling the depth tex

	float closestDepth = texture(shadowMap, projectionCoords.xy).r;
	float currentDepth = projectionCoords.z;
	float bias = max(0.05 * (1.0 - dot(normalVector, lightDir)), 0.005);
	
	// PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projectionCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

	if(projectionCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}


void main()
{
	//vec4 objectColor = mix(texture(tex0, texCoord), vec4(0.3, 0.3, 0.3, 1.0), 0.8);
	vec4 objectColor = vec4(0.3, 0.3, 0.3, 1.0);

	//
	// Ambient light
	//
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	//
	// Diffuse Light
	//
	vec3 norm = normalize(normalVector);
	vec3 lightDir = normalize(lightPosition - fragPosition);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	//
	// Specular Light (Use Blinn-phong model with specular) <https://learnopengl.com/Advanced-Lighting/Advanced-Lighting>
	//
	float specularStrength = 0.5;
	vec3 viewDir = normalize(viewPosition - fragPosition);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normalVector, halfwayDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	//
	// Calculate Shadow
	//
	float shadow = shadowCalculation(lightDir);

	fragmentColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor.rgb, objectColor.a);
	//if (shadow > 0.5) fragmentColor = vec4(1, 0, 0, 1);			// DEBUG

	// Apply gamma correction
	const float gammaValue = 2.2f;
	fragmentColor.rgb = pow(fragmentColor.rgb, vec3(1.0f / gammaValue));
}