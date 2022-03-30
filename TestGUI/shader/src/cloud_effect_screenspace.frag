#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;
uniform vec3 mainCameraPosition;

uniform float cloudLayerY;
uniform float cloudLayerThickness;
uniform sampler2DArray cloudNoiseTexture;

// ext: zBuffer
uniform sampler2D depthTexture;

#define NB_RAYMARCH_STEPS 10


void main()
{
	// Get WS position based off depth texture
    float z = texture(depthTexture, texCoord).x * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverseProjectionMatrix * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;   // Perspective division
    vec3 worldSpaceFragPosition = vec3(inverseViewMatrix * viewSpacePosition);

    vec3 currentPosition = mainCameraPosition;
    vec3 projectedDeltaPosition = (worldSpaceFragPosition - mainCameraPosition);
    projectedDeltaPosition /= abs(projectedDeltaPosition.y);       // @NOTE: This projects this to (0, +-1, 0)... so I guess it technically isn't being normalized though hahaha

    // Find the correct starting position if the camera position isn't inside the cloud volume already
    if (mainCameraPosition.y > cloudLayerY)
    {
        // If this is the furthest draw distance (skybox), then just have this extend to the cloud plane for the best raymarching eh.
        if (z == 1.0)
        {
            if (projectedDeltaPosition.y < 0.0)  // NOTE: won't resolve if looking upwards! Clouds will always be below
            {
                float differenceYToCloud = max(0.0, mainCameraPosition.y - cloudLayerY + 0.1);
                vec3 extendedDeltaPosition = projectedDeltaPosition * differenceYToCloud;
                worldSpaceFragPosition = mainCameraPosition + extendedDeltaPosition;
            }
        }

        // Return if fragPosition never even penetrated the cloud layer!
        if (worldSpaceFragPosition.y > cloudLayerY)
        {
            fragmentColor = vec4(0.0);
            return;
        }

        // Starting raymarching position (if camera ain't inside the cloud layer already bc that's a possibility yo)
        currentPosition = worldSpaceFragPosition;
    }

    vec3 targetPosition = worldSpaceFragPosition + (projectedDeltaPosition * cloudLayerThickness);
    vec3 realDeltaPosition = targetPosition - currentPosition;
    float rayLength = length(realDeltaPosition);
    vec3 deltaStepIncrement = realDeltaPosition / float(NB_RAYMARCH_STEPS);

    // Offset the start position
    float ditherPattern[16] = {
        0.0, 0.5, 0.125, 0.625,
        0.75, 0.22, 0.875, 0.375,
        0.1875, 0.6875, 0.0625, 0.5625,
        0.9375, 0.4375, 0.8125, 0.3125
    };
    uint index = (uint(gl_FragCoord.x) % 4) * 4 + uint(gl_FragCoord.y) % 4;
    currentPosition += deltaStepIncrement * ditherPattern[index];

    // RAYMARCH!!!!!
    for (int i = 0; i < NB_RAYMARCH_STEPS; i++)
    {
        // DO STUFF @TODO: continue here eh!!!! @WIP
        fdassdfasdfasdf
        // Advance raymarch
        currentPosition += deltaStepIncrement;
    }
}

    /*

    // Setup raymarching
    vec3 currentPosition = mainCameraPosition;
    vec3 deltaPosition = worldSpaceFragPosition - mainCameraPosition;
    float rayLength = length(deltaPosition);
    vec3 deltaPositionNormalized = deltaPosition / rayLength;
    float rayStepIncrement = min(rayLength, farPlane) / NB_RAYMARCH_STEPS;      // Clamp the furthest the rayLength can go is where shadows end (farPlane)
    vec3 stepVector = deltaPositionNormalized * rayStepIncrement;
    float lightDotView = dot(deltaPositionNormalized, -mainlightDirection);

    

    vec4 accumulatedCloud = vec4(0.0);      // @NOTE: once .a reaches 1, exit early

}*/
