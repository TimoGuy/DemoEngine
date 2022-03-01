#version 430

out vec4 fragColor;

in vec2 texCoord;
in vec2 fragLocalPosition;

uniform vec2 extents;
uniform vec3 color1;
uniform vec3 color2;
uniform float fadeAlpha;

void main()
{
	vec3 color = color1;

	vec2 absLocalPos = abs(fragLocalPosition.xy);
	if (absLocalPos.x > extents.x || absLocalPos.y > extents.y)
	{
		color = color2;		// Border color
	}

	fragColor = vec4(color, fadeAlpha);
}
