#version 430

out float FragColor;

uniform uint objectID;

void main()
{
    FragColor = float(objectID);
}
