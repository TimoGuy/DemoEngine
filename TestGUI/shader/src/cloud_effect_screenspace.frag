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
uniform vec3 cloudNoiseDetailOffset;
uniform sampler3D cloudNoiseTexture;
uniform sampler3D cloudNoiseDetailTexture;
uniform float raymarchOffset;
uniform int raymarchOffsetDitherIndexOffset;
uniform sampler3D atmosphericScattering;
uniform float cloudMaxDepth;
uniform vec2 sampleSmoothEdgeNearFar;
uniform float farRaymarchStepsizeMultiplier;
//uniform float maxRaymarchLength;

uniform float densityOffset;
uniform float densityMultiplier;
uniform float densityRequirement;

uniform float ambientDensity;
uniform float irradianceStrength;
uniform float lightAbsorptionTowardsSun;
uniform float lightAbsorptionThroughCloud;

uniform vec3 lightColor;
uniform vec3 lightDirection;

uniform vec4 phaseParameters;

// ext: zBuffer
uniform sampler2D depthTexture;


// ext: PBR daynight cycle
uniform samplerCube irradianceMap;
//uniform samplerCube prefilterMap;
//uniform sampler2D brdfLUT;









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
const float planetRadius = 6361e2;  // 6365e2;







// @NOTE: the NB_RAYMARCH_STEPS value is a base for marching thru,
// so the number of samples is going to be >64. 64 samples is if the ray
// is perfectly orthogonal to the cloudLayerThickness (the y thru the cloud layer)

// @NOTE: NB_RAYMARCH_STEPS could be 16 for low quality.
#define NB_RAYMARCH_STEPS 64
#define NB_IN_SCATTER_RAYMARCH_STEPS 8

float maxRaymarchLength()       // @NOTE: this is probs best to do cpu side instead of on every pixel, especially since this is using just consts.
{
    // Use pythagorean's theorem
    const float outer_radius = cameraBaseHeight + cloudLayerY;
    const float inner_radius = cameraBaseHeight + cloudLayerY - cloudLayerThickness;
    const float t = sqrt(outer_radius * outer_radius - inner_radius * inner_radius);
    return t;       // @NOTE: This times 2.0 is the max value with the two RSI's... so we need to cut it off at half the amount
}

vec4 textureArrayInterpolate(sampler3D tex, float numTexLayers, vec3 str)
{
    // SAMPLER3D method
    vec4 color = texture(tex, str).rgba;
    return color;
}

float sampleDensityAtPoint(vec3 point)
{
    // Sample density cutoff from height below cloud layer
    float distanceFromCloudY = cloudLayerY - (length(point + vec3(0, cameraBaseHeight, 0)) - cameraBaseHeight);		// @NOTE: switched to this. Due to long distance clouds creating artifacts.  @NOTE: Hey, it seems like this doesn't hurt performance too much. That length() function is FAST! It took 2-5fps (270->265fps), so that's little!
    //float distanceFromCloudY = cloudLayerY - point.y;     // @NOTE: this is faster, but not as accurate. Doesn't seem like it makes any difference really.
    distanceFromCloudY /= cloudLayerThickness;
    const float densityMult = max(0.0, 1.0 - pow(2.0 * distanceFromCloudY - 1.0, 2.0));     // https://www.desmos.com/calculator/ltnhmcb1ow

    // Sample density from noise textures
    const float sampleScale = 1.0 / cloudNoiseMainSize;
    const float sampleScaleDetailed = 1.0 / cloudNoiseDetailSize;
    vec4 noise = textureArrayInterpolate(cloudNoiseTexture, 128.0, sampleScale * point.xzy);
    vec4 noiseDetail = textureArrayInterpolate(cloudNoiseDetailTexture, 32.0, sampleScaleDetailed * (point + cloudNoiseDetailOffset).xzy);
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
    return ditherPattern[(index + raymarchOffsetDitherIndexOffset) % 16] * raymarchOffset;
    //return ditherPattern[index] * raymarchOffset;
}

