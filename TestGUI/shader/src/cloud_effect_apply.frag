#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D skyboxTexture;
uniform sampler2D skyboxDetailTexture;
//uniform sampler2D cloudEffect;


void main()
{
	vec3 skyboxColor = texture(skyboxTexture, texCoord).rgb + texture(skyboxDetailTexture, texCoord).rgb;
	//vec4 cloudEffectColor = texture(cloudEffect, texCoord).rgba;

	vec3 color = skyboxColor; // * cloudEffectColor.a + cloudEffectColor.rgb;
	fragmentColor = vec4(color, 1.0);
}
