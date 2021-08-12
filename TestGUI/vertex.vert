#version 430

in vec3 vertexPosition;

uniform mat4 cameraMatrix;

void main()
{
	gl_Position = cameraMatrix * vec4(vertexPosition, 1.0);
}