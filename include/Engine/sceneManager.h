#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Engine/object.h"
#include "Engine/light.h"

#include <string>
#include <vector>

class SceneManager {
public:
    std::vector<Object> objects;
    std::vector<Light> lights;

    void addObject(const Object& obj);
    void addLight(const Light& light);

    void update(float deltaTime);
    void render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
};

#endif
