#version 430

layout (location=0) in vec3 aPosition;
layout (location=1) in vec2 aTexCoords;

out vec2 texCoord;
out vec2 fragLocalPosition;

uniform float padding;
uniform vec2 staminaBarExtents;


uniform mat4 viewMatrix;  // @REFACTOR: put these in a ubo
uniform mat4 modelMatrix;  // @REFACTOR: put these in a ubo

void main()
{
	gl_Position = viewMatrix * modelMatrix * vec4(aPosition, 1.0);
	fragLocalPosition = aPosition.xy * (staminaBarExtents + vec2(padding));
	texCoord = aTexCoords;
}