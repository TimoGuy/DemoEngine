#version 430
out vec4 fragColor;

in vec3 localPos;

uniform vec3 mainCameraPosition;
uniform vec3 sunOrientation;

uniform bool renderNight;
uniform samplerCube nightSkybox;
uniform mat3 nightSkyTransform;

// FOR DEPTH-SLICED ATMOSPHERE //
uniform float depthZFar;
/////////////////////////////////

// UNUSED //
uniform vec3 sunColor;
uniform vec3 skyColor1;
uniform vec3 groundColor;

uniform float sunIntensity;
uniform float globalExposure;

uniform float cloudHeight;
uniform float perlinDim;
uniform float perlinTime;
////////////

/////////////////////////////////////////////////////////////////////////////////////////////


// @SLOW: @NOTE: I noticed that when the camera is more facing the ground, the lower the fps. Perhaps the ground area is harder to sample for???
// IDK, maybe look into it once you got better time.



//https://github.com/fynnfluegge/oreon-engine/blob/master/oreonengine/oe-gl-components/src/main/resources/shaders/atmosphere/atmospheric_scattering.frag
// Which apparently was plaigiarized from:
//      https://github.com/wwwtyro/glsl-atmosphere/blob/master/index.glsl
//          It's licensed under the Unlicense btw.
//              Rye Terrell
//                  wwwtyro






precision highp float;

uniform mat4 projection;
uniform mat4 view;
//uniform vec3 v_Sun;       NOTE: use sunOrientation and sunRadius
//uniform float r_Sun;
//uniform int width;            NOTE: became unused bc used the ndc from the vertex shader instead haha
//uniform int height;
//uniform int isReflection;     NOTE: use showSun
//uniform float horizonVerticalShift;
//uniform float reflectionVerticalShift;
const vec3 sunBaseColor = vec3(1.0f,0.79f,0.43f);

#define PI 3.1415926535897932384626433832795
#define iSteps 16
#define jSteps 8


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


vec4 atmosphereDefineDepth(vec3 r, vec3 r0, float iStepSize, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g)
{
    // Initialize the primary ray time.
    float iTime = 0.0;

    // Initialize accumulators for Rayleigh and Mie scattering.
    vec3 totalRlh = vec3(0,0,0);
    vec3 totalMie = vec3(0,0,0);

    // Initialize optical depth accumulators for the primary ray.
    float iOdRlh = 0.0;
    float iOdMie = 0.0;

    // Initialize transmittance
    vec3 transmittance = vec3(1.0);
    const float scaledIStepSize = iStepSize / (rAtmos - rPlanet);

    // Calculate the Rayleigh and Mie phases.
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    // Sample the primary ray.
    for (int i = 0; i < iSteps; i++)
    {
        // Calculate the primary ray sample position.
        vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);

        // Calculate the height of the sample.
        float iPosLength = length(iPos);
        if (iPosLength > rAtmos)
            continue;       // @NOTE: when outside the atmosphere's sphere, this number gets bonkers. Best just to cancel any calculations if this'll happen here.

        float iHeight = iPosLength - rPlanet;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
        float odStepMie = exp(-iHeight / shMie) * iStepSize;

        // Accumulate optical depth.
        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        // Calculate the step size of the secondary ray.
        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

        // Initialize the secondary ray time.
        float jTime = 0.0;

        // Initialize optical depth accumulators for the secondary ray.
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        // Sample the secondary ray.
        for (int j = 0; j < jSteps; j++)
        {
            // Calculate the secondary ray sample position.
            vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

            // Calculate the height of the sample.
            float jHeight = length(jPos) - rPlanet;

            // Accumulate the optical depth.
            jOdRlh += exp(-jHeight / shRlh) * jStepSize;
            jOdMie += exp(-jHeight / shMie) * jStepSize;

            // Increment the secondary ray time.
            jTime += jStepSize;
        }

        // Calculate attenuation.
        const vec3 extinction = -(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh));
        vec3 attn = exp(extinction);
        transmittance *= exp(extinction * scaledIStepSize);

        // Accumulate scattering.
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;

        // Increment the primary ray time.
        iTime += iStepSize;
    }

    // Calculate and return the final color.
    vec3 finalColor = iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
    const vec3 luma = vec3(0.299, 0.587, 0.114);

    // Calculate and return the transmittance.
    //
    // @NOTE: this probably isn't mathematically/physically correct, however due
    // to all sources of local lights (point lights etc.) getting cut out bc of
    // this transmittance equation going to 0 when it's nighttime (makes sense
    // bc transmittance should be how much light from the 'sun' (atmosphere) is
    // getting to the view pixel), I decided that if the luma of the sky pixel is
    // gonna be 0, then the transmittance (this time meaning from your eye to the
    // point where the pixel is on the screen) algorithm should change to reflect
    // the light that *can possibly transmit from the pixel on screen into your eye*
    // ... Which I know is technically incorrect, however hey, it fixed the problem
    // so what I can I say. If you wanna argue with me, email me dawg.  -Timo
    float transmittanceLuma = dot(transmittance, luma);  // @NOTE: takes the average of the transmittance (has some tradeoffs according to https://sebh.github.io/publications/egsr2020.pdf)
    float finalColorLuma = dot(finalColor, luma);
    return vec4(finalColor, max(1.0 - 3.0 * finalColorLuma, transmittanceLuma));
}


