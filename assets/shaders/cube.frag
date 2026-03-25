#version 330 core
in vec2 TexCoord;
in float face;
in float ao;
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

    // Ambient occlusion: ao is 0-3, map to brightness multiplier
    // 3 = no occlusion (1.0), 0 = fully occluded (0.45)
    float aoFactor = 0.45 + (ao / 3.0) * 0.55;

    FragColor = vec4(texColour.rgb * brightness * aoFactor, 1.0);
}
