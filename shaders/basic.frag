#version 330 core

in vec3 vertexColor;

out vec4 finalColor;

void main()
{
    finalColor = vec4(vertexColor, 1.0);
}