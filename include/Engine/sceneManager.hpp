#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Engine/object.hpp"
#include "Engine/light.hpp"

#include "glad/glad.h"

#include <cfloat>
#include <string>
#include <vector>

#include "nsm/math.hpp"

#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;
using namespace NMATH;

struct GizmoAxis {
    Vec3d start;
    Vec3d end;
    Vec3d color;
    Vec3d dir;

	GizmoAxis(const Vec3d& s, const Vec3d& e, const Vec3d& c, const Vec3d& d)
		: start(s), end(e), color(c), dir(d) {}

    // State captured at pick time to make dragging stable
    float initialProj = 0.0f;
    Vec3d initialObjPos = Vec3d(0.0f);
};

class SceneManager {
public:
    std::vector<Object*> objects;
    std::vector<Light> lights;

    Object* selectedObject = nullptr;
    
    GizmoAxis grabbedAxis;
    bool axisGrabbed = false;
    bool objectDrag = false;
    Vec3d dragPlaneNormal = Vec3d(0.0f);
    Vec3d dragInitialPoint = Vec3d(0.0f);
    Vec3d dragInitialObjPos = Vec3d(0.0f);
    float axisGrabDistance = 0.12f;

    GLuint axisVAO, axisVBO;
    GLuint lightVAO = 0, lightVBO = 0;
    int selectedLightIndex = -1;

    int objCounter = 0;

    void clearScene();

    void initGizmo();
    void initLightGizmo();
    void drawGizmo(GLuint shaderProgram, const Mat4& view, const Mat4& projection);
    bool pickGizmoAxis(const Vec3d& rayOrigin, const Vec3d& rayDir, GizmoAxis& outAxis);
    bool pickLight(const Vec3d& rayOrigin, const Vec3d& rayDir, int& outIndex, float radius = 0.5f);
    void dragSelectedObject(const Vec3d& rayOrigin, const Vec3d& rayDir);

    Object* addObject(const std::string& type, const std::string& name);
    void removeObject(Object* objPtr);
    void removeLight(int index);
    void addLight(const Light& light);

    void update(float deltaTime);
    void render(GLuint shaderProgram, const Mat4& view, const Mat4& projection);
    Object* pickObject(const Vec3d& rayOrigin, const Vec3d& rayDir);

    void initGrid(int gridSize = 20, float spacing = 1.0f);
    void drawGrid(GLuint shaderProgram, const Mat4& view, const Mat4& projection);

    void saveScene(const std::string& path);
    void loadScene(const std::string& path);

private:
    GLuint gridVAO = 0, gridVBO = 0;
    int gridVertexCount = 0;
};

#endif
