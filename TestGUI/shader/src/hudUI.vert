#version 430

layout (location=0) in vec3 aPosition;
layout (location=2) in vec2 aTexCoords;

out vec2 texCoord;
out vec2 fragLocalPosition;

uniform float padding;
uniform vec2 staminaBarExtents;

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
	gl_Position = cameraProjectionView * modelMatrix * vec4(aPosition, 1.0);
	fragLocalPosition = aPosition.xy * (staminaBarExtents + vec2(padding));
	texCoord = aTexCoords;
}
