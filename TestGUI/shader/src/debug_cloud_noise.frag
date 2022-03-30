#version 430

out vec4 FragColor;

in vec2 TexCoords;

uniform float layer;
uniform sampler2DArray noiseMap;


void main()
{
    vec3 thingo = texture(noiseMap, vec3(TexCoords, layer)).rrr;
    FragColor = vec4(thingo, 1.0);
}
