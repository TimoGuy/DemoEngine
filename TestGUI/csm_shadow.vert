#version 430

layout (location=0) in vec3 vertexPosition;

uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;


void main()
{
	vec3 fragPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));
	gl_Position = lightSpaceMatrix * vec4(fragPosition, 1.0);
}
