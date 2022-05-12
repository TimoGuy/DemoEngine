#version 430

out vec4 fragColor;
in vec3 localPos;

uniform float mapInterpolationAmt;
uniform mat3 sunSpinAmount;
uniform samplerCube texture1;
uniform samplerCube texture2;


void main()
{
    vec3 N = sunSpinAmount * normalize(localPos);
    fragColor = mix(texture(texture1, N).rgba, texture(texture2, N).rgba, mapInterpolationAmt);
}
