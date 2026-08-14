#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position =
        projection * //creates 3D perspective
        view * //represents the camera
        model * //movs, rotates or resizes an object
        vec4(position, 1.0);

    vertexColor = color;
}