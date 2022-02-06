#version 430

in vec2 texCoord;

uniform sampler2D ubauTexture;


void main()
{
	// A life spent making mistakes is not only more honorable,
	// but more useful than a life spent doing nothing.
	//		- George Bernard Show
	//
	//
	// A fragment shader for a depth map's sole purpose in life...
	// is to do nothing. And it's extremely useful.
	//		- Timothy Bennett



	// AND IN A WILD TURN OF EVENTS, THE DO_NOTHING.FRAG SHADER NOW DOES SOMETHING!!!
	float textureAlpha = texture(ubauTexture, texCoord).a;
	if (textureAlpha < 0.5)
		discard;
}