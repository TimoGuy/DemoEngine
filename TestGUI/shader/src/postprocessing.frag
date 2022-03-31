#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;
uniform sampler2D bloomColorBuffer;
uniform sampler2D luminanceProcessed;
uniform sampler2D volumetricLighting;
uniform sampler2D cloudEffect;

uniform vec3 sunLightColor;

uniform float exposure;
uniform float bloomIntensity;

//----------------------------------------------------------------
// https://github.com/TyLindberg/glsl-vignette/blob/master/simple.glsl
float vignette(vec2 uv, float radius, float smoothness) {
	float diff = radius - distance(uv, vec2(0.5, 0.5));
	return smoothstep(-smoothness, smoothness, diff);
}
//----------------------------------------------------------------
void main()
{
	//fragColor = vec4(texture(hdrColorBuffer, texCoord).rgb, 1.0);
	//return;
	//
	//////////////////////////////////////////////////////////////////////////////////////////////

	vec3 hdrColor = texture(hdrColorBuffer, texCoord).rgb + sunLightColor * texture(volumetricLighting, texCoord).r;

	hdrColor += texture(bloomColorBuffer, texCoord).rgb * bloomIntensity;
	float avgLuminance = texture(luminanceProcessed, vec2(0.5, 0.5)).r;

	hdrColor *= exposure * vignette(texCoord, 0.5, 0.75) * 0.5 / (avgLuminance + 0.001);
	hdrColor = hdrColor / (hdrColor + 0.155) * 1.019;				// UE4 Tonemapper

	vec4 cloudEffectColor = texture(cloudEffect, texCoord).rgba;		// @TODO: @FIXME: @HACK: this is super duper one-off and should get fixed asap!!!!! (And make a more temporal solution in the future yo...)
	hdrColor = hdrColor * cloudEffectColor.a + cloudEffectColor.rgb;

	fragColor = vec4(hdrColor, 1.0);
}
