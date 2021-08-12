#version 430

out vec4 fragColor;

in vec3 texCoord;

uniform samplerCube skyboxTex;

void main()
{
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	fragColor = texture(skyboxTex, texCoord);
}