#version 430
out vec4 fragColor;

in vec3 localPos;

uniform vec3 mainCameraPosition;
uniform vec3 sunOrientation;
uniform float sunRadius;
uniform float sunAlpha;

uniform samplerCube nightSkybox;
uniform mat3 nightSkyTransform;

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

vec3 atmosphere(vec3 r, vec3 r0, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g)
{
    // Normalize the sun and view directions.
    r = normalize(r);
    pSun = normalize(pSun);

    // Calculate the step size of the primary ray.
    vec2 p = rsi(r0, r, rAtmos);    
    vec2 o = rsi(r0, r, rPlanet);
    float rjthegod = texture(depthTexture, gl_FragPosition * invResolution);      // @TODO: start from here to grab the scene depth. Also make sure that the depth test is off.

    // @NOTE: Perhaps we need to do the slicing based rendering method. every frame compute depth and step size (rg32 texture) onto a texture (32x32?) and then iterate thru the max depth. Logorithmically??
    
    // @NOTE: do the distance you wanna traverse and add onto previous slice (step size? or maybe you need to recalculate with the same 16 step size?)



    p.y = min(p.y, o.x);
    p.y = min(p.y, rjthegod);

    float iStepSize = (p.y - p.x) / float(iSteps);

    // Initialize the primary ray time.
    float iTime = 0.0;

    // Initialize accumulators for Rayleigh and Mie scattering.
    vec3 totalRlh = vec3(0,0,0);
    vec3 totalMie = vec3(0,0,0);

    // Initialize optical depth accumulators for the primary ray.
    float iOdRlh = 0.0;
    float iOdMie = 0.0;

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
        float iHeight = length(iPos) - rPlanet;

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
        vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

        // Accumulate scattering.
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;

        // Increment the primary ray time.
        iTime += iStepSize;
    }

    // Calculate and return the final color.
    return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}


void main()
{
    vec3 ray_world = normalize(localPos);           // TIMO: Does this work??? I guess so...
	
	vec3 out_LightScattering = vec3(0);             // @TODO: see if this is important. Perhaps this is for volumetric lighting?????? We'll see I guess
	
    // TIMO
    vec3 v_Sun = -sunOrientation;// * -2600.0;

    vec3 out_Color = atmosphere(
        ray_world,        	            // normalized ray direction
        mainCameraPosition + vec3(0, 6376e2, 0),            	// ray origin
        v_Sun,                  		// position of the sun
        22.0,                           // intensity of the sun
        6365e2,                         // radius of the planet in meters
        6465e2,                         // radius of the atmosphere in meters
        vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
        21e-6,                          // Mie scattering coefficient
        8e3,                            // Rayleigh scale height
        1.2e3,                          // Mie scale height
        0.758                           // Mie preferred scattering direction
    );
	
	// Apply exposure.
    out_Color = 1.0 - exp(-1.0 * out_Color);

    out_Color += texture(nightSkybox, nightSkyTransform * ray_world).rgb * 0.005;       // TIMO
	
	float _sunRadius = length(normalize(ray_world) - normalize(v_Sun));
	
	// no sun rendering when scene reflection
	if(_sunRadius < sunRadius)
	{
		_sunRadius /= sunRadius;
		float smoothRadius = smoothstep(0,1,0.1f/_sunRadius-0.1f);
		out_Color = mix(out_Color, sunBaseColor * 4, smoothRadius * sunAlpha);
		
		smoothRadius = smoothstep(0,1,0.18f/_sunRadius-0.2f);
		out_LightScattering = mix(vec3(0), sunBaseColor, smoothRadius);
	}

    fragColor = vec4(out_Color, 1);
}
