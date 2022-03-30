#version 430

in vec2 texCoord;
out vec4 fragmentColor;

uniform int renderArrayIndex;
uniform sampler2DArray R;
uniform sampler2DArray G;
uniform sampler2DArray B;
uniform sampler2DArray A;


void main()
{
	fragmentColor =
		vec4(
			texture(R, vec3(texCoord, renderArrayIndex)).r,
			texture(G, vec3(texCoord, renderArrayIndex)).r,
			texture(B, vec3(texCoord, renderArrayIndex)).r,
			texture(A, vec3(texCoord, renderArrayIndex)).r
		);
}
