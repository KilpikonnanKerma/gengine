#ifndef BILLBOARD_HPP
#define BILLBOARD_HPP

#include "math/math.hpp"

#include <glad/glad.h>

using namespace NMATH;
class Billboard {
public:
    static void DrawBillboard(
        const Vec3d& start,
        const Vec3d& end,
        float thickness,
        const Vec3d& color,
        GLuint shader,
        const Mat4& model,
        const Mat4& view,
        const Mat4& projection
    );
};

#endif