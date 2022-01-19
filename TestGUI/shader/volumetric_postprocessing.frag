#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D depthTexture;
layout(binding = 1) uniform sampler2DArray csmShadowMap;

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;
uniform float zNear;
uniform float zFar;

uniform vec3 mainlightDirection;

layout (std140, binding = 0) uniform LightSpaceMatrices { mat4 lightSpaceMatrices[16]; };
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1
uniform mat4 cameraView;
uniform float farPlane;


float sampleCSMShadowMapLinear(vec2 coords, vec2 texelSize, int layer, float compare)           // https://www.youtube.com/watch?v=yn5UJzMqxj0 (29:27)
{
    vec2 pixelPos = coords / texelSize + vec2(0.5);
    vec2 fracPart = fract(pixelPos);
    vec2 startTexel = (pixelPos - fracPart - vec2(0.5)) * texelSize;                // NOTE: -vec2(0.5) fixes problem of one cascade being off from another.

    float blTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(0.0, 0.0), layer)).r);
    float brTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(texelSize.x, 0.0), layer)).r);
    float tlTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(0.0, texelSize.y), layer)).r);
    float trTexel = step(compare, texture(csmShadowMap, vec3(startTexel + vec2(texelSize.x, texelSize.y), layer)).r);
    
    float mixA = mix(blTexel, tlTexel, fracPart.y);
    float mixB = mix(brTexel, trTexel, fracPart.y);

    return mix(mixA, mixB, fracPart.x);
}

float shadowSampleCSMLayer(vec3 lightDir, int layer, vec3 fragPosition)
{
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosition, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)     // NOTE: This is tuned for 1024x1024 stable shadow maps
    vec3 normal = vec3(0, 1, 0);  //normalize(normalVector);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float distanceMultiplier = pow(0.45, layer);      // NOTE: this worked for stable fit shadows
    //float distanceMultiplier = 0.5;                   // NOTE: this is for close fit shadows... but I cannot get them to look decent

    if (layer == cascadeCount)
    {
        bias *= 1 / (farPlane * distanceMultiplier);
    }
    else
    {
        bias *= 1 / (cascadePlaneDistances[layer] * distanceMultiplier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(csmShadowMap, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            //float pcfDepth = texture(csmShadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;        // @Debug: not filtered shadows
            //shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
            shadow += sampleCSMShadowMapLinear(projCoords.xy + vec2(x, y) * texelSize, texelSize, layer, currentDepth - bias);
        }
    }
    shadow /= 9.0;
    shadow = 1.0 - shadow;

    //if (shadow > 0.01)        // For debugging purposes
    //    shadow = 1.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
    {
        shadow = 0.0;
    }

    return shadow;
}

float shadowCalculationCSM(vec3 lightDir, vec3 fragPosition)
{
	// select cascade layer
    vec4 fragPosViewSpace = cameraView * vec4(fragPosition, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }
    if (depthValue > farPlane)                  // (TODO: This works, but I don't want it bc it can lop off more than half of the last cascade. This should probs account for the bounds of the shadow cascade (perhaps use projCoords.x and .y too????))
    {
        return 0.0;
    }

    // Fading between shadow cascades
    float fadingEdgeAmount = 2.5 * (layer + 1);  //(layer == cascadeCount) ? 15.0 : 2.5;        // <---- Method that I tried to implement with close fit shadows... but never really worked.
    float visibleAmount = -1;
    if (layer == cascadeCount)
        visibleAmount = clamp(farPlane - depthValue, 0.0, fadingEdgeAmount) / fadingEdgeAmount;
    else
        visibleAmount = clamp(cascadePlaneDistances[layer] - depthValue, 0.0, fadingEdgeAmount) / fadingEdgeAmount;

    // Sample the shadow map(s)
    float shadow1 = 0, shadow2 = 0;
    if (layer != cascadeCount && visibleAmount < 1.0)
        shadow2 = shadowSampleCSMLayer(lightDir, layer + 1, fragPosition);
    shadow1 = shadowSampleCSMLayer(lightDir, layer, fragPosition);

    // Mix sampled shadows       (TODO: This works, but I don't want it bc it can lop off more than half of the last cascade. This should probs account for the bounds of the shadow cascade (perhaps use projCoords.x and .y too????))
    return mix(shadow2, shadow1, visibleAmount);
}
//---------------------------------------------------------
void main()
{
    // Get WS position based off depth texture
    float z = texture(depthTexture, texCoord).x * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverseProjectionMatrix * clipSpacePosition;

    viewSpacePosition /= viewSpacePosition.w;   // Perspective division

    vec3 worldSpaceFragPosition = vec3(inverseViewMatrix * viewSpacePosition);

    float shadow = shadowCalculationCSM(mainlightDirection, worldSpaceFragPosition);

	fragColor = vec4(vec3(shadow), 1.0);
}
