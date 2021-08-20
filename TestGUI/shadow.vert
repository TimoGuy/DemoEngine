#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec2 uvCoordinate;

uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;


void main()
{
	gl_Position = lightSpaceMatrix * vec4(vertexPosition, 1.0);
}
