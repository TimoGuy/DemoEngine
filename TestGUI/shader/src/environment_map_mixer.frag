#version 430

out vec4 fragColor;
in vec3 localPos;

uniform float mapInterpolationAmt;
uniform mat3 sunSpinAmount;
uniform samplerCube texture1;
uniform samplerCube texture2;
uniform float lod;


void main()
{
    vec3 N = sunSpinAmount * normalize(localPos);
    fragColor = mix(textureLod(texture1, N, lod).rgba, textureLod(texture2, N, lod).rgba, mapInterpolationAmt);
}
