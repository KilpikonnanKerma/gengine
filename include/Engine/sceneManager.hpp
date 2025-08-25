#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Engine/object.hpp"
#include "Engine/light.hpp"

#include <GL/gl.h>

#include <cfloat>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;

struct GizmoAxis {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
    glm::vec3 dir;
};

class SceneManager {
public:
    std::vector<Object*> objects;
    std::vector<Light> lights;

    Object* selectedObject = nullptr;
    
    GizmoAxis grabbedAxis;
    bool axisGrabbed = false;
    float axisGrabDistance = 0.1f;
    GLuint axisVAO, axisVBO;

    int objCounter = 0;

    void clearScene();

    void initGizmo();
    void drawGizmo(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
    bool pickGizmoAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir, GizmoAxis& outAxis);
    void dragSelectedObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir);

    Object* addObject(const std::string& type, const std::string& name);
    void removeObject(Object* objPtr);

    void addLight(const Light& light);

    void update(float deltaTime);
    void render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
    Object* pickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir);

    void initGrid(int gridSize = 20, float spacing = 1.0f);
    void drawGrid(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);

    void saveScene(const std::string& path);
    void loadScene(const std::string& path);

private:
    GLuint gridVAO = 0, gridVBO = 0;
    int gridVertexCount = 0;
};

#endif
