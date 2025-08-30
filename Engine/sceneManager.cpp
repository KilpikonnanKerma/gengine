#include "Engine/sceneManager.hpp"
#include <iostream>

void SceneManager::clearScene() {
    for (auto* o : objects) {
        delete o;
    }
    objects.clear();
    selectedObject = nullptr;
}

static void initMeshForType(Object* obj, const std::string& type) {
    if (type == "Cube")         obj->initCube(1.0f);
    else if (type == "Sphere")  obj->initSphere(0.5f, 16, 16);
    else if (type == "Plane")   obj->initPlane(5.0f, 5.0f);
    else if (type == "Pyramid") obj->initPyramid(1.0f, 1.0f);
    else                        obj->initCube(1.0f);
}

Vec3d closestPointOnLine(const Vec3d& rayOrigin, const Vec3d& rayDir,
                             const Vec3d& linePoint, const Vec3d& lineDir)
{
    // Solve for closest point on 'line' (point=linePoint + lineDir * s) to the ray (origin + rayDir * t)
    Vec3d w0 = rayOrigin - linePoint;
    float a = lineDir.dot(lineDir);           // |u|^2
    float b = lineDir.dot(rayDir);            // u.v
    float c = rayDir.dot(rayDir);             // |v|^2
    float d = lineDir.dot(w0);                // u.(w0)
    float e = rayDir.dot(w0);                 // v.(w0)

    float denom = a * c - b * b;
    const float EPS = 1e-8f;
    float s;
    if (fabs(denom) < EPS) {
        // Lines are almost parallel — pick projection of ray origin onto the line
        if (a < EPS) {
            // degenerate lineDir
            return linePoint;
        }
        s = d / a; // projection of w0 onto u
    } else {
        s = (b * e - c * d) / denom;
    }

    return linePoint + lineDir * s;
}

void SceneManager::initGizmo() {
    // 3 lines: X, Y, Z axes
    std::vector<Vec3d> gizmoVertices = {
        {0,0,0}, {1,0,0},   // X
        {0,0,0}, {0,1,0},   // Y
        {0,0,0}, {0,0,1}    // Z
    };

    if (!selectedObject) {
        if (objects.empty()) return;
        selectedObject = objects[0];
    }

    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3d)*gizmoVertices.size(), gizmoVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3d),(void*)0);

    glBindVertexArray(0);
}

void SceneManager::initLightGizmo() {
    // simple single-point gizmo (we use GL_POINTS) - one vertex at origin, will be translated per-light
    Vec3d v = {0,0,0};
    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);

    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3d), &v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vec3d),(void*)0);
    glBindVertexArray(0);
}


void SceneManager::drawGizmo(GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
    // Draw object gizmo axes only when an object is selected
    if (selectedObject) {
        Vec3d pos = selectedObject->position;

        std::vector<GizmoAxis> axes = {
            {{0,0,0},{1,0,0},{1,0,0},{1,0,0}}, // X red
            {{0,0,0},{0,1,0},{0,1,0},{0,1,0}}, // Y green
            {{0,0,0},{0,0,1},{0,0,1},{0,0,1}}  // Z blue
        };

        glBindVertexArray(axisVAO);
        glLineWidth(2.0f);
        for (size_t i = 0; i < axes.size(); ++i) {
            const auto& axis = axes[i];
            Mat4 model = translate(Mat4(1.0f), pos);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE, model.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE, view.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE, projection.value_ptr());
            glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 1);
            glUniform3fv(glGetUniformLocation(shaderProgram,"overrideColor"),1,&axis.color[0]);

            glDrawArrays(GL_LINES, (GLint)(i * 2), 2);
        }
        // restore override flag and GL state
        glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 0);
        glLineWidth(1.0f);
        glBindVertexArray(0);
    }

    // Draw light gizmos (always draw, even if no object is selected)
    if (lightVAO != 0) {
        glBindVertexArray(lightVAO);
        glPointSize(10.0f);
        for (size_t i = 0; i < lights.size(); ++i) {
            Mat4 model = translate(Mat4(1.0f), lights[i].position);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE, model.value_ptr());
            glUniform3fv(glGetUniformLocation(shaderProgram,"overrideColor"),1,&lights[i].color[0]);
            glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 1);
            if ((int)i == selectedLightIndex) {
                // brighter / bigger
                glPointSize(14.0f);
            }
            glDrawArrays(GL_POINTS, 0, 1);
            if ((int)i == selectedLightIndex) glPointSize(10.0f);
        }
        glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 0);
        glPointSize(1.0f);
        glBindVertexArray(0);
    }
}

