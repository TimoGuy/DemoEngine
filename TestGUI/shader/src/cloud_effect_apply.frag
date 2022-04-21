#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D skyboxTexture;
uniform sampler2D cloudEffect;
uniform sampler2D cloudAmbientIrradiance;
uniform float darknessThreshold;


void main()
{
	vec4 cloudEffectColor = texture(cloudEffect, texCoord).rgba;
	vec4 cloudAmbientColor = texture(cloudAmbientIrradiance, texCoord).rgba * darknessThreshold * (1.0 - cloudEffectColor.a);

	vec3 color = texture(skyboxTexture, texCoord).rgb * cloudEffectColor.a + cloudEffectColor.rgb + cloudAmbientColor.rgb;
	fragmentColor = vec4(color, 1.0);
}
