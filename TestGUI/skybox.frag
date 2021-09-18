#version 430
out vec4 fragColor;

in vec3 texCoord;

uniform samplerCube skyboxTex;

void main()
{
	vec3 envColor = texture(skyboxTex, texCoord).rgb;
    fragColor = vec4(envColor, 1.0);
}