bool SceneManager::pickLight(const Vec3d& rayOrigin, const Vec3d& rayDir, int& outIndex, float radius) {
    float closestT = FLT_MAX;
    int idx = -1;
    Vec3d dir = rayDir.normalized();
    for (size_t i = 0; i < lights.size(); ++i) {
        const Light& L = lights[i];
        float t = 0.0f;
        if (intersectRaySphere(rayOrigin, dir, L.position, radius * radius, t)) {
            if (t >= 0.0f && t < closestT) {
                closestT = t;
                idx = (int)i;
            }
        }
    }
    if (idx >= 0) {
        outIndex = idx;
        return true;
    }
    return false;
}

bool SceneManager::pickGizmoAxis(const Vec3d& rayOrigin, const Vec3d& rayDir, GizmoAxis& outAxis) {
    if (!selectedObject) return false;
    Vec3d pos = selectedObject->position;
    std::vector<GizmoAxis> axes = {
        {{0,0,0},{1,0,0},{1,0,0},{1,0,0}}, // X
        {{0,0,0},{0,1,0},{0,1,0},{0,1,0}}, // Y
        {{0,0,0},{0,0,1},{0,0,1},{0,0,1}}  // Z
    };

    float closestDist = FLT_MAX;
    bool hit = false;
    for(auto& axis : axes){
        Vec3d p1 = pos + axis.start;
        Vec3d p2 = pos + axis.end;
        Vec3d lineDir = (p2 - p1);
        float segLen = lineDir.length();
        if (segLen <= 1e-6f) continue;
        Vec3d lineDirNorm = lineDir / segLen;

        // Closest point on infinite line
        Vec3d closest = closestPointOnLine(rayOrigin, rayDir, p1, lineDir);

        // Project closest onto the segment parameter t in [0, segLen]
        float t = (closest - p1).dot(lineDirNorm);
        if (t < 0.0f) t = 0.0f;
        if (t > segLen) t = segLen;

        Vec3d clamped = p1 + lineDirNorm * t;

        // Distance from the ray to the clamped point (perpendicular distance)
        Vec3d rayDirNorm = rayDir.normalized();
        Vec3d v = clamped - rayOrigin;
        float along = v.dot(rayDirNorm);
        Vec3d closestOnRay = rayOrigin + rayDirNorm * along;
        float perpDist = (clamped - closestOnRay).length();

        // slightly increase grab distance for easier picking
        float grabThreshold = axisGrabDistance * 3.0f;
        if(perpDist < grabThreshold && perpDist < closestDist){
            closestDist = perpDist;
            // return axis with full segment vector in dir (world space) and world-space endpoints
            GizmoAxis worldAxis = axis;
            worldAxis.start = p1; // world-space
            worldAxis.end = p2;   // world-space
            worldAxis.dir = (p2 - p1); // full segment vector in world space
            // record initial projection along axis and object's position at pick time
            worldAxis.initialProj = t;
            worldAxis.initialObjPos = selectedObject ? selectedObject->position : Vec3d(0.0f);
            outAxis = worldAxis;
            hit = true;
        }
    }
    return hit;
}

void SceneManager::dragSelectedObject(const Vec3d& rayOrigin, const Vec3d& rayDir){
    if(!axisGrabbed || !selectedObject) return;

    // Use the grabbed axis endpoints recorded at pick time (world-space)
    Vec3d start = grabbedAxis.start;
    Vec3d end = grabbedAxis.end;
    Vec3d axisDir = grabbedAxis.dir;
    float len = axisDir.length();
    if (len < 1e-6f) return;
    Vec3d axisDirNorm = axisDir / len;

    // Compute closest point on the full axis segment's infinite line
    Vec3d closest = closestPointOnLine(rayOrigin, rayDir, start, axisDir);

    // Compute projected distance along axis from start (in world units)
    float closestProj = (closest - start).dot(axisDirNorm);

    // Use the projection captured at pick time so dragging is relative to where the user grabbed
    float initial = grabbedAxis.initialProj;
    Vec3d basePos = grabbedAxis.initialObjPos;

    float delta = closestProj - initial;

    // small deadzone to avoid jitter when mouse barely moves
    const float DEADZONE = 1e-4f;
    if (fabs(delta) < DEADZONE) delta = 0.0f;

    Vec3d newPos = basePos - axisDirNorm * delta;

    selectedObject->position = newPos;
}

