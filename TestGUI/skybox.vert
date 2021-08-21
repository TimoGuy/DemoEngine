#version 430

layout (location=0) in vec3 vertexPosition;

out vec3 texCoord;

uniform mat4 skyboxProjViewMatrix;

void main()
{
	gl_Position = skyboxProjViewMatrix * vec4(vertexPosition, 1.0);
	texCoord = vertexPosition;
}