#version 430

out vec4 FragColor;

uniform vec4 color;
uniform float colorIntensity;

void main()
{
    vec3 totalColor = color.rgb * colorIntensity;
    FragColor = vec4(totalColor, color.a);
}
