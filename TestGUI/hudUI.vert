#version 430

layout (location=0) in vec3 aPosition;
layout (location=1) in vec2 aTexCoords;

out vec2 texCoord;

uniform float referenceScreenHeight;
uniform float padding;
uniform float staminaBarWidth;
uniform float staminaBarHeight;
uniform float staminaAmountFilled;
uniform mat4 viewMatrix1;
uniform mat4 modelMatrix1;

void main()
{
	gl_Position = viewMatrix1 * modelMatrix1 * vec4(aPosition, 1.0);
	texCoord = aTexCoords;
}