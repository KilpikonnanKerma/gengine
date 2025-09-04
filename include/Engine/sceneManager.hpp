#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP

#include "Engine/objects/object.hpp"
#include "Engine/objects/light.hpp"
#include "Engine/gizmos/transformTool.hpp"

#include "glad/glad.h"
#include "nlohmann/json.hpp"

#include <cfloat>
#include <string>
#include <vector>

#include "math/math.hpp"

#include <fstream>

using json = nlohmann::json;
using namespace NMATH;

struct GizmoAxis {
    Vec3d start;
    Vec3d end;
    Vec3d color;
    Vec3d dir;

    // Default constructor so SceneManager can be default-constructed
    GizmoAxis()
        : start(Vec3d(0.0)), end(Vec3d(0.0)), color(Vec3d(0.0)), dir(Vec3d(0.0)) {}

	GizmoAxis(const Vec3d& s, const Vec3d& e, const Vec3d& c, const Vec3d& d)
		: start(s), end(e), color(c), dir(d) {}

    // State captured at pick time to make dragging stable
    float initialProj = 0.0f;
    Vec3d initialObjPos = Vec3d(0.0f);
};

class SceneManager {
public:
    SceneManager();
    std::vector<Object*> objects;
    std::vector<Light> lights;

    Object* selectedObject;
    
    GizmoAxis grabbedAxis;
    bool axisGrabbed;
    int grabbedAxisIndex; // -1 if none, 0=X, 1=Y, 2=Z
    bool objectDrag;
    Vec3d dragPlaneNormal;
    Vec3d dragInitialPoint;
    Vec3d dragInitialObjPos;
    float axisGrabDistance;

    // Width in pixels for gizmo axis lines
    float gizmoLineWidth;

    GLuint axisVAO, axisVBO;
    GLuint lightVAO, lightVBO;
    int selectedLightIndex;

    int objCounter;

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

    GLuint getActiveProgram() const { return lastActiveProgram; }

    void initGrid(int gridSize = 20, float spacing = 1.0f);
    void drawGrid(GLuint shaderProgram, const Mat4& view, const Mat4& projection);

    void saveScene(const std::string& path);
    void loadScene(const std::string& path);

private:
    GLuint gridVAO = 0, gridVBO = 0;
    int gridVertexCount = 0;
    GLuint lastActiveProgram = 0;
};

#endif
