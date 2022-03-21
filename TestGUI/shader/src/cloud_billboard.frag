#version 430

in vec2 texCoords;
out vec4 fragmentColor;

uniform sampler2D posVolumeTexture;
uniform sampler2D negVolumeTexture;
uniform vec3 mainLightDirectionVS;      // @NOTE: this means that the direction is in view space
uniform vec3 mainLightColor;

void main()
{
	vec4 posVolume = texture(posVolumeTexture, texCoords);
	vec4 negVolume = texture(negVolumeTexture, texCoords);

	float intensity = 0;
	if (mainLightDirectionVS.x < 0)
		intensity += negVolume.x * -mainLightDirectionVS.x;
	else
		intensity += posVolume.x * mainLightDirectionVS.x;

	if (mainLightDirectionVS.y < 0)
		intensity += negVolume.y * -mainLightDirectionVS.y;
	else
		intensity += posVolume.y * mainLightDirectionVS.y;

	if (mainLightDirectionVS.z < 0)
		intensity += negVolume.z * -mainLightDirectionVS.z;
	else
		intensity += posVolume.z * mainLightDirectionVS.z;

	fragmentColor = vec4(mainLightColor * intensity, posVolume.a);
}
