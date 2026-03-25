#version 330 core
in vec3 viewDir;
out vec4 FragColor;

uniform vec3 cameraPos;

void main() {
    vec3 dir = normalize(viewDir - cameraPos);
    float t = dir.y; // -1 (straight down) to +1 (straight up)

    // Sky gradient
    vec3 zenith  = vec3(0.25, 0.45, 0.85); // deep blue at top
    vec3 horizon = vec3(0.65, 0.80, 0.95); // pale blue at horizon
    vec3 ground  = vec3(0.35, 0.35, 0.38); // dark gray below horizon

    vec3 color;
    if (t > 0.0) {
        // Above horizon: blend from horizon to zenith
        float s = pow(t, 0.6); // softer curve for a wider bright horizon band
        color = mix(horizon, zenith, s);
    } else {
        // Below horizon: quick fade to ground color
        float s = pow(min(-t * 3.0, 1.0), 0.5);
        color = mix(horizon, ground, s);
    }

    FragColor = vec4(color, 1.0);
}
