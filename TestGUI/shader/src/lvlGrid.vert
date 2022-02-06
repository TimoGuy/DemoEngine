#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec2 uvCoordinate;

out vec2 texCoord;

uniform mat4 modelMatrix;  // @REFACTOR: put these in a ubo
uniform mat4 cameraMatrix;  // @REFACTOR: put these in a ubo

void main()
{
	texCoord = uvCoordinate;
	gl_Position = cameraMatrix * modelMatrix * vec4(vertexPosition, 1.0);
}