vec3 atmosphere(vec3 r, vec3 r0, vec2 isectPlanet, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g)
{
    // Normalize the sun and view directions.
    r = normalize(r);
    pSun = normalize(pSun);

    // Calculate the step size of the primary ray.
    vec2 p = rsi(r0, r, rAtmos);
    p.y = min(p.y, isectPlanet.x);

    // Get r0 to match up with the atmosphere's edge
    r0 = r0 + r * p.x;
	p.y -= p.x;
	p.x = 0.0;

    float iStepSize = (p.y - p.x) / float(iSteps);
    return atmosphereDefineDepth(r, r0, iStepSize, pSun, iSun, rPlanet, rAtmos, kRlh, kMie, shRlh, shMie, g).rgb;
}


void main()
{
    vec3 ray_world = normalize(localPos);           // Does this work??? I guess so...  -Timo

    // TIMO
    const vec3 r0 = mainCameraPosition + vec3(0, 6376e2, 0);
    const float rPlanet = 6351e2;  // 6361e2;  // 6365e2;
    const float rAtmos  = 6465e2;
    vec3 v_Sun = -sunOrientation;

    //
    // If normal skybox render
    //
    if (depthZFar < 0.0)
    {
        vec2 isectPlanet = rsi(r0, ray_world, rPlanet);

        vec3 out_Color = atmosphere(
            ray_world,        	            // normalized ray direction
            r0,            	                // ray origin
            isectPlanet,                    // Intersection with the planet
            v_Sun,                  		// position of the sun
            22.0,                           // intensity of the sun
            rPlanet,                        // radius of the planet in meters
            rAtmos,                         // radius of the atmosphere in meters
            vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
            21e-6,                          // Mie scattering coefficient
            8e3,                            // Rayleigh scale height
            1.2e3,                          // Mie scale height
            0.758                           // Mie preferred scattering direction
        );
	
	    // Apply exposure
        out_Color = 1.0 - exp(-out_Color);

        if (renderNight)
        {
            // Apply the night sky texture too (reflect off the planet's ground sphere)  -Timo
            vec3 nightSkyboxSampleRay = ray_world;
            if (isectPlanet.x < isectPlanet.y)
            {
                vec3 collisionPoint = r0 + ray_world * isectPlanet.x;
                nightSkyboxSampleRay = reflect(nightSkyboxSampleRay, normalize(collisionPoint));        // @NOTE: sphere.CENTER is 0,0,0 in this case!
            }
            out_Color += texture(nightSkybox, nightSkyTransform * nightSkyboxSampleRay).rgb * 0.005;
        }

        fragColor = vec4(out_Color, 1);
    }
    //
    // If sliced depth rendering
    //
    else
    {
        vec4 out_Color = atmosphereDefineDepth(
            ray_world,        	            // normalized ray direction
            r0,            	                // ray origin
            depthZFar / float(iSteps),      // step size (this * iSteps should be the total distance of the ray)
            v_Sun,                  		// position of the sun
            22.0,                           // intensity of the sun
            rPlanet,                        // radius of the planet in meters
            rAtmos,                         // radius of the atmosphere in meters
            vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
            21e-6,                          // Mie scattering coefficient
            8e3,                            // Rayleigh scale height
            1.2e3,                          // Mie scale height
            0.758                           // Mie preferred scattering direction
        );

        // Apply exposure (only on the color channels, not the transmittance channel)
        out_Color.rgb = 1.0 - exp(-out_Color.rgb);

        // @NOTE: Only exposure is applied since the night sky and the rendered sun
        // are not physically based, bc they don't change depending on the depth slice
        fragColor = out_Color;
    }
}
