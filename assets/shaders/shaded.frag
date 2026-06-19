#version 330 core

in vec3 Normal;
in vec2 TexCoord;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 albedo = vec3(1.0, 1.0, 1.0);
uniform float ambientStrength;
uniform float specularStrength;
uniform float fresnelStrength;

uniform vec3 lightDir = vec3(-0.8, -1.0, -0.6);
uniform vec3 viewPos;

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(-lightDir);
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);

    // Diffuse
    float diffuse = max(dot(N, L), 0.0);

    // Specular
    float spec = pow(max(dot(N, H), 0.0), 128.0f);

    // Fresnel
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    // Mix resulting color
    vec3 color = albedo * (ambientStrength + diffuse);
    color += vec3(1.0) * spec * specularStrength;
    color += albedo * fresnel * fresnelStrength;

    FragColor = vec4(color, 1.0);
}
