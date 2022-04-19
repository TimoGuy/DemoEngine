#version 430

in vec2 texCoord;
layout (location=0) out vec4 fragmentColor;
layout (location=1) out vec4 calculatedDepthValue;

uniform mat4 inverseProjectionMatrix;
uniform mat4 inverseViewMatrix;
uniform vec3 mainCameraPosition;
uniform float mainCameraZNear;
uniform float mainCameraZFar;

uniform float cloudLayerY;
uniform float cloudLayerThickness;
uniform float cloudNoiseMainSize;
uniform float cloudNoiseDetailSize;
uniform sampler3D cloudNoiseTexture;
uniform sampler3D cloudNoiseDetailTexture;
uniform float raymarchOffset;
uniform sampler3D atmosphericScattering;
uniform float cloudMaxDepth;
uniform float maxCloudscapeRadius;
uniform float maxRaymarchLength;

uniform float densityOffset;
uniform float densityMultiplier;
uniform float densityRequirement;

uniform float darknessThreshold;
uniform float lightAbsorptionTowardsSun;
uniform float lightAbsorptionThroughCloud;

uniform vec3 lightColor;
uniform vec3 lightDirection;

uniform vec4 phaseParameters;

// ext: zBuffer
uniform sampler2D depthTexture;









// NOTE: this is my rsi function after reading (https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection)
vec2 rsi(vec3 r0, vec3 rd, float sr)
{
    // RSI from https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code
    float b = dot(r0, rd);
    float c = dot(r0, r0) - sr * sr;

    // Exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0 && b > 0)
        return vec2(1e5, -1e5);

    float discr = b * b - c;

    // Neg discriminant corresponds to ray missing sphere
    if (discr < 0.0)
        return vec2(1e5, -1e5);

    // Ray now found to intersect sphere, compute smallest t value of intersection
    float discrRoot = sqrt(discr);
    return vec2(max(0.0, -b - discrRoot), -b + discrRoot);
}









const float cameraBaseHeight = 6376e2;
const float planetRadius = 6365e2;







// @NOTE: the NB_RAYMARCH_STEPS value is a base for marching thru,
// so the number of samples is going to be >64. 64 samples is if the ray
// is perfectly orthogonal to the cloudLayerThickness (the y thru the cloud layer)
#define NB_RAYMARCH_STEPS 64
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
    // Sample density cutoff from height below cloud layer
    //float distanceFromCloudY = cloudLayerY - (length(point + vec3(0, cameraBaseHeight, 0)) - cameraBaseHeight);
    float distanceFromCloudY = cloudLayerY - point.y;     // @NOTE: this is faster, but not as accurate. Doesn't seem like it makes any difference really.
    distanceFromCloudY /= cloudLayerThickness;
    const float densityMult = max(0.0, 1.0 - pow(2.0 * distanceFromCloudY - 1.0, 2.0));     // https://www.desmos.com/calculator/ltnhmcb1ow

    // Sample density from noise textures
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
    return (density - 0.15 * detailSubtract + densityOffset) * densityMultiplier * densityMult;   // @HARDCODE: the 0.25 * detailSubtract amount is hardcoded. Make a slider sometime eh!
}

float offsetAmount()
{
    float ditherPattern[16] = {
        0.0, 0.5, 0.125, 0.625,
        0.75, 0.22, 0.875, 0.375,
        0.1875, 0.6875, 0.0625, 0.5625,
        0.9375, 0.4375, 0.8125, 0.3125
    };
    uint index = (uint(gl_FragCoord.x) % 4) * 4 + uint(gl_FragCoord.y) % 4;
    return ditherPattern[index] * raymarchOffset;
}

