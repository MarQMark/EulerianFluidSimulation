#version 330 core
const float PI = 3.14159265359;

in vec3 Direction;

out vec4 FragColor;

uniform sampler2D skybox;

vec2 projectLongLat(vec3 direction) {
    float theta = atan(direction.x, -direction.z) + PI;
    float phi = acos(-direction.y);
    return vec2(theta / (2.0 * PI), phi / PI);
}

void main() {
    vec4 skyColor = vec4(1.0f);
    skyColor *= textureLod(skybox, projectLongLat(normalize(Direction)), 0.0);
    FragColor = skyColor;
}