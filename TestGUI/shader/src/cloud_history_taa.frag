#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D cloudEffectBuffer;
uniform sampler2D cloudEffectHistoryBuffer;


void main()
{
	vec4 cloudEffectColor = texture(cloudEffectBuffer, texCoord).rgba;				// GL_NEAREST
	vec4 cloudEffectHistory = texture(cloudEffectHistoryBuffer, texCoord).rgba;		// GL_LINEAR
	
	const float modulationFactor = 0.9;
	//const float modulationFactor = 0.0;
	fragmentColor = mix(cloudEffectColor, cloudEffectHistory, modulationFactor);
}