float inScatterLightMarch(vec3 position)
{
    vec3 projectedLightDirection = -lightDirection / abs(lightDirection.y);
    vec3 inScatterDeltaPosition = projectedLightDirection * abs(position.y - cloudLayerY);        // @NOTE: this assumes that the lightdirection is always going to be pointing down (sun is above cloud layer)
    vec3 inScatterDeltaStepIncrement = inScatterDeltaPosition / float(NB_IN_SCATTER_RAYMARCH_STEPS);
        
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
    
    // Find the points to do raymarching
    vec3 r0 = mainCameraPosition + vec3(0, cameraBaseHeight, 0);
    vec3 rd = normalize(worldSpaceFragPosition - mainCameraPosition);

    vec2 isect_outer = rsi(r0, rd, cameraBaseHeight + cloudLayerY);
    if (isect_outer.x > isect_outer.y)
    {
        // Outer sphere collision failed. Abort clouds
        fragmentColor = vec4(0, 0, 0, 1.0);
        calculatedDepthValue = vec4(0.0);
        return;
    }

    vec2 isect_inner = rsi(r0, rd, cameraBaseHeight + cloudLayerY - cloudLayerThickness);
    vec2 isect_planet = rsi(r0, rd, planetRadius);

    if (isect_planet.x == 0.0)
    {
        // Inside planet. Abort clouds
        fragmentColor = vec4(0, 0, 0, 1.0);
        calculatedDepthValue = vec4(0.0);
        return;
    }

    // FIND t0
    float t0 = 0.0;
    float t1 = 0.0;
    
    // Inside inner boundary. Set t0 to end of inner sphere.
    if (isect_inner.x == 0.0)
    {
        // Occluded by planet. Abort clouds
        if (isect_inner.y > isect_planet.x)
        {
            fragmentColor = vec4(0, 0, 0, 1.0);
            calculatedDepthValue = vec4(0.0);
            return;
        }

        // Not occluded by planet!
        t0 = isect_inner.y;
        t1 = isect_outer.y;
    }
    // Outside outer boundary. Set t0 to start of outer sphere.
    else if (isect_outer.x > 0.0)
    {
        t0 = isect_outer.x;
        t1 = isect_inner.x;
    }
    // t0 is between cloud boundaries. t1 is whichever exiting boundary is closer. (inner.x, outer.y). When collision failed, 1e5 may not be a large enough number????  -Timo
    else
        t1 = min(isect_inner.x, isect_outer.y);



    // Setup raymarching
    calculatedDepthValue = vec4(mainCameraZFar, 0, 0, 1);
    
    vec3 currentPosition = mainCameraPosition + rd * t0;
    vec3 targetPosition  = mainCameraPosition + rd * t1;






    ////////////////////////vec3 currentPosition = mainCameraPosition;
    ////////////////////////vec3 projectedDeltaPosition = (worldSpaceFragPosition - mainCameraPosition);
    ////////////////////////projectedDeltaPosition /= abs(projectedDeltaPosition.y);       // @NOTE: This projects this to (0, +-1, 0)... so I guess it technically isn't being normalized though hahaha
    ////////////////////////
    ////////////////////////// Find the Y points where the collisions would happen
    ////////////////////////if (mainCameraPosition.y > cloudLayerY ||
    ////////////////////////    mainCameraPosition.y < cloudLayerY - cloudLayerThickness)
    ////////////////////////{
    ////////////////////////    float nearY = cloudLayerY - (projectedDeltaPosition.y > 0.0 ? cloudLayerThickness : 0.0);     // @NOTE: if the camera position is not contained within the cloud layer, then a nearY is calculated.
    ////////////////////////
    ////////////////////////    // @NOTE: essentially what this is checking is that the looking direction should be down when the nearY is below (+ * -), or looking direction upwards with nearY being above (- * +), so it should equate a neg. number at all times. If they're positive that means that it's a messup
    ////////////////////////    if ((mainCameraPosition.y - nearY) * (projectedDeltaPosition.y) > 0.0)
    ////////////////////////    {
    ////////////////////////        fragmentColor = vec4(0, 0, 0, 1.0);
    ////////////////////////        return;
    ////////////////////////    }
    ////////////////////////
    ////////////////////////    // Recalculate the currentPosition
    ////////////////////////    currentPosition = mainCameraPosition + projectedDeltaPosition * abs(mainCameraPosition.y - nearY);
    ////////////////////////}
    ////////////////////////
    ////////////////////////// Calc the raymarch start/end points
    ////////////////////////const float farY = cloudLayerY - (projectedDeltaPosition.y > 0.0 ? 0.0 : cloudLayerThickness);
    ////////////////////////vec3 targetPosition = mainCameraPosition + projectedDeltaPosition * abs(mainCameraPosition.y - farY);

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
            calculatedDepthValue = vec4(0, 0, 0, 1);        // @NOTE: this is a special value (< mainCameraZNear) so that it doesn't get processed in the flood fill and leak into the actual geometry when it's rendered (see pbr.frag with the depth comparison with the cloudDepthTexture)  -Timo.
            return;
        }

        if (depthTestSqr < depthTargetPos)
        {
            targetPosition = worldSpaceFragPosition;
            calculatedDepthValue = vec4(0, 0, 0, 1);    // @NOTE: this block means that there is geometry blocking the raymarch from going to infinity. If this fails, then keep the depth value as 0.0 (the bail value)
        }
    }

    //// Setup the raymarching magic!!!
    //if (length(currentPosition.xz - mainCameraPosition.xz) > maxCloudscapeRadius)
    //{
    //    fragmentColor = vec4(0, 0, 0, 1.0);
    //    return;
    //}

    /////////////////////////vec3 targetPositionFromCamera = targetPosition - mainCameraPosition;       @nOte: this is the code that limited the xz drawing for clouds
    /////////////////////////const float targetPositionLength = length(targetPositionFromCamera.xz);
    /////////////////////////if (targetPositionLength > maxCloudscapeRadius)
    /////////////////////////{
    /////////////////////////    targetPositionFromCamera = targetPositionFromCamera * maxCloudscapeRadius / targetPositionLength;
    /////////////////////////    targetPosition = mainCameraPosition + targetPositionFromCamera;
    /////////////////////////}
    
    vec3 deltaPosition = targetPosition - currentPosition;

    const float RAYMARCH_STEP_SIZE = cloudLayerThickness / float(NB_RAYMARCH_STEPS);    // @NOTE: this is a good number I think. The problem is that this could lead to too much raymarching, but I think it'll be fine. The transmittance should break the length of time that this calculates.
    const float rayLength = min(length(deltaPosition), maxRaymarchLength);

    // Fitting starting position to raymarching "grid"
    const vec3 deltaPositionNormalized = normalize(deltaPosition);
    vec3 deltaStepIncrement = deltaPositionNormalized * RAYMARCH_STEP_SIZE;
    const float offsetAmount = offsetAmount();
    const float distanceFromCameraToStartingPt = length(mainCameraPosition - currentPosition);
    const float griddedDistanceFromCameraToStartingPt = distanceFromCameraToStartingPt - mod(distanceFromCameraToStartingPt, RAYMARCH_STEP_SIZE);
    currentPosition = mainCameraPosition + deltaPositionNormalized * griddedDistanceFromCameraToStartingPt;     // @NOTE: This modulus op is for the top and bottom of the cloud layers to start at the correct position when the camera's position is above/below the cloud area.  -Timo
    currentPosition += deltaStepIncrement * offsetAmount;     // NOTE: when having this be a normalized and static offset, it really suffered when inside of clouds. Turns out that it didn't really change the look much compared to this method where we use the step incremeent size as the baseline for the offset value. Oh well. Looks like this way is just a little better, so we'll do it this way. When things get really far with far off stuff, I guess I really gotta make the step size shorter or something... and then just have the transmittance value be the limit for this... Idk man it's kinda hilarious. Maybe make some kind of distance endpoint and have the for loop go on forever... Idk.  -Timo

    //
    // RAYMARCH!!!
    //
    float transmittance = 1.0;
    float lightEnergy = 0.0;
    float phaseValue = phase(dot(deltaPositionNormalized, -lightDirection));
    float distanceTraveled = offsetAmount;      // @NOTE: offset the distanceTraveled by the starting offset value
    vec4 atmosValues = vec4(0.0);

    while (distanceTraveled < rayLength)
    {
        float density = sampleDensityAtPoint(currentPosition);

        if (density > densityRequirement)
        {
            float inScatterTransmittance = inScatterLightMarch(currentPosition);

            float d = density - densityRequirement;
            lightEnergy += inScatterTransmittance * phaseValue * lightAbsorptionThroughCloud;
            transmittance = 0.0;
            
            //calculatedDepthValue = vec4(vec3(clamp((distanceTraveled - mainCameraZNear) / (mainCameraZFar - mainCameraZNear), 0.0, 1.0)), 1.0);
            const float distanceTraveledActual = length(currentPosition - mainCameraPosition);
            calculatedDepthValue = vec4(vec3(clamp(distanceTraveledActual, mainCameraZNear, mainCameraZFar)), 1.0);
            atmosValues = texture(atmosphericScattering, vec3(texCoord, distanceTraveledActual / cloudMaxDepth * 3.2));
            break;
        }

        // Advance march
        currentPosition += deltaStepIncrement;
        distanceTraveled += RAYMARCH_STEP_SIZE;
    }

    fragmentColor =
        vec4(
            mix(
                atmosValues.rgb * (1.0 - transmittance),  // @ATMOS: combine atmospheric scattering
                lightColor * lightEnergy,
                atmosValues.a
            ),
            transmittance
        );
}
