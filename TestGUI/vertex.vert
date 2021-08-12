#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec2 uvCoordinate;

out vec2 texCoord;

uniform mat4 cameraMatrix;

void main()
{
	gl_Position = cameraMatrix * vec4(vertexPosition, 1.0);
	texCoord = uvCoordinate;
}