float inScatterLightMarch(vec3 position)
{
    vec3 projectedLightDirection = -lightDirection / abs(lightDirection.y);
    vec3 inScatterDeltaPosition = projectedLightDirection * abs(position.y - cloudLayerY);        // @NOTE: this assumes that the lightdirection is always going to be pointing down (sun is above cloud layer)
    vec3 inScatterDeltaStepIncrement = inScatterDeltaPosition / float(NB_IN_SCATTER_RAYMARCH_STEPS);

    //position += inScatterDeltaStepIncrement * offsetAmount;
        
    float inScatterDensity = 0.0;
    float inScatterStepWeight = length(inScatterDeltaStepIncrement) / float(NB_IN_SCATTER_RAYMARCH_STEPS);

    for (int i = 0; i < NB_IN_SCATTER_RAYMARCH_STEPS - 1; i++)
    {
        inScatterDensity += max(0.0, sampleDensityAtPoint(position) * inScatterStepWeight);
        position += inScatterDeltaStepIncrement;        // @NOTE: since density is already sampled at the first spot, move first, then sample density!
    }

    float transmittance = exp(-inScatterDensity * lightAbsorptionTowardsSun);
    return transmittance;
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
        //t1 = isect_inner.x;
        //t1 = (isect_inner.y > isect_planet.x) ? isect_inner.x : isect_inner.y;        // @TRIPPY
        t1 = (isect_planet.x < isect_outer.y) ? isect_inner.x : isect_outer.y;      // @NOTE: so this looks so much better, however, it's not perfect. And I think the reason behind it is the raylength being very large, forcing the raymarch in the far area to be sparser
    }
    // t0 is between cloud boundaries. t1 is whichever exiting boundary is closer. (inner.x, outer.y). When collision failed, 1e5 may not be a large enough number????  -Timo
    else
        //t1 = min(isect_inner.x, isect_outer.y);
        //t1 = min((isect_inner.y > isect_planet.x) ? isect_inner.x : isect_inner.y, isect_outer.y);        // @TRIPPY
        t1 = min((isect_planet.x < isect_outer.y) ? isect_inner.x : isect_outer.y, isect_outer.y);      // @NOTE: so this looks so much better, however, it's not perfect. And I think the reason behind it is the raylength being very large, forcing the raymarch in the far area to be sparser



    // Setup raymarching
    calculatedDepthValue = vec4(mainCameraZFar, 0, 0, 1);
    
    const float offsetAmount = offsetAmount();
    const float rayLength = min(t1 - t0, maxRaymarchLength());
    float distanceTraveled = offsetAmount;      // @NOTE: offset the distanceTraveled by the starting offset value

    const float NEAR_RAYMARCH_STEP_SIZE = cloudLayerThickness / float(NB_RAYMARCH_STEPS);
    const float FAR_RAYMARCH_STEP_SIZE = rayLength / float(NB_RAYMARCH_STEPS) * farRaymarchStepsizeMultiplier;
    float RAYMARCH_STEP_SIZE = mix(NEAR_RAYMARCH_STEP_SIZE, FAR_RAYMARCH_STEP_SIZE, smoothstep(sampleSmoothEdgeNearFar.x, sampleSmoothEdgeNearFar.y, t0 + distanceTraveled));

    // Resize t0 to line up with the view space orientation
    t0 = floor(t0 / RAYMARCH_STEP_SIZE) * RAYMARCH_STEP_SIZE;
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

    ////////////////// Setup the raymarching magic!!!
    ////////////////if (length(currentPosition.xz - mainCameraPosition.xz) > maxCloudscapeRadius)
    ////////////////{
    ////////////////    fragmentColor = vec4(0, 0, 0, 1.0);
    ////////////////    return;
    ////////////////}

    /////////////////////////vec3 targetPositionFromCamera = targetPosition - mainCameraPosition;       @nOte: this is the code that limited the xz drawing for clouds
    /////////////////////////const float targetPositionLength = length(targetPositionFromCamera.xz);
    /////////////////////////if (targetPositionLength > maxCloudscapeRadius)
    /////////////////////////{
    /////////////////////////    targetPositionFromCamera = targetPositionFromCamera * maxCloudscapeRadius / targetPositionLength;
    /////////////////////////    targetPosition = mainCameraPosition + targetPositionFromCamera;
    /////////////////////////}
    
    vec3 deltaPosition = targetPosition - currentPosition;
    const vec3 deltaPositionNormalized = normalize(deltaPosition);
    vec3 deltaStepIncrement = deltaPositionNormalized * RAYMARCH_STEP_SIZE;
    
    //// Fitting starting position to raymarching "grid"
    //const float distanceFromCameraToStartingPt = length(mainCameraPosition - currentPosition);
    //const float griddedDistanceFromCameraToStartingPt = distanceFromCameraToStartingPt - mod(distanceFromCameraToStartingPt, RAYMARCH_STEP_SIZE);
    //currentPosition = mainCameraPosition + deltaPositionNormalized * griddedDistanceFromCameraToStartingPt;     // @NOTE: This modulus op is for the top and bottom of the cloud layers to start at the correct position when the camera's position is above/below the cloud area.  -Timo
    
    //// Offset the starting position
    //currentPosition += deltaStepIncrement * offsetAmount;     // NOTE: when having this be a normalized and static offset, it really suffered when inside of clouds. Turns out that it didn't really change the look much compared to this method where we use the step incremeent size as the baseline for the offset value. Oh well. Looks like this way is just a little better, so we'll do it this way. When things get really far with far off stuff, I guess I really gotta make the step size shorter or something... and then just have the transmittance value be the limit for this... Idk man it's kinda hilarious. Maybe make some kind of distance endpoint and have the for loop go on forever... Idk.  -Timo

    //
    // RAYMARCH!!!
    //
    // @NOTE: These are the moments when the clouds went from the old method to the new method.
    //      https://github.com/TimoGuy/DemoEngine/commit/0ca87fbcf02253911a86fa585003b35a6161e272
    //      https://github.com/TimoGuy/DemoEngine/commit/c23da6c18c33c25c5f05dab5b41948facda1e6e7
    //
    // @NOTE: Reverted from the new method of signed distance fields. The performance just wasn't as good.
    //
    float transmittance = 1.0;
    float lightEnergy = 0.0;
    float phaseValue = phase(dot(deltaPositionNormalized, -lightDirection));
    vec3 ambientLightEnergy = vec3(0.0);
    vec4 atmosValues = vec4(0.0);

    int count = 0;

    while (distanceTraveled < rayLength)
    {
        count++;
        // Keep the offset relevant depending on the RAYMARCH_STEP_SIZE
        const vec3 offsetCurrentPosition = currentPosition + deltaStepIncrement * offsetAmount;

        float density = sampleDensityAtPoint(offsetCurrentPosition);

        if (density > densityRequirement)
        {
            float inScatterTransmittance = inScatterLightMarch(offsetCurrentPosition);

            float d = density - densityRequirement;
            lightEnergy += inScatterTransmittance * phaseValue * lightAbsorptionThroughCloud;
            transmittance = 0.0;
            
            //calculatedDepthValue = vec4(vec3(clamp((distanceTraveled - mainCameraZNear) / (mainCameraZFar - mainCameraZNear), 0.0, 1.0)), 1.0);
            const float distanceTraveledActual = length(offsetCurrentPosition - mainCameraPosition);
            calculatedDepthValue = vec4(vec3(clamp(distanceTraveledActual, mainCameraZNear, mainCameraZFar)), 1.0);
            atmosValues = texture(atmosphericScattering, vec3(texCoord, distanceTraveledActual / cloudMaxDepth));
            
            // Calculate Ambient Light Energy (https://shaderbits.com/blog/creating-volumetric-ray-marcher)
            const vec3 directionFromOrigin = normalize(offsetCurrentPosition + vec3(0, cameraBaseHeight, 0));
            float shadowDensity = 0.0;
            shadowDensity += sampleDensityAtPoint(offsetCurrentPosition + directionFromOrigin * 0.05);
            shadowDensity += sampleDensityAtPoint(offsetCurrentPosition + directionFromOrigin * 0.10);
            shadowDensity += sampleDensityAtPoint(offsetCurrentPosition + directionFromOrigin * 0.20);

            // Calculate the irradiance in the direction of the ray
            const vec3 sampleDirection = vec3(0, -1, 0);  // @NOTE: the sampleDirection was changed from the view direction to straight down bc ambient light reflecting off the ground would be in the straight down view direction.
	        vec3 irradiance = texture(irradianceMap, sampleDirection).rgb;
            ambientLightEnergy = exp(-shadowDensity * ambientDensity)/* * d*/ * irradiance * irradianceStrength;
            break;
        }

        // Update deltaStepIncrement
        RAYMARCH_STEP_SIZE = mix(NEAR_RAYMARCH_STEP_SIZE, FAR_RAYMARCH_STEP_SIZE, smoothstep(sampleSmoothEdgeNearFar.x, sampleSmoothEdgeNearFar.y, t0 + distanceTraveled));
        deltaStepIncrement = deltaPositionNormalized * RAYMARCH_STEP_SIZE;

        // Advance march
        currentPosition += deltaStepIncrement;
        distanceTraveled += RAYMARCH_STEP_SIZE;
    }

    // @DEBUG: See how many times a cloud's textures were calculated
    // fragmentColor = vec4(vec3(float(count)), 1.0);
    // return;

    fragmentColor =
        vec4(
            mix(
                atmosValues.rgb * (1.0 - transmittance),  // @ATMOS: combine atmospheric scattering
                lightColor * lightEnergy,
                atmosValues.a
            ) + ambientLightEnergy,       // NOTE: when there's atmosphericScattering at night, it seems to make the clouds look completely black. Maybe the transmittance isn't allowing any of the clouds to see. @THEREFORE: we made the ambient light term not get affected by the atmosphere values
            transmittance
        );
}