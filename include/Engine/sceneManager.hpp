#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Engine/object.hpp"
#include "Engine/light.hpp"

#include <string>
#include <vector>

class SceneManager {
public:
    std::vector<Object*> objects;
    std::vector<Light> lights;

    void addObject(Object& obj);
    void addLight(const Light& light);

    void update(float deltaTime);
    void render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
};

#endif
