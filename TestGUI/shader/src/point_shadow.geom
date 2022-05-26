#version 430

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

in vec2 texCoordToGeom[];
out vec2 texCoord;

uniform mat4 shadowMatrices[6];

out vec4 fragPosition; // fragPosition from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            fragPosition = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * fragPosition;
            texCoord = texCoordToGeom[0];
            EmitVertex();
        }
        EndPrimitive();
    }
}
