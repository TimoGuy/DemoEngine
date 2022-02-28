#version 430

layout (location=0) in vec3 aPosition;
layout (location=1) in vec2 aTexCoords;

out vec2 texCoord;
out vec2 fragLocalPosition;

uniform float padding;
uniform vec2 extents;


uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

void main()
{
	gl_Position = viewMatrix * modelMatrix * vec4(aPosition, 1.0);
	fragLocalPosition = aPosition.xy * (extents + vec2(padding));
	texCoord = aTexCoords;
}
