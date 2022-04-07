#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;
uniform vec3 mainCameraPosition;

uniform float cloudLayerY;
uniform float cloudLayerThickness;
uniform float cloudNoiseMainSize;
uniform float cloudNoiseDetailSize;
uniform sampler3D cloudNoiseTexture;
uniform sampler3D cloudNoiseDetailTexture;
uniform float raymarchOffset;
uniform float maxCloudscapeRadius;
uniform float maxRaymarchLength;

uniform float densityOffset;
uniform float densityMultiplier;

uniform float darknessThreshold;
uniform float lightAbsorptionTowardsSun;
uniform float lightAbsorptionThroughCloud;

uniform vec3 lightColor;
uniform vec3 lightDirection;

uniform vec4 phaseParameters;

// ext: zBuffer
uniform sampler2D depthTexture;

#define NB_RAYMARCH_STEPS 16
#define NB_IN_SCATTER_RAYMARCH_STEPS 8

vec4 textureArrayInterpolate(sampler3D tex, float numTexLayers, vec3 str)
{
    //float zInterpolation = mod(abs(str.z), 1.0);
    //str.xy = mod(str.xy, 1.0);
    //
    //vec4 color =
    //    mix(
    //        texture(tex, vec3(str.xy, mod(floor(zInterpolation * numTexLayers - 1.0), numTexLayers))).rgba,
    //        texture(tex, vec3(str.xy, floor(zInterpolation * numTexLayers))).rgba,
    //        zInterpolation
    //    );

    // SAMPLER3D method
    vec4 color = texture(tex, str).rgba;

    //return (color + densityOffset) * densityMultiplier;
    return color;
}

float sampleDensityAtPoint(vec3 point)
{
    const float sampleScale = 1.0 / cloudNoiseMainSize;
    const float sampleScaleDetailed = 1.0 / cloudNoiseDetailSize;
    vec4 noise = textureArrayInterpolate(cloudNoiseTexture, 128.0, sampleScale * point.xzy);
    vec4 noiseDetail = textureArrayInterpolate(cloudNoiseDetailTexture, 32.0, sampleScaleDetailed * point.xzy);
    float density =
        0.5333333 * noise.r
		+ 0.2666667 * noise.g
		+ 0.1333333 * noise.b
		+ 0.0666667 * noise.a;
    float detailSubtract =
        0.533333333 * noiseDetail.r
        + 0.2666667 * noiseDetail.g
        + 0.1333333 * noiseDetail.b;
    return (density - 0.25 * detailSubtract + densityOffset) * densityMultiplier;   // @HARDCODE: the 0.25 * detailSubtract amount is hardcoded. Make a slider sometime eh!
}

vec3 offsetPoint(vec3 point, vec3 direction)
{
    float ditherPattern[16] = {
        0.0, 0.5, 0.125, 0.625,
        0.75, 0.22, 0.875, 0.375,
        0.1875, 0.6875, 0.0625, 0.5625,
        0.9375, 0.4375, 0.8125, 0.3125
    };
    uint index = (uint(gl_FragCoord.x) % 4) * 4 + uint(gl_FragCoord.y) % 4;
    return point + direction * ditherPattern[index] * raymarchOffset;
}

