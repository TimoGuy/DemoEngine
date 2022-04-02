#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform float currentRenderDepth;
uniform sampler3D sample1;
uniform sampler3D sample2;
uniform sampler3D sample3;


void main()
{
	float noise =
		0.5333333   * texture(sample1, vec3(texCoord, currentRenderDepth)).r
		+ 0.2666667 * texture(sample2, vec3(texCoord, currentRenderDepth)).r
		+ 0.2666667 * texture(sample3, vec3(texCoord, currentRenderDepth)).r;
	fragmentColor = vec4(vec3(noise), 1.0);
}
