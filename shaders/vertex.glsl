#version 120

attribute vec3 aPos;
attribute vec3 aColor;
attribute vec3 aNormal;
attribute vec2 aTexCoord;

varying vec3 FragPos;
varying vec3 FragPosWorld;
varying vec3 Color;
varying vec3 Normal;
varying vec3 NormalWorld;
varying vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // world-space position
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPosWorld = worldPos.xyz;
    FragPos = FragPosWorld;

    Color = aColor;

    // Transform normal to world space (assumes no non-uniform scale)
    NormalWorld = normalize(mat3(model) * aNormal);
    Normal = NormalWorld;

    TexCoord = aTexCoord;
    gl_Position = projection * view * worldPos;
}