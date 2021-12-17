#version 430
out vec4 fragColor;

in vec3 localPos;

uniform vec3 sunOrientation;
uniform float sunRadius;

uniform vec3 sunColor;
uniform vec3 skyColor1;
uniform vec3 groundColor;

uniform float sunIntensity;
uniform float globalExposure;

uniform float cloudHeight;
uniform float perlinDim;
uniform float perlinTime;

uniform bool showSun;


//
// Perlin noise https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
//
/*#define M_PI 3.14159265358979323846

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
}*/

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
    if (showSun && step(sunRadius, lengthFromSunPos) == 0 && v.y > 0)
    {
        fragColor = vec4(sunColor, 1) * sunIntensity * globalExposure;
        return;
    }

    vec3 color = vec3(1, 0, 1);

    // Base: color based off blue params
    {
        // Complex funk that took me a bunch of time to figger out ahhh
        color = skyColor1 * (vec3(pow(1 - clamp(v.y, 0, 1), 1 / clamp(-sunOrientation.y, 0, 1))) * 0.95 + 0.05);
    }

    // Add on: mie scattering (sunset/rise)
    if (v.y > 0)
    {
        vec3 stretchedView1 = normalize(vec3(v.x, v.y * 10, v.z));
        float vDotSun1 = clamp(1 - pow(1 - dot(-sunOrientation, stretchedView1), 0.2), 0, 1);
        
        vec3 stretchedView2 = normalize(vec3(v.x, v.y * 20, v.z));
        float vDotSun2 = clamp(1 - pow(1 - dot(-sunOrientation, stretchedView2), 0.2), 0, 1);
        
        float influence = pow(1 - clamp(-sunOrientation.y, 0, 1), 2.5);
        
        float value1 = vDotSun1 * influence;
        float value2 = vDotSun2 * influence;

        color += vec3(.9 * value1, .6 * value2, 0);
    }

    // Turn down exposure if (neg)sunOrientation.y is below 0
    {
        float amount = 1 - clamp(sunOrientation.y, 0.0, 0.125) * (1 / 0.125);
        color *= amount;
    }

    // Ground underneath
    if (v.y < 0)
        color = groundColor;

    // Add on: global exposure
    color *= globalExposure;

    fragColor = vec4(color, 1.0);
}