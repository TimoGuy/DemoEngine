#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D skyboxTexture;
uniform sampler2D cloudEffect;


void main()
{
	vec4 cloudEffectColor = texture(cloudEffect, texCoord).rgba;
	const float cloudAmount = 1.0;

	vec3 color = texture(skyboxTexture, texCoord).rgb * (cloudAmount * cloudEffectColor.a + (1.0 - cloudAmount)) + cloudEffectColor.rgb * cloudAmount;
	fragmentColor = vec4(color, 1.0);
}
