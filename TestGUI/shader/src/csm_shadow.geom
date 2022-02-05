#version 450

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

in vec2 texCoordToGeom[];
out vec2 texCoord;

layout (std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};

void main()
{
    // Draw triangles in the different matrices
    for (int i = 0; i < 3; i++)
    {
        gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        texCoord = texCoordToGeom[0];
        EmitVertex();
    }
    EndPrimitive();
}
