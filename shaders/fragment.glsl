#version 120

varying vec3 FragPos;
varying vec3 Color;
varying vec3 Normal;
varying vec2 TexCoord;

uniform sampler2D uTexture;
uniform bool useTexture;
uniform vec3 overrideColor;
uniform int useOverrideColor;

const int MAX_DIR_LIGHTS = 4;
const int MAX_POINT_LIGHTS = 4;

uniform int uNumDirLights;
uniform vec3 dirLightDirs[MAX_DIR_LIGHTS];
uniform vec3 dirLightColors[MAX_DIR_LIGHTS];
uniform float dirLightIntensities[MAX_DIR_LIGHTS];

uniform int uNumPointLights;
uniform vec3 pointLightPositions[MAX_POINT_LIGHTS];
uniform vec3 pointLightColors[MAX_POINT_LIGHTS];
uniform float pointLightIntensities[MAX_POINT_LIGHTS];


void main() {
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0);

    // Get base color (from texture OR from vertex color)
    vec3 baseColor = Color;
    if (useTexture) {
        baseColor = texture2D(uTexture, TexCoord).rgb; 
    }

    // If an override color is requested, use it as the base color (ignores textures and vertex colors)
    if (useOverrideColor == 1) {
        baseColor = overrideColor;
    }

    // Directional lights
    for(int i=0;i< MAX_DIR_LIGHTS;i++){
		if(i>=uNumDirLights) break;
        vec3 lightDir = normalize(-dirLightDirs[i]);
        float diff = max(dot(norm, lightDir),0.0);
        result += (0.1 + diff * dirLightIntensities[i]) * dirLightColors[i] * baseColor;
    }

    // Point lights
    for(int i=0;i<MAX_POINT_LIGHTS;i++){
		if(i>=uNumPointLights) break;
        vec3 lightDir = normalize(pointLightPositions[i] - FragPos);
        float diff = max(dot(norm, lightDir),0.0);
        float distance = length(pointLightPositions[i] - FragPos);
        float attenuation = 1.0 / (distance*distance);
        result += (0.1 + diff * pointLightIntensities[i] * attenuation) * pointLightColors[i] * baseColor;
    }

    gl_FragColor = vec4(result,1.0);
}
