#version 430

out vec4 fragColor;

in vec2 texCoord;
in vec2 fragLocalPosition;

uniform vec3 color1;
uniform vec3 color2;
uniform float fadeAlpha;

void main()
{
	fragColor =
		vec4(
			mix(color1, color2, texCoord.x),
			fadeAlpha
		);
}
