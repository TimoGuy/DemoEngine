#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform float currentRenderDepth;
uniform vec3 worleyPoints[512];
uniform int numPoints;

void main()
{
	fragmentColor = vec4(0.0);
	vec3 myPos = vec3(texCoord, currentRenderDepth);
	float closestDistance = 1.0;
	for (int i = 0; i < numPoints; i++)
		for (int x = -1; x <= 1; x++)
			for (int y = -1; y <= 1; y++)
				for (int z = -1; z <= 1; z++)
					closestDistance = min(closestDistance, distance(myPos, worleyPoints[i] + vec3(x, y, z)));
	fragmentColor = vec4(vec3(closestDistance * 7), 1.0);
}
