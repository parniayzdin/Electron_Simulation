#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
out vec3 pointColor;
out float worldX;
uniform mat4 view;
uniform mat4 projection;
uniform float viewportHeight;
uniform float dotRadius;
uniform float maxPointSize;
void main() {
    vec4 eye = view * vec4(position, 1.0);
    gl_Position = projection * eye;
    gl_PointSize = clamp(dotRadius * viewportHeight * projection[1][1] /
        max(-eye.z, 0.001), 1.5, maxPointSize);
    pointColor = color;
    worldX = position.x;
}
