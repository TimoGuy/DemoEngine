#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;
uniform sampler2D bloomColorBuffer;

uniform float bloomIntensity;

void main()
{
	vec3 hdrColor = texture(hdrColorBuffer, texCoord).rgb + texture(bloomColorBuffer, texCoord).rgb * bloomIntensity;

	hdrColor = hdrColor / (hdrColor + 0.155) * 1.019;				// UE4 Tonemapper

	fragColor = vec4(hdrColor, 1.0);
}
