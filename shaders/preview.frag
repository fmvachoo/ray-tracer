#version 430 core

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 u_color;
uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform float u_roughness;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(u_lightDir);
    vec3 V = normalize(u_viewPos - vFragPos);
    vec3 H = normalize(L + V);

    // Ambient
    vec3 ambient = 0.15 * u_color;

    // Diffuse (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * u_color;

    // Specular (Blinn-Phong, roughness-based)
    float shininess = mix(256.0, 4.0, u_roughness);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = vec3(spec * (1.0 - u_roughness) * 0.5);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}