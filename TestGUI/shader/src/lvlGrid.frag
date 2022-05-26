#version 430

out vec4 FragColor;
in vec2 texCoord;

// material parameters
uniform vec3 gridColor;
uniform vec4 tilingAndOffset;       // NOTE: x, y are tiling, and z, w are offset
uniform sampler2D gridTexture;

void main()
{
    vec2 adjustedTexCoord = texCoord * tilingAndOffset.xy + tilingAndOffset.zw;

    float mipmapLevel       = textureQueryLod(gridTexture, adjustedTexCoord).x;
    vec4 grid              = textureLod(gridTexture, adjustedTexCoord, mipmapLevel).rgba;
    if (grid.a < 0.5)
        discard;

    FragColor = vec4(grid.rgb * gridColor, grid.a);
}
