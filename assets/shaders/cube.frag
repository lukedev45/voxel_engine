#version 330 core
in float blockType;
in float face;
out vec4 FragColor;

void main() {
    vec3 colour;
    if (blockType < 1.5)
        colour = vec3(0.5, 0.5, 0.5);
    else if (blockType < 2.5)
        colour = vec3(0.6, 0.4, 0.2);
    else
        colour = vec3(0.2, 0.7, 0.2);

    // Face brightness
    float brightness;
    int faceIndex = int(face + 0.5);
    if (faceIndex == 5)      brightness = 1.0;  // top
    else if (faceIndex == 4) brightness = 0.5;  // bottom
    else if (faceIndex == 0) brightness = 0.8;  // front
    else if (faceIndex == 1) brightness = 0.7;  // back
    else                     brightness = 0.75; // left/right    if (faceIndex == 5)     

    FragColor = vec4(colour * brightness, 1.0); // light blue cube
}
