#version 430 core

uniform vec3 uColor;
uniform float uIntensity;

out vec4 FragColor;

void main()
{
    // Показываем яркий цвет, зажатый в [0,1]
    vec3 c = uColor * min(uIntensity, 5.0) / 5.0;
    c = clamp(c * 0.5 + 0.5, 0.0, 1.0); // делаем светлым, чтобы было видно
    FragColor = vec4(c, 0.8); // полупрозрачный
}