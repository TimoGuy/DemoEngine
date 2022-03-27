#version 430

in vec2 texCoords;
out vec4 fragmentColor;

uniform float currentRenderDepth;
uniform int numChannels;
uniform ivec4 pointsPerChannel;
uniform vec3 channel1Points[100];
uniform vec3 channel2Points[100];
uniform vec3 channel3Points[100];
uniform vec3 channel4Points[100];

void main()
{
	fragmentColor = vec4(0.0);
	vec3 myPos = vec3(texCoords, currentRenderDepth);

	//
	// R
	//
	if (numChannels < 1)
		return;

	float closestDistance = 1.0;
	for (int i = 0; i < pointsPerChannel.r; i++)
		closestDistance = min(closestDistance, distance(myPos, channel1Points[i]));
	fragmentColor.r = closestDistance;

	//
	// G
	//
	if (numChannels < 2)
		return;

	closestDistance = 1.0;
	for (int i = 0; i < pointsPerChannel.g; i++)
		closestDistance = min(closestDistance, distance(myPos, channel2Points[i]));
	fragmentColor.g = closestDistance;
	
	//
	// B
	//
	if (numChannels < 3)
		return;

	closestDistance = 1.0;
	for (int i = 0; i < pointsPerChannel.b; i++)
		closestDistance = min(closestDistance, distance(myPos, channel3Points[i]));
	fragmentColor.b = closestDistance;
	
	//
	// A
	//
	if (numChannels < 4)
		return;

	closestDistance = 1.0;
	for (int i = 0; i < pointsPerChannel.a; i++)
		closestDistance = min(closestDistance, distance(myPos, channel4Points[i]));
	fragmentColor.a = closestDistance;
}
