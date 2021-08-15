#version 430

out vec4 fragmentColor;

in vec2 texCoord;
in vec3 fragPosition;
in vec3 normalVector;

uniform sampler2D texture_diffuse1;		// NOTE: depending on the number of textures, you may need more samplers

uniform vec3 lightPosition;
uniform vec3 viewPosition;
uniform vec3 lightColor;

void main()
{
	//vec4 objectColor = texture(texture_diffuse1, texCoord);
	vec4 objectColor = vec4(0.3, 0.3, 0.3, 1.0);

	//
	// Ambient light
	//
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	//
	// Diffuse Light
	//
	vec3 norm = normalize(normalVector);
	vec3 lightDir = normalize(lightPosition - fragPosition);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	//
	// Specular Light
	//
	float specularStrength = 0.5;
	vec3 viewDir = normalize(viewPosition - fragPosition);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	fragmentColor = vec4((ambient + diffuse + specular) * objectColor.rgb, objectColor.a);
}