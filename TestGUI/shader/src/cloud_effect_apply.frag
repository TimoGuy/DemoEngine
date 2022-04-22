#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D skyboxTexture;
uniform sampler2D cloudEffect;


void main()
{
	vec4 cloudEffectColor = texture(cloudEffect, texCoord).rgba;

	vec3 color = texture(skyboxTexture, texCoord).rgb * cloudEffectColor.a + cloudEffectColor.rgb;
	fragmentColor = vec4(color, 1.0);
}
