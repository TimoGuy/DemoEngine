#version 430
out vec4 fragColor;

in vec3 localPos;

uniform vec3 sunOrientation;
uniform float sunRadius;

uniform float horizonFresnelLength;
uniform vec3 sunColor;
uniform vec3 skyColor1;
uniform vec3 skyColor2;
uniform vec3 groundColor;

uniform float sunIntensity;
uniform float globalExposure;
uniform float rimLightfresnelPower;

uniform float cloudHeight;
uniform float perlinDim;
uniform float perlinTime;


//
// Perlin noise https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
//
#define M_PI 3.14159265358979323846

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }
float rand (vec2 co, float l) { return rand(vec2(rand(co), l)); }
float rand (vec2 co, float l, float t) { return rand(vec2(rand(co, l), t)); }

float perlin(vec2 p, float dim, float time)
{
	vec2 pos = floor(p * dim);
	vec2 posx = pos + vec2(1.0, 0.0);
	vec2 posy = pos + vec2(0.0, 1.0);
	vec2 posxy = pos + vec2(1.0);
	
	float c = rand(pos, dim, time);
	float cx = rand(posx, dim, time);
	float cy = rand(posy, dim, time);
	float cxy = rand(posxy, dim, time);
	
	vec2 d = fract(p * dim);
	d = -0.5 * cos(d * M_PI) + 0.5;
	
	float ccx = mix(c, cx, d.x);
	float cycxy = mix(cy, cxy, d.x);
	float center = mix(ccx, cycxy, d.y);
	
	return center * 2.0 - 1.0;
}

//
// Let's get funcy
//
void main()
{
    vec3 v = normalize(localPos);

    //
    // Short circuit if the sun orientation is in the way lol
    //
    float lengthFromSunPos = length(v - -sunOrientation);
    if (step(sunRadius, lengthFromSunPos) == 0)
    {
        fragColor = vec4(sunColor, 1) * sunIntensity * globalExposure;
        return;
    }

    vec3 color = vec3(1, 0, 1);

    // Base: color based off blue params
    color = mix(skyColor1, skyColor2, v.y);

    // Add on: sky rim
    //float skyRim = horizonFresnelLength - ;
    //if (skyRim > 0)
    {
        float addonAmount = 1 / pow(v.y * horizonFresnelLength + 1, rimLightfresnelPower);
        color += vec3(addonAmount) * 2;
    }

    // Add on: cloud level
    if (v.y <= 0)
    {
        // Calculate what the cloud height would be
        float mult = cloudHeight / v.y;
        vec2 samplePos = vec2(v.x, v.z) * mult;

        float perlin = perlin(samplePos, perlinDim, 0);
        perlin = perlin + 1;

        color = mix(groundColor, vec3(1, 1, 1), perlin);
    }

    // Add on: global exposure
    color *= globalExposure;

    fragColor = vec4(color, 1.0);
}