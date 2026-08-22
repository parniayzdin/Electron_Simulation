#version 330 core

in vec3 vertexColor;

out vec4 finalColor;

uniform float brightness;

void main()
{
    //A small brightness boost gives electrons a more luminous appearance.
    finalColor = vec4(min(vertexColor * brightness, vec3(1.0)), 1.0);
}
