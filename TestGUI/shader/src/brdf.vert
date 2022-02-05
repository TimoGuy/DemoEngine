#version 430

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 texCoords;

out vec2 texCoord;

void main()
{
    texCoord = texCoords;
	gl_Position = vec4(vertexPosition, 1.0);
}
