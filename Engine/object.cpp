#include "Engine/object.h"
#include <glm/gtc/matrix_transform.hpp>

Object::Object() {
    position = glm::vec3(0.0f);
    rotation = glm::vec3(0.0f);
    scale    = glm::vec3(1.0f);
}

void Object::initCube(float size) {
    ShapeGenerator::createCube(size, vertices, indices);
    setupMesh();
}

void Object::initPlane(float width, float height) {
    ShapeGenerator::createPlane(width, height, vertices, indices);
    setupMesh();
}

void Object::initSphere(float radius, int segments, int rings) {
    ShapeGenerator::createSphere(radius, segments, rings, vertices, indices);
    setupMesh();
}

void Object::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
}

glm::mat4 Object::getModelMatrix() {
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1,0,0));
    model = glm::rotate(model, rotation.y, glm::vec3(0,1,0));
    model = glm::rotate(model, rotation.z, glm::vec3(0,0,1));
    model = glm::scale(model, scale);
    return model;
}
