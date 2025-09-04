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

const int MAX_DIR_SHADOWS = 4;
uniform sampler2D shadowMap[MAX_DIR_SHADOWS];
uniform mat4 lightSpaceMatrix[MAX_DIR_SHADOWS];
uniform float uShadowMapSize; // e.g. 2048.0
uniform float uShadowCullHeight[MAX_DIR_SHADOWS]; // added: per-shadow cull height

varying vec3 FragPosWorld;
varying vec3 NormalWorld;

float sampleShadow(int idx, vec3 normal, vec4 worldPosLightSpace) {
    // Cull shadow sampling for fragments above configured height for this shadow (e.g. very high objects)
    if (idx >= 0 && idx < MAX_DIR_SHADOWS) {
        if (FragPosWorld.y > uShadowCullHeight[idx]) {
            return 0.0;
        }
    }

    vec3 proj = worldPosLightSpace.xyz / worldPosLightSpace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 0.0;
    float currentDepth = proj.z;
    float bias = max(0.005 * (1.0 - dot(normalize(normal), vec3(0,0,1))), 0.0005);
    float shadow = 0.0;
    float texel = 1.0 / uShadowMapSize;
    for (int x=-1; x<=1; ++x) {
        for (int y=-1; y<=1; ++y) {
            vec2 offs = vec2(float(x), float(y)) * texel;
            float depth = texture2D(shadowMap[idx], proj.xy + offs).r;
            if (currentDepth - bias > depth) shadow += 1.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

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
    for(int i = 0; i < MAX_DIR_LIGHTS; ++i) {
        if (i >= uNumDirLights) break;
        vec3 lightDir = normalize(-dirLightDirs[i]);
        float diff = max(dot(norm, lightDir), 0.0);
        float intensity = (0.1 + diff * dirLightIntensities[i]);
        float sh = 0.0;
        if (i < MAX_DIR_SHADOWS) {
            vec4 fragLS = lightSpaceMatrix[i] * vec4(FragPosWorld, 1.0);
            sh = sampleShadow(i, NormalWorld, fragLS);
        }
        result += (1.0 - sh) * intensity * dirLightColors[i] * baseColor;
    }

    // Point lights (no shadow sampling here)
    for(int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (i >= uNumPointLights) break;
        vec3 lightDir = normalize(pointLightPositions[i] - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        float distance = length(pointLightPositions[i] - FragPos);
        float attenuation = 1.0 / (distance * distance);
        result += (0.1 + diff * pointLightIntensities[i] * attenuation) * pointLightColors[i] * baseColor;
    }

    gl_FragColor = vec4(result, 1.0);
}