Object* SceneManager::addObject(const std::string& type, const std::string& name) {
    Object* obj = new Object();
    initMeshForType(obj, type);
    obj->name = name;
    objects.push_back(obj);
    return obj;
}

void SceneManager::removeObject(Object* objPtr) {
    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [&](Object* obj) { return obj == objPtr; }),
        objects.end()
    );

    delete objPtr;
}

void SceneManager::removeLight(int index) {
    if (index >= 0 && index < (int)lights.size()) {
        lights.erase(lights.begin() + index);
    }
}

void SceneManager::addLight(const Light& light){
    lights.push_back(light);
    // ensure gizmo resources exist when lights are present
    if (lightVAO == 0) initLightGizmo();
}

void SceneManager::update(float deltaTime) {}

void SceneManager::render(GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
    glUseProgram(shaderProgram);

    int dirCount=0, pointCount=0;
    for (size_t i = 0; i < lights.size(); i++) {
        Light& l = lights[i];
        if (l.type == LightType::Directional) {
            std::string base = "dirLights[" + std::to_string(dirCount) + "]";
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".direction").c_str()),1,&l.direction[0]);
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".color").c_str()),1,&l.color[0]);
            glUniform1f(glGetUniformLocation(shaderProgram,(base+".intensity").c_str()),l.intensity);
            dirCount++;
        } else if (l.type == LightType::Point) {
            std::string base = "pointLights[" + std::to_string(pointCount) + "]";
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".position").c_str()),1,&l.position[0]);
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".color").c_str()),1,&l.color[0]);
            glUniform1f(glGetUniformLocation(shaderProgram,(base+".intensity").c_str()),l.intensity);
            pointCount++;
        }
    }
    glUniform1i(glGetUniformLocation(shaderProgram,"numDirLights"), dirCount);
    glUniform1i(glGetUniformLocation(shaderProgram,"numPointLights"), pointCount);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc  = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc  = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.value_ptr());
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.value_ptr());

    // Draw scene objects
    for (auto* obj : objects) {
        Mat4 model = obj->getModelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.value_ptr());

        if (obj->textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj->textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        } else {
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
        }

        // ensure override is off for normal object draw
        glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 0);

        glBindVertexArray(obj->VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)obj->indices.size(), GL_UNSIGNED_INT, 0);

        if (obj == selectedObject) {
            // Draw outline overlay for selected object
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(5.0f);

            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
            glUniform3f(glGetUniformLocation(shaderProgram, "overrideColor"), 0.0f, 0.0f, 0.0f);
            glUniform1i(glGetUniformLocation(shaderProgram, "useOverrideColor"), 1);

            glDrawElements(GL_TRIANGLES, (GLsizei)obj->indices.size(), GL_UNSIGNED_INT, 0);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glLineWidth(1.0f);
        }

        glBindVertexArray(0);
    }

    // Draw object gizmo axes only when an object is selected
    if (selectedObject) {
        Vec3d pos = selectedObject->position;

        std::vector<GizmoAxis> axes = {
            {{0,0,0},{1,0,0},{1,0,0},{1,0,0}}, // X red
            {{0,0,0},{0,1,0},{0,1,0},{0,1,0}}, // Y green
            {{0,0,0},{0,0,1},{0,0,1},{0,0,1}}  // Z blue
        };

        glBindVertexArray(axisVAO);
        glLineWidth(2.0f);
        for (size_t i = 0; i < axes.size(); ++i) {
            const auto& axis = axes[i];
            Mat4 model = translate(Mat4(1.0f), pos);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE, model.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE, view.value_ptr());
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE, projection.value_ptr());
            glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 1);
            glUniform3fv(glGetUniformLocation(shaderProgram,"overrideColor"),1,&axis.color[0]);

            glDrawArrays(GL_LINES, (GLint)(i * 2), 2);
        }
        // restore override flag and GL state
        glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 0);
        glLineWidth(1.0f);
        glBindVertexArray(0);
    }

    // Draw light gizmos (always draw, even if no object is selected)
    if (lightVAO != 0) {
        glBindVertexArray(lightVAO);
        glPointSize(10.0f);
        for (size_t i = 0; i < lights.size(); ++i) {
            Mat4 model = translate(Mat4(1.0f), lights[i].position);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE, model.value_ptr());
            glUniform3fv(glGetUniformLocation(shaderProgram,"overrideColor"),1,&lights[i].color[0]);
            glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 1);
            if ((int)i == selectedLightIndex) {
                // brighter / bigger
                glPointSize(14.0f);
            }
            glDrawArrays(GL_POINTS, 0, 1);
            if ((int)i == selectedLightIndex) glPointSize(10.0f);
        }
        glUniform1i(glGetUniformLocation(shaderProgram,"useOverrideColor"), 0);
        glPointSize(1.0f);
        glBindVertexArray(0);
    }
}
Object* SceneManager::pickObject(const Vec3d& rayOrigin, const Vec3d& rayDir) {
    float minDist = FLT_MAX;
    Object* picked = nullptr;

    Vec3d dir = rayDir.normalized();

    for (auto* obj : objects) {
        const Vec3d center = obj->position;
        const float radius = obj->boundingRadius();
        float t = 0.0f;

        if (intersectRaySphere(rayOrigin, dir, center, radius * radius, t)) {
            if (t >= 0.0f && t < minDist) {
                minDist = t;
                picked = obj;
            }
        }
    }
    return picked;
}


