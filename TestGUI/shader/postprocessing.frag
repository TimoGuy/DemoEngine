#version 430

out vec4 fragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D hdrColorBuffer;
layout(binding = 1) uniform sampler2D bloomColorBuffer;
layout(binding = 2) uniform sampler2D luminanceProcessed;
//layout(binding = 3) uniform sampler2D ssaoTexture;		@Deprecate
layout(binding = 3) uniform sampler2D volumetricLighting;

//uniform float ssaoScale;		@Deprecate
//uniform float ssaoBias;

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
	vec3 hdrColor = texture(hdrColorBuffer, texCoord).rgb + sunLightColor * texture(volumetricLighting, texCoord).r;

	//float ssao = clamp(texture(ssaoTexture, texCoord).r + ssaoBias, 0.0, 1.0);		@Deprecate
	//hdrColor = mix(hdrColor, hdrColor * ssao, ssaoScale);

	hdrColor += texture(bloomColorBuffer, texCoord).rgb * bloomIntensity;
	float avgLuminance = texture(luminanceProcessed, vec2(0.5, 0.5)).r;

	hdrColor *= exposure * vignette(texCoord, 0.5, 0.75) * 0.5 / (avgLuminance + 0.001);
	hdrColor = hdrColor / (hdrColor + 0.155) * 1.019;				// UE4 Tonemapper

	fragColor = vec4(hdrColor, 1.0);
}
