#version 430

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D hdrColorBuffer;

void main()
{
	vec3 hdrColor = texture(hdrColorBuffer, texCoord).rgb;
	
	//// HDR tonemapping          TODO: Implement hdr and then have there be tonemapping at the end and bloom!
    //hdrColor = hdrColor / (hdrColor + vec3(1.0));
    //// gamma correct
    //hdrColor = pow(hdrColor, vec3(1.0/2.2));

	// UE4 Tonemapper
	hdrColor = hdrColor / (hdrColor + 0.155) * 1.019;

	fragColor = vec4(hdrColor, 1.0);
}