void SceneManager::initGrid(int gridSize, float spacing) {
    std::vector<Vec3d> gridVertices;

    for (int i = -gridSize; i <= gridSize; i++) {
        gridVertices.push_back(Vec3d(i * spacing, 0.0f, -gridSize * spacing));
        gridVertices.push_back(Vec3d(i * spacing, 0.0f,  gridSize * spacing));

        gridVertices.push_back(Vec3d(-gridSize * spacing, 0.0f, i * spacing));
        gridVertices.push_back(Vec3d( gridSize * spacing, 0.0f, i * spacing));
    }

    gridVertexCount = gridVertices.size();

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(Vec3d),
                 gridVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3d), (void*)0);

    glBindVertexArray(0);
}

void SceneManager::drawGrid(GLuint shaderProgram, const Mat4& view, const Mat4& projection) {
    if (gridVAO == 0) return;

    Mat4 model = Mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model.value_ptr());
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.value_ptr());
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.value_ptr());

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertexCount);
    glBindVertexArray(0);
}

void SceneManager::saveScene(const std::string& path) {
    json j;
    j["objects"] = json::array();

    for (auto* obj : objects) {
        json o;
        o["name"] = obj->name;
        o["type"] = obj->type.empty() ? "Cube" : obj->type;
        o["position"] = { obj->position.x, obj->position.y, obj->position.z };
        o["rotation"] = { obj->rotation.x, obj->rotation.y, obj->rotation.z };
        o["scale"]    = { obj->scale.x,    obj->scale.y,    obj->scale.z    };
        o["texturePath"] = obj->texturePath;
        j["objects"].push_back(o);
    }

    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(4);
    } else {
        std::cerr << "[saveScene] Failed to open " << path << " for writing\n";
    }
}

void SceneManager::loadScene(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[loadScene] Failed to open " << path << "\n";
        return;
    }

    json j;
    try {
        file >> j;
    } catch (const std::exception& e) {
        std::cerr << "[loadScene] JSON parse error: " << e.what() << "\n";
        return;
    }

    clearScene();

	if (j.find("objects") == j.end() || !j["objects"].is_array()) {
		std::cerr << "[loadScene] No 'objects' array in scene\n";
		return;
	}

    for (const auto& objJson : j["objects"]) {
        std::string type = objJson.value("type", "Cube");
        std::string name = objJson.value("name", type);

        Vec3d pos(0.0f), rot(0.0f), scl(1.0f);
        if (objJson.find("position") != objJson.end() && objJson["position"].size() == 3) {
            pos = Vec3d(objJson["position"][0], objJson["position"][1], objJson["position"][2]);
        }
        if (objJson.find("rotation") != objJson.end() && objJson["rotation"].size() == 3) {
            rot = Vec3d(objJson["rotation"][0], objJson["rotation"][1], objJson["rotation"][2]);
        }
        if (objJson.find("scale") != objJson.end() && objJson["scale"].size() == 3) {
            scl = Vec3d(objJson["scale"][0], objJson["scale"][1], objJson["scale"][2]);
        }
        std::string texPath = objJson.value("texturePath", "");

        Object* o = new Object();
        o->name = name;
        o->type = type;

        initMeshForType(o, type);

        o->position = pos;
        o->rotation = rot;
        o->scale    = scl;

        if (!texPath.empty()) {
            o->texture(texPath);
        }

        objects.push_back(o);
    }
}