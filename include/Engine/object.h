#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>
#include <vector>
#include "glad/glad.h"
#include "Util/shapegen.h"

class Object {
public:
    glm::vec3 position;
    glm::vec3 rotation;             // Euler angles
    glm::vec3 scale;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO, VBO, EBO;

    Object();
    void initCube(float size);
    void initPlane(float width, float height);
    void initSphere(float radius, int segments, int rings);

    void setupMesh();
    glm::mat4 getModelMatrix();
};

#endif
