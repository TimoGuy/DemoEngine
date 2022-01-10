#version 450

in vec2 texCoord;

uniform sampler2D ubauTexture;
uniform float ditherAlpha;


float ditherTransparency(float transparency)   // https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Dither-Node.html
{
    vec2 uv = gl_FragCoord.xy;
    float DITHER_THRESHOLDS[16] =
    {
        1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
    };
    uint index = (uint(uv.x) % 4) * 4 + uint(uv.y) % 4;
    return transparency - DITHER_THRESHOLDS[index];
}

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
	if (ditherTransparency(ditherAlpha * 2.0) < 0.5)
        discard;

	float textureAlpha = texture(ubauTexture, texCoord).a;
	if (textureAlpha < 0.5)
		discard;
}