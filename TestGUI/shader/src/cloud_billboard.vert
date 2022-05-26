#version 430

layout (location = 0) in vec4 vertexPosition;
layout (location = 1) in vec2 aTexCoords;
out vec2 texCoords;

uniform mat4 modelMatrix;

// Camera
layout (std140, binding = 3) uniform CameraInformation
{
    mat4 cameraProjection;
	mat4 cameraView;
	mat4 cameraProjectionView;
};

void main()
{
    gl_Position = cameraProjectionView * modelMatrix * vec4(vertexPosition.xy, 0.0, 1.0);
    texCoords = aTexCoords;
}
