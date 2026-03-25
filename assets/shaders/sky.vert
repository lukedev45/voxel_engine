#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 invViewProj;

out vec3 viewDir;

void main() {
    gl_Position = vec4(aPos, 1.0, 1.0); // z=1 puts it at the far plane
    // Reconstruct world-space view direction from clip-space position
    vec4 worldPos = invViewProj * vec4(aPos, 1.0, 1.0);
    viewDir = worldPos.xyz / worldPos.w;
}
