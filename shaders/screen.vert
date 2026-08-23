#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 textureCoordinateIn;

out vec2 textureCoordinate;

void main()
{
    //This quad already fills the screen, so it does not need a 3D camera.
    gl_Position = vec4(position, 0.0, 1.0);
    textureCoordinate = textureCoordinateIn;
}
