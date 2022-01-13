#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;
uniform vec2 uvStretchAmount;

//----------------------------------------------------------------
// https://github.com/TyLindberg/glsl-vignette/blob/master/simple.glsl
float vignette(vec2 uv, float radius, float smoothness) {
	float diff = radius - distance(uv, vec2(0.5, 0.5));
	return smoothstep(-smoothness, smoothness, diff);
}
//----------------------------------------------------------------
void main()
{
	float luminance =
		dot(vec3(0.2125, 0.7154, 0.0721),
			texture(hdrColorBuffer, texCoord * uvStretchAmount).rgb) *			// NOTE: this is cheap and dirty. It's like a hoe.
		vignette(texCoord, 0.5, 0.75);

	fragColor = vec4(vec3(luminance), 1.0);
}
