#version 430

layout (location=0) in vec4 vertexPosition;
out vec2 texCoords;

uniform mat4 modelMatrix;
uniform mat4 cameraMatrix;

void main()
{
    gl_Position = cameraMatrix * modelMatrix * vec4(vertexPosition.xy, 0.0, 1.0);
    texCoords = vertexPosition.zw;
}
