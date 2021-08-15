#version 430

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uvCoordinate;

out vec2 texCoord;
out vec3 fragPosition;
out vec3 normalVector;

uniform mat4 modelMatrix;
uniform mat3 normalsModelMatrix;
uniform mat4 cameraMatrix;

void main()
{
	fragPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));
	normalVector = normalsModelMatrix * normal;
	//fragPosition = vertexPosition;
	//normalVector = normal;

	gl_Position = cameraMatrix * vec4(fragPosition, 1.0);
	texCoord = uvCoordinate;
}