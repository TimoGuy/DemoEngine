#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;
uniform sampler2D volumetricLighting;
uniform sampler2D cloudEffect;

uniform vec3 sunLightColor;

//----------------------------------------------------------------
// https://github.com/TyLindberg/glsl-vignette/blob/master/simple.glsl
float vignette(vec2 uv, float radius, float smoothness) {
	float diff = radius - distance(uv, vec2(0.5, 0.5));
	return smoothstep(-smoothness, smoothness, diff);
}
//----------------------------------------------------------------
void main()
{
	vec4 cloudEffectColor = texture(cloudEffect, texCoord).rgba;

	const float cloudAmount = 0.5;
	float luminance =
		dot(vec3(0.2125, 0.7154, 0.0721),
			texture(hdrColorBuffer, texCoord).rgb * (cloudAmount * cloudEffectColor.a + (1.0 - cloudAmount)) + cloudEffectColor.rgb * cloudAmount +	// @HACK: Multiplying the cloudEffectColor.rgb by 0.5 bc I'm a hoe  -Timo
			sunLightColor * texture(volumetricLighting, texCoord).r) *			// NOTE: this is cheap and dirty. It's like a hoe.
		vignette(texCoord, 0.5, 0.75);

	fragColor = vec4(vec3(luminance), 1.0);
}
