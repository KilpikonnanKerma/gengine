#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>
#include <vector>
#include "glad/glad.h"
#include "util/shapegen.hpp"

class Object {
public:
    glm::vec3 position;
    glm::vec3 rotation;             // Euler angles
    std::string type;
    glm::vec3 scale;
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
    glm::mat4 getModelMatrix() const;

    void texture(const std::string& path);
    void draw() const;
    float boundingRadius() const;
};

#endif
