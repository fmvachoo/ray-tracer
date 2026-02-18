#version 430 core

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);  // направление К свету
    vec3 V = normalize(uViewPos - vFragPos);
    vec3 H = normalize(L + V);

    // Ambient
    vec3 ambient = 0.15 * uColor;

    // Diffuse (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * uColor * uLightColor;

    // Specular (Blinn-Phong)
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.3;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}