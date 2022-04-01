#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform float currentRenderDepth;
uniform sampler3D R;
uniform sampler3D G;
uniform sampler3D B;
uniform sampler3D A;


void main()
{
	fragmentColor =
		vec4(
			texture(R, vec3(texCoord, currentRenderDepth)).r,
			texture(G, vec3(texCoord, currentRenderDepth)).r,
			texture(B, vec3(texCoord, currentRenderDepth)).r,
			texture(A, vec3(texCoord, currentRenderDepth)).r
		);
}
