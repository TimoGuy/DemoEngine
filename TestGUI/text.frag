#version 430

in vec2 texCoords;
out vec4 fragmentColor;

uniform sampler2D textTexture;
uniform vec3 diffuseTint;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(textTexture, texCoords).r);
    fragmentColor = vec4(diffuseTint, 1.0) * sampled;
}
