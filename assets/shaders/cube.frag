#version 330 core
in vec2 TexCoord;
in float face;
out vec4 FragColor;

uniform sampler2D texAtlas;

void main() {
    vec4 texColour = texture(texAtlas, TexCoord);

    // Face brightness
    float brightness;
    int faceIndex = int(face + 0.5);
    if (faceIndex == 5)      brightness = 1.0;  // top
    else if (faceIndex == 4) brightness = 0.5;  // bottom
    else if (faceIndex == 0) brightness = 0.8;  // front
    else if (faceIndex == 1) brightness = 0.7;  // back
    else                     brightness = 0.75; // left/right

    FragColor = vec4(texColour.rgb * brightness, 1.0);
}
