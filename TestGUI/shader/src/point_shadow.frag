#version 430

in vec2 texCoord;
in vec4 fragPosition;

uniform sampler2D ubauTexture;
uniform vec3 lightPosition;
uniform float farPlane;

void main()
{
    // See if should discard fragment
    float textureAlpha = texture(ubauTexture, texCoord).a;
	if (textureAlpha < 0.5)
    {
        discard;
    }

	// get distance between fragment and light source
    float lightDistance = length(fragPosition.xyz - lightPosition);
    
    // map to [0;1] range by dividing by farPlane
    lightDistance = lightDistance / farPlane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}
