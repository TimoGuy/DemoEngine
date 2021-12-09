#version 430

out vec3 FragColor;

uniform uint objectID;

void main()
{
    FragColor = vec3(25.0, 1, 1);//float(objectID);
}
