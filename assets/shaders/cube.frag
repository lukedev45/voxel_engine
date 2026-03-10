#version 330 core
in float blockType;
out vec4 FragColor;

void main() {
    vec3 colour;

    if (blockType < 1.5)
        colour = vec3(0.5, 0.5, 0.5);
    else if (blockType < 2.5)
        colour = vec3(0.6, 0.4, 0.2);
    else
        colour = vec3(0.2, 0.7, 0.2);

    FragColor = vec4(colour, 1.0); // light blue cube
}
