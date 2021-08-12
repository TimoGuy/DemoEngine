#version 430

out vec4 fragmentColor;

in vec2 texCoord;

uniform sampler2D tex0;

void main()
{
	//fragmentColor = vec4(texCoord, 0.0f, 1.0f);
	fragmentColor = texture(tex0, texCoord);
	//fragmentColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
}