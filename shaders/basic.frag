#version 330 core
struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

#define MAX_DIR_LIGHTS 4
#define MAX_POINT_LIGHTS 4

uniform DirLight dirLights[MAX_DIR_LIGHTS];
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numDirLights;
uniform int numPointLights;

in vec3 FragPos;
in vec3 Color;
in vec3 Normal;
out vec4 FragColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0);

    // Directional lights
    for(int i=0;i<numDirLights;i++){
        vec3 lightDir = normalize(-dirLights[i].direction);
        float diff = max(dot(norm, lightDir),0.0);
        result += (0.1 + diff * dirLights[i].intensity) * dirLights[i].color * Color;
    }

    // Point lights
    for(int i=0;i<numPointLights;i++){
        vec3 lightDir = normalize(pointLights[i].position - FragPos);
        float diff = max(dot(norm, lightDir),0.0);
        float distance = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (distance*distance);
        result += (0.1 + diff * pointLights[i].intensity * attenuation) * pointLights[i].color * Color;
    }

    FragColor = vec4(result,1.0);
}
