#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

enum class LightType { Directional, Point };

class Light {
public:
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;

    Light(LightType t, glm::vec3 col, float inten)
        : type(t), color(col), intensity(inten) {
        position = glm::vec3(0.0f);
        direction = glm::vec3(0.0f, -1.0f, 0.0f);
    };
};

#endif