float inScatterLightMarch(vec3 position)
{
    vec3 projectedLightDirection = -lightDirection / abs(lightDirection.y);
    vec3 inScatterDeltaPosition = projectedLightDirection * abs(position.y - cloudLayerY);        // @NOTE: this assumes that the lightdirection is always going to be pointing down (sun is above cloud layer)
    vec3 inScatterDeltaStepIncrement = inScatterDeltaPosition / float(NB_IN_SCATTER_RAYMARCH_STEPS);
    
    //position = offsetPoint(position, inScatterDeltaStepIncrement);
        
    float inScatterDensity = 0.0;
    float inScatterStepWeight = length(inScatterDeltaStepIncrement) / float(NB_IN_SCATTER_RAYMARCH_STEPS);

    for (int i = 0; i < NB_IN_SCATTER_RAYMARCH_STEPS - 1; i++)
    {
        inScatterDensity += max(0.0, sampleDensityAtPoint(position) * inScatterStepWeight);
        position += inScatterDeltaStepIncrement;        // @NOTE: since density is already sampled at the first spot, move first, then sample density!
    }

    float transmittance = exp(-inScatterDensity * lightAbsorptionTowardsSun);
    return darknessThreshold + transmittance * (1.0 - darknessThreshold);
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
    float hgBlend = hg(a, phaseParameters.x) * (1 - blend) + hg(a, -phaseParameters.y) * blend;
    return phaseParameters.z + hgBlend * phaseParameters.w;
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
    if (mainCameraPosition.y > cloudLayerY ||
        mainCameraPosition.y < cloudLayerY - cloudLayerThickness)
    {
        float nearY = cloudLayerY - (projectedDeltaPosition.y > 0.0 ? cloudLayerThickness : 0.0);     // @NOTE: if the camera position is not contained within the cloud layer, then a nearY is calculated.

        // @NOTE: essentially what this is checking is that the looking direction should be down when the nearY is below (+ * -), or looking direction upwards with nearY being above (- * +), so it should equate a neg. number at all times. If they're positive that means that it's a messup
        if ((mainCameraPosition.y - nearY) * (projectedDeltaPosition.y) > 0.0)
        {
            fragmentColor = vec4(0, 0, 0, 1.0);
            return;
        }

        // Recalculate the currentPosition
        currentPosition = mainCameraPosition + projectedDeltaPosition * abs(mainCameraPosition.y - nearY);
    }

    // Calc the raymarch start/end points
    const float farY = cloudLayerY - (projectedDeltaPosition.y > 0.0 ? 0.0 : cloudLayerThickness);
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
    if (length(currentPosition.xz - mainCameraPosition.xz) > maxCloudscapeRadius)
    {
        fragmentColor = vec4(0, 0, 0, 1.0);
        return;
    }

    vec3 targetPositionFromCamera = targetPosition - mainCameraPosition;
    const float targetPositionLength = length(targetPositionFromCamera.xz);
    if (targetPositionLength > maxCloudscapeRadius)
    {
        targetPositionFromCamera = targetPositionFromCamera * maxCloudscapeRadius / targetPositionLength;
        targetPosition = mainCameraPosition + targetPositionFromCamera;
    }
    
    vec3 deltaPosition = targetPosition - currentPosition;

    @TODO: for some reason when going into the cloud, you see more cloud than you're supposed to 

    const float MAX_RAYMARCH_DISTANCE = cloudLayerThickness / float(NB_RAYMARCH_STEPS);    // @NOTE: this is a good number I think. The problem is that this could lead to too much raymarching, but I think it'll be fine. The transmittance should break the length of time that this calculates.
    const float rayLength = maxRaymarchLength;//min(length(deltaPosition), maxRaymarchLength);      // @NOTE: the raymarching shader kept crashing the game bc of how long it was taking for some really long raymarched clouds took, so I think what I'll do is keep this in. I don't know how much this'll affect the look of the clouds, but setting some kind of max limit would be good to keep.  -Timo PS: it's 2am.
    const vec3 deltaPositionNormalized = normalize(deltaPosition);
    float stepSize = rayLength / float(NB_RAYMARCH_STEPS);
    if (stepSize > MAX_RAYMARCH_DISTANCE)
    {
        const float idealStepSize = rayLength / MAX_RAYMARCH_DISTANCE;
        const float remainder = mod(idealStepSize, 1.0);
        const float idealStepSize_rounded = floor(idealStepSize);
        const float weightPadding = remainder / idealStepSize_rounded;
        stepSize = MAX_RAYMARCH_DISTANCE * (1.0 + weightPadding);
    }
    vec3 deltaStepIncrement = deltaPositionNormalized * stepSize;
    currentPosition = offsetPoint(currentPosition, deltaStepIncrement);     // NOTE: when having this be a normalized and static offset, it really suffered when inside of clouds. Turns out that it didn't really change the look much compared to this method where we use the step incremeent size as the baseline for the offset value. Oh well. Looks like this way is just a little better, so we'll do it this way. When things get really far with far off stuff, I guess I really gotta make the step size shorter or something... and then just have the transmittance value be the limit for this... Idk man it's kinda hilarious. Maybe make some kind of distance endpoint and have the for loop go on forever... Idk.  -Timo

    //
    // RAYMARCH!!!
    //
    float transmittance = 1.0;
    float lightEnergy = 0.0;
    float phaseValue = phase(dot(deltaPositionNormalized, -lightDirection));
    float distanceTraveled = 0.0;

    while (distanceTraveled < rayLength)
    {
        float density = sampleDensityAtPoint(currentPosition);

        if (density > 0.0)
        {
            float inScatterTransmittance = inScatterLightMarch(currentPosition);
            float d = density * stepSize * lightAbsorptionThroughCloud;

            lightEnergy += density * stepSize * transmittance * (1.0 - exp(-d * 2.0)) * inScatterTransmittance * phaseValue;       // Beer's-Powder approximation (https://www.guerrilla-games.com/media/News/Files/The-Real-time-Volumetric-Cloudscapes-of-Horizon-Zero-Dawn.pdf PAGE 64)
            transmittance *= exp(-d);

            if (transmittance < 0.01)
                break;


            /*
            // Prep for inscatter raymarching
            vec3 inScatterCurrentPosition = currentPosition;
            vec3 projectedLightDirection = -lightDirection / abs(lightDirection.y);
            vec3 inScatterDeltaPosition = projectedLightDirection * abs(inScatterCurrentPosition.y - cloudLayerY);        // @NOTE: this assumes that the lightdirection is always going to be pointing down (sun is above cloud layer)
            vec3 inScatterDeltaStepIncrement = inScatterDeltaPosition / float(NB_IN_SCATTER_RAYMARCH_STEPS);
        
            float inScatterDensity = 0.0;
            float inScatterStepWeight = length(inScatterDeltaStepIncrement);
        
            for (int j = 0; j < NB_IN_SCATTER_RAYMARCH_STEPS; j++)
            {
                float density = sampleDensityAtPoint(inScatterCurrentPosition);
                inScatterDensity += max(0.0, (density + densityOffset) * densityMultiplier) * inScatterStepWeight;

                //if (inScatterDensity > 10.0)
                //    break;

                // Advance march
                inScatterCurrentPosition += inScatterDeltaStepIncrement;
            }

            if (inScatterDensity > 0.0)
            {
                //lightEnergy += densityAtStep * stepSize * transmittance * inScatterTransmittance * phaseValue;
                transmittance *= exp(-inScatterDensity * stepSize * lightAbsorptionThroughCloud);
            }

            totalDensity += inScatterDensity * stepSize;

            if (transmittance < 0.01)
                break;

            //if (totalDensity > 10.0)
            //    break;

            */
        }

        // Advance march
        currentPosition += deltaStepIncrement;
        distanceTraveled += stepSize;
    }

    fragmentColor = vec4(lightColor * lightEnergy, transmittance);
}









    /*

    // RAYMARCH!!!!!
    float accumulatedDensity = 0.0;
    vec4 sampleScale = 1.0 / cloudLayerTileSize;
    float stepWeight = rayLength / float(NB_RAYMARCH_STEPS);

    float phaseValue = phase(dot(realDeltaPosition / rayLength, -lightDirection));
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
            //if (inScatterDensity > 7.6)   // @NOTE: this is essentially 0 when it's e^-x
            //    break;

            // Advance raymarch
            inScatterCurrentPosition += inScatterDeltaStepIncrement;

            if (j == 0)
                inScatterCurrentPosition += inScatterDeltaStepIncrement * ditherPattern[index];
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


*/


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
