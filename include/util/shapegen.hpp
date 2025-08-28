#ifndef SHAPEGEN_H
#define SHAPEGEN_H

#include <vector>
#include <GL/gl.h>
#include <GL/glext.h>

#include <stb_image.h>

#include <iostream>

#include "nsm/math.hpp"

using namespace NMATH;

struct Vertex {
    Vec3d pos;
    Vec3d color;
    Vec3d normal;
    Vec2d texCoord;
};

class ShapeGenerator {

public:
    static void createCube(float size, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
    static void createPlane(float width, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
    static void createPyramid(float size, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
    static void createSphere(float radius, int segments, int rings, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);

    unsigned int loadTexture(const char* path);
};

#endif