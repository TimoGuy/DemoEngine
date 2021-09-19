#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;
uniform sampler2D smallerReconstructHDRColorBuffer;			// Reserved for only reconstruction

uniform int stage;					// Stage1: copy over image. Stage2: horizontal blur. Stage3: vertical blur.
uniform bool firstcopy;				// Only happens during an image copy.

uniform float downscaledFactor;		// How downscaled the render texture is compared to native res.

uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);		// Gaussian blur 5-point kernel

void main()
{
	vec2 moddedTexCoord = texCoord * downscaledFactor;

	//
	// Stage 1
	//
	if (stage == 1)
	{
		// TODO: this needs some downscaling filters
		vec3 colorCopy = texture(hdrColorBuffer, moddedTexCoord).rgb;
		
		if (firstcopy)
		{
			colorCopy -= vec3(1.0);
			colorCopy = max(vec3(0.0), colorCopy);
		}
		
		fragColor = vec4(colorCopy, 1.0);
	}

	//
	// Stage 2
	//
	else if (stage == 2)
	{
		vec2 texOffset = 1.0 / textureSize(hdrColorBuffer, 0); // gets size of single texel
		vec3 result = texture(hdrColorBuffer, moddedTexCoord).rgb * weight[0]; // current fragment's contribution

		for (int i = 1; i < 5; i++)
        {
            result += texture(hdrColorBuffer, moddedTexCoord + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(hdrColorBuffer, moddedTexCoord - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        }
		
		fragColor = vec4(result, 1.0);
	}

	//
	// Stage 3
	//
	else if (stage == 3)
	{
		vec2 texOffset = 1.0 / textureSize(hdrColorBuffer, 0); // gets size of single texel
		vec3 result = texture(hdrColorBuffer, moddedTexCoord).rgb * weight[0]; // current fragment's contribution

		for (int i = 1; i < 5; i++)
        {
            result += texture(hdrColorBuffer, moddedTexCoord + vec2(0.0, texOffset.y * i)).rgb * weight[i];
            result += texture(hdrColorBuffer, moddedTexCoord - vec2(0.0, texOffset.y * i)).rgb * weight[i];
        }

		fragColor = vec4(result, 1.0);
	}

	//
	// Stage 4
	//
	else if (stage == 4)
	{
		vec3 colorBigger = texture(hdrColorBuffer, moddedTexCoord).rgb;
		vec3 colorSmaller = texture(smallerReconstructHDRColorBuffer, moddedTexCoord).rgb;
		fragColor = vec4(colorBigger + colorSmaller, 1.0);
	}
}
