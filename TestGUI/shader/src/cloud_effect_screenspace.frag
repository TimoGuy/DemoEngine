#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;
uniform vec3 mainCameraPosition;

uniform float cloudLayerY;
uniform float cloudLayerThickness;
uniform vec4 cloudLayerTileSize;
uniform float cloudDensityMultiplier;
uniform float cloudDensityOffset;
uniform sampler2DArray cloudNoiseTexture;

uniform float densityOffset;
uniform float densityMultiplier;

uniform float darknessThreshold;
uniform float lightAbsorptionTowardsSun;
uniform float lightAbsorptionThroughCloud;

uniform vec3 lightColor;
uniform vec3 lightDirection;

// ext: zBuffer
uniform sampler2D depthTexture;

#define NB_RAYMARCH_STEPS 10
#define NB_IN_SCATTER_RAYMARCH_STEPS 5

vec4 textureArrayInterpolate(sampler2DArray tex, float numTexLayers, vec3 str)
{
    float zInterpolation = mod(str.z, 1.0);
    str.xy = mod(str.xy, 1.0);

    vec4 color =
        mix(
            texture(tex, vec3(str.xy, floor(zInterpolation * numTexLayers))).rgba,
            texture(tex, vec3(str.xy, mod(floor(zInterpolation * numTexLayers + 1.0), numTexLayers))).rgba,
            zInterpolation
        );

    return (color + densityOffset) * densityMultiplier;
}

// Henyey-Greenstein
float hg(float a, float g)
{
    float g2 = g*g;
    return (1-g2) / (4*3.1415*pow(1+g2-2*g*(a), 1.5));
}

float phase(float a)
{
    float blend = .5;
    vec4 phaseParams = vec4(0.83, 0.3, 0.8, 0.15);      // @HARDCODE: Forward scattering, Backscattering, BaseBrightness, PhaseFactor
    float hgBlend = hg(a, phaseParams.x) * (1 - blend) + hg(a, -phaseParams.y) * blend;
    return phaseParams.z + hgBlend * phaseParams.w;
}

// @DEBUG: @NOTE: this is simply for debugggin purposes!!!!!!
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

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

    // Find the Y points where the collisions would happen
    float nearY = currentPosition.y;
    if (currentPosition.y > cloudLayerY ||
        currentPosition.y < cloudLayerY - cloudLayerThickness)
    {
        nearY = cloudLayerY - (projectedDeltaPosition.y > 0.0 ? cloudLayerThickness : 0.0);     // @NOTE: if the camera position is not contained within the cloud layer, then a nearY is calculated.

        // @NOTE: essentially what this is checking is that the looking direction should be down when the nearY is below (+ * -), or looking direction upwards with nearY being above (- * +), so it should equate a neg. number at all times. If they're positive that means that it's a messup
        if ((mainCameraPosition.y - nearY) * (projectedDeltaPosition.y) > 0.0)
        {
            fragmentColor = vec4(0, 0, 0, 1.0);
            return;
        }
    }
    float farY = cloudLayerY - (projectedDeltaPosition.y > 0.0 ? 0.0 : cloudLayerThickness);

    // Calc the raymarch start/end points
    currentPosition     = mainCameraPosition + projectedDeltaPosition * abs(mainCameraPosition.y - nearY);
    vec3 targetPosition = mainCameraPosition + projectedDeltaPosition * abs(mainCameraPosition.y - farY);

    // Check for depth test
    if (z != 1.0)
    {
        vec3 deltaWSFragPos     = worldSpaceFragPosition - mainCameraPosition;
        vec3 deltaCurrentPos    = currentPosition - mainCameraPosition;
        vec3 deltaTargetPos     = targetPosition - mainCameraPosition;
        float depthTestSqr      = dot(deltaWSFragPos, deltaWSFragPos);
        float depthCurrentPos   = dot(deltaCurrentPos, deltaCurrentPos);
        float depthTargetPos    = dot(deltaTargetPos, deltaTargetPos);

        if (depthTestSqr < depthCurrentPos)
        {
            fragmentColor = vec4(0, 0, 0, 1.0);
            return;
        }

        if (depthTestSqr < depthTargetPos)
        {
            targetPosition = worldSpaceFragPosition;
        }
    }

    // Setup the raymarching magic!!!
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
    float accumulatedDensity = 0.0;
    vec4 sampleScale = 1.0 / cloudLayerTileSize;
    float stepWeight = rayLength / float(NB_RAYMARCH_STEPS);

    float phaseValue = phase(dot(realDeltaPosition / rayLength, lightDirection));
    float transmittance = 1.0;
    vec3 lightEnergy = vec3(0.0);

    for (int i = 0; i < NB_RAYMARCH_STEPS; i++)
    {
        float densityAtStep = 0.0;

        //
        // Calculate the raymarch towards the light
        //
        vec3 inScatterCurrentPosition = currentPosition;
        vec3 projectedLightDirection = -lightDirection / abs(lightDirection.y);
        vec3 inScatterDeltaPosition = projectedLightDirection * abs(inScatterCurrentPosition.y - cloudLayerY);        // @NOTE: this assumes that the lightdirection is always going to be pointing down (sun is above cloud layer)
        vec3 inScatterDeltaStepIncrement = inScatterDeltaPosition / float(NB_IN_SCATTER_RAYMARCH_STEPS);
        float inScatterStepWeight = length(inScatterDeltaStepIncrement);
        float inScatterDensity = 0.0;

        for (int j = 0; j < NB_IN_SCATTER_RAYMARCH_STEPS; j++)
        {
            vec4 noise = textureArrayInterpolate(cloudNoiseTexture, 128.0, sampleScale.r * inScatterCurrentPosition.xzy);
            float density =
                0.5333333 * noise.r
			    + 0.2666667 * noise.g
			    + 0.1333333 * noise.b
			    + 0.0666667 * noise.a;

            if (j == 0)
            {
                densityAtStep = density;
                if (densityAtStep <= 0.0)
                    break;
            }

            inScatterDensity += max(0.0, density) * inScatterStepWeight;
            if (inScatterDensity > 7.6)   // @NOTE: this is essentially 0 when it's e^-x
                break;

            // Advance raymarch
            inScatterCurrentPosition += inScatterDeltaStepIncrement;
        }
        float inScatterTransmittance = darknessThreshold + exp(-inScatterDensity * lightAbsorptionTowardsSun) * (1.0 - darknessThreshold);

        //
        // Add on the inScatterTransmittance and lightEnergy
        //
        if (densityAtStep > 0.0)
        {
            lightEnergy += densityAtStep * stepWeight * transmittance * inScatterTransmittance * phaseValue;
            transmittance *= exp(-densityAtStep * stepWeight * lightAbsorptionThroughCloud);

            if (transmittance < 0.01)
                break;
        }

        // Advance raymarch
        currentPosition += deltaStepIncrement;
    }


    //fragmentColor = vec4(hsv2rgb(vec3(float(i) / float(NB_RAYMARCH_STEPS), 1.0, 1.0)), 1.0);
    fragmentColor = vec4(lightColor * lightEnergy, transmittance);
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
