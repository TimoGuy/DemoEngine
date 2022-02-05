#version 430

out vec4 FragColor;

uniform vec4 wireframeColor;
uniform float colorIntensity;

void main()
{
    vec3 totalColor = wireframeColor.rgb * colorIntensity;
    FragColor = vec4(totalColor, wireframeColor.a);
}
