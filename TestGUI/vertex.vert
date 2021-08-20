#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec2 uvCoordinate;

out vec2 texCoord;
out vec4 fragPositionLightSpace;

uniform mat4 cameraMatrix;
uniform mat4 lightSpaceMatrix;

void main()
{
	fragPositionLightSpace = lightSpaceMatrix * vec4(vertexPosition, 1.0);
	gl_Position = cameraMatrix * vec4(vertexPosition, 1.0);
	texCoord = uvCoordinate;
}