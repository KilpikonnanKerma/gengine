#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include "glad/glad.h"
#include "util/shapegen.hpp"

#include "nsm/math.hpp"
using namespace NMATH;

class Object {
public:
    Vec3d position;
    Vec3d rotation;             // Euler angles
    std::string type;
    Vec3d scale;
    std::string name;

    unsigned int VAO, VBO, EBO;
    unsigned int textureID;
    std::string texturePath;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    Object();
    void initCube(float size);
    void initPlane(float width, float height);
    void initSphere(float radius, int segments, int rings);
    void initPyramid(float size, float height);

    void setupMesh();
    Mat4 getModelMatrix() const;

    void texture(const std::string& path);
    void draw() const;
    float boundingRadius() const;
};

#endif
