#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;
uniform sampler2D bloomColorBuffer;

uniform float bloomIntensity;

void main()
{
	vec3 hdrColor = texture(hdrColorBuffer, texCoord).rgb + texture(bloomColorBuffer, texCoord).rgb * bloomIntensity;
	
	//// HDR tonemapping          TODO: Implement hdr and then have there be tonemapping at the end and bloom!
    //hdrColor = hdrColor / (hdrColor + vec3(1.0));
    //// gamma correct
    //hdrColor = pow(hdrColor, vec3(1.0 / 2.2));

	float brightness = dot(hdrColor.rgb, vec3(0.2126, 0.7152, 0.0722));

	// UE4 Tonemapper
	hdrColor = hdrColor / (hdrColor + 0.155) * 1.019;

	fragColor = vec4(hdrColor, 1.0);

	float brightness2 = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	/*if (brightness2 > 1.0)
	{
		fragColor = vec4(0, 1, 0, 1);
	}
	else if (brightness > 1.0)
	{
		fragColor = vec4(1, 0, 0, 1);
	}*/
}
