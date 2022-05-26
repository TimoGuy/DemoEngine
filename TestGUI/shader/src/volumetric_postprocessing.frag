#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform vec3 mainlightDirection;


// Camera
layout (std140, binding = 3) uniform CameraInformation
{
    mat4 cameraProjection;
	mat4 cameraView;
	mat4 cameraProjectionView;
};

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;
uniform vec3 mainCameraPosition;

// ext: zBuffer
uniform sampler2D depthTexture;

// ext: csm_shadow (Simplified)
uniform sampler2DArray csmShadowMap;
layout (std140, binding = 0) uniform LightSpaceMatrices { mat4 lightSpaceMatrices[16]; };
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;   // number of frusta - 1
uniform float farPlane;


float simpleShadowSampleCSMLayer(vec3 lightDir, int layer, vec3 fragPosition)
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

    // Shadow sampling
    float pcfDepth = texture(csmShadowMap, vec3(projCoords.xy, layer)).r;
    float shadow = currentDepth > pcfDepth ? 1.0 : 0.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
    {
        shadow = 0.0;
    }

    return shadow;
}

float simpleShadowCalculationCSM(vec3 lightDir, vec3 fragPosition)
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

    return simpleShadowSampleCSMLayer(lightDir, layer, fragPosition);
}
//---------------------------------------------------------
// Mie scattering approximated with Henyey-Greenstein phase function.
#define NB_RAYMARCH_STEPS 10
const float PI = 3.14159265359;
const float G_SCATTERING = 0.7;
const float ACCUMULATION_CEILING = 15.0;        // NOTE: with the params above, the max seems to be 15.03

float computeScattering(float lightDotView)
{
    float result = 1.0 - G_SCATTERING * G_SCATTERING;
    result /= (4.0 * PI * pow(1.0 + G_SCATTERING * G_SCATTERING - (2.0 * G_SCATTERING) * lightDotView, 1.5));
    return result;
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

    // Setup raymarching
    vec3 currentPosition = mainCameraPosition;
    vec3 deltaPosition = worldSpaceFragPosition - mainCameraPosition;
    float rayLength = length(deltaPosition);
    vec3 deltaPositionNormalized = deltaPosition / rayLength;
    float rayStepIncrement = min(rayLength, farPlane) / NB_RAYMARCH_STEPS;      // Clamp the furthest the rayLength can go is where shadows end (farPlane)
    vec3 stepVector = deltaPositionNormalized * rayStepIncrement;
    float lightDotView = dot(deltaPositionNormalized, -mainlightDirection);

    // Offset the start position
    float ditherPattern[16] = {
        0.0, 0.5, 0.125, 0.625,
        0.75, 0.22, 0.875, 0.375,
        0.1875, 0.6875, 0.0625, 0.5625,
        0.9375, 0.4375, 0.8125, 0.3125
    };
    uint index = (uint(gl_FragCoord.x) % 4) * 4 + uint(gl_FragCoord.y) % 4;
    currentPosition += stepVector * ditherPattern[index];

    float accumulatedFog = 0.0;

    for (int i = 0; i < NB_RAYMARCH_STEPS; i++)
    {
        float shadow = simpleShadowCalculationCSM(mainlightDirection, currentPosition);
        if (shadow < 0.5)
        {
            accumulatedFog += computeScattering(lightDotView);
        }
        currentPosition += stepVector;
    }
    
    float remappedAccum = (1.0 - pow(1.0 - clamp(accumulatedFog / ACCUMULATION_CEILING, 0, 1), 5.0)) * ACCUMULATION_CEILING;
    //float remappedAccum = smoothstep(clamp(accumulatedFog / ACCUMULATION_CEILING, 0.0, 1.0), 0.0, 1.0) * ACCUMULATION_CEILING;
	fragColor = vec4(vec3(remappedAccum), 1.0);
}
