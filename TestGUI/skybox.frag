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
    float skyRim = horizonFresnelLength - v.y;
    if (skyRim > 0)
    {
        float addonAmount = pow(skyRim / horizonFresnelLength, rimLightfresnelPower);
        color += vec3(addonAmount) * sunIntensity;
    }

    // Add on: global exposure
    color *= globalExposure;

    // ground level
    if (v.y < 0)
        color = groundColor;

    fragColor = vec4(color, 1.0);
}