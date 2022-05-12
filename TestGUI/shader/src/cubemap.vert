#version 430

layout (location = 0) in vec3 vertexPosition;

uniform mat4 projection;
uniform mat4 view;

out vec3 localPos;

void main()
{
    localPos = vertexPosition;

    mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    vec4 clipPos = projection * rotView * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
}
