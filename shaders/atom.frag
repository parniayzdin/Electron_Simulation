#version 330 core
in vec3 pointColor;
in float worldX;
out vec4 finalColor;
uniform float opacity;
uniform float brightness;
uniform int cutaway;
uniform float cutPosition;
void main() {
    if (cutaway != 0 && worldX > cutPosition) discard;
    vec2 local = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(local, local);
    if (r2 > 1.0) discard;
    float coverage = 1.0 - smoothstep(0.15, 1.0, r2);
    finalColor = vec4(pointColor * brightness, opacity * coverage * brightness);
}
