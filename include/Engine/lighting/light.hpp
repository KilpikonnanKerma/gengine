#ifndef LIGHT_H
#define LIGHT_H

#include "math/math.hpp"

using namespace NMATH;

enum class LightType { Directional, Point };

class Light {
public:
    LightType type;
    Vec3d position;
    Vec3d direction;
    Vec3d color;
    float intensity;

    Light(LightType t, Vec3d col, float inten)
        : type(t), color(col), intensity(inten) {
        position = Vec3d(0.0f);
        direction = Vec3d(0.0f, -1.0f, 0.0f);
    };
};

#endif
