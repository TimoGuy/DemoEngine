#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec2 uvCoordinate;

out vec2 texCoord;

// Camera
layout (std140, binding = 3) uniform CameraInformation
{
    mat4 cameraProjection;
	mat4 cameraView;
	mat4 cameraProjectionView;
};

uniform mat4 modelMatrix;

void main()
{
	texCoord = (mat3(inverse(modelMatrix)) * vec3(modelMatrix * vec4(vertexPosition, 1.0))).xy;
	gl_Position = cameraProjectionView * modelMatrix * vec4(vertexPosition, 1.0);
}