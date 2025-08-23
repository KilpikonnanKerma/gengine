#ifndef SHAPEGEN_H
#define SHAPEGEN_H

#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
};

class ShapeGenerator {

public:
    static void createCube(float size, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
    static void createPlane(float width, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
    static void createPyramid(float size, float height, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
    static void createSphere(float radius, int segments, int rings, std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
};

#endif