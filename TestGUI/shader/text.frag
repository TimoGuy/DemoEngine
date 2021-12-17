#version 430

in vec2 texCoords;
out vec4 fragmentColor;

uniform sampler2D textTexture;
uniform vec3 diffuseTint;

void main()
{
    float alpha = texture(textTexture, texCoords).r;
    if (alpha < 0.5)
        discard;
    fragmentColor = vec4(diffuseTint, 1.0) * vec4(1.0, 1.0, 1.0, alpha);
}
