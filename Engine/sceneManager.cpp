#include "Engine/sceneManager.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

void SceneManager::clearScene() {
    for (auto* o : objects) {
        delete o;
    }
    objects.clear();
    selectedObject = nullptr;
}

static void initMeshForType(Object* obj, const std::string& type) {
    if (type == "Cube")       obj->initCube(1.0f);
    else if (type == "Sphere")obj->initSphere(0.5f, 16, 16);
    else if (type == "Plane") obj->initPlane(5.0f, 5.0f);
    else if (type == "Pyramid") obj->initPyramid(1.0f, 1.0f);
    else                      obj->initCube(1.0f);
}

glm::vec3 closestPointOnLine(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                             const glm::vec3& linePoint, const glm::vec3& lineDir)
{
    glm::vec3 w0 = rayOrigin - linePoint;
    float a = glm::dot(lineDir, lineDir);
    float b = glm::dot(lineDir, rayDir);
    float c = glm::dot(rayDir, rayDir);
    float d = glm::dot(lineDir, w0);
    float e = glm::dot(rayDir, w0);

    float denom = a*c - b*b;
    float sc = (b*e - c*d) / denom;

    return linePoint + lineDir * sc;
}

void SceneManager::initGizmo() {
    // 3 lines: X, Y, Z axes
    std::vector<glm::vec3> gizmoVertices = {
        {0,0,0}, {1,0,0},   // X
        {0,0,0}, {0,1,0},   // Y
        {0,0,0}, {0,0,1}    // Z
    };

    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*gizmoVertices.size(), gizmoVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);

    glBindVertexArray(0);
}

void SceneManager::drawGizmo(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    if(!selectedObject) return;

    glm::vec3 pos = selectedObject->position;

    std::vector<GizmoAxis> axes = {
        {{0,0,0},{1,0,0},{1,0,0},{1,0,0}}, // X red
        {{0,0,0},{0,1,0},{0,1,0},{0,1,0}}, // Y green
        {{0,0,0},{0,0,1},{0,0,1},{0,0,1}}  // Z blue
    };

    glBindVertexArray(axisVAO);
    for(auto& axis : axes){
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram,"overrideColor"),1,&axis.color[0]);

        glDrawArrays(GL_LINES,0,2);
    }
    glBindVertexArray(0);
}

bool SceneManager::pickGizmoAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir, GizmoAxis& outAxis) {
    if(!selectedObject) return false;

    glm::vec3 pos = selectedObject->position;
    std::vector<GizmoAxis> axes = {
        {{0,0,0},{1,0,0},{1,0,0},{1,0,0}}, // X
        {{0,0,0},{0,1,0},{0,1,0},{0,1,0}}, // Y
        {{0,0,0},{0,0,1},{0,0,1},{0,0,1}}  // Z
    };

    float closestDist = FLT_MAX;
    bool hit = false;
    for(auto& axis : axes){
        glm::vec3 p1 = pos + axis.start;
        glm::vec3 p2 = pos + axis.end;
        glm::vec3 closest;
        float t;
        closest = closestPointOnLine(rayOrigin, rayDir, p1, p2 - p1);
        float dist = glm::length(closest - rayOrigin);
        if(dist < axisGrabDistance && dist < closestDist){
            closestDist = dist;
            outAxis = axis;
            hit = true;
        }
    }
    return hit;
}

void SceneManager::dragSelectedObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir){
    if(!axisGrabbed || !selectedObject) return;

    glm::vec3 pos = selectedObject->position;
    glm::vec3 closest;
    float t;
    closest = closestPointOnLine(rayOrigin, rayDir, pos, grabbedAxis.dir);
    selectedObject->position = closest; // snap exactly along axis
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

void SceneManager::addLight(const Light& light){
    lights.push_back(light);
}

void SceneManager::update(float deltaTime) {}

void SceneManager::render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
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
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    for (auto* obj : objects) {
        glm::mat4 model = obj->getModelMatrix();
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        if (obj->textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj->textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        } else {
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
        }

        glBindVertexArray(obj->VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)obj->indices.size(), GL_UNSIGNED_INT, 0);

        if (obj == selectedObject) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(5.0f);

            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
            glUniform3f(glGetUniformLocation(shaderProgram, "overrideColor"), 0.0f, 0.0f, 0.0f); // black

            glDrawElements(GL_TRIANGLES, (GLsizei)obj->indices.size(), GL_UNSIGNED_INT, 0);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glLineWidth(1.0f);
        }
    }
}

Object* SceneManager::pickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
    float minDist = FLT_MAX;
    Object* picked = nullptr;

    glm::vec3 dir = glm::normalize(rayDir);

    for (auto* obj : objects) {
        const glm::vec3 center = obj->position;
        const float radius = obj->boundingRadius();
        float t = 0.0f;

        if (glm::intersectRaySphere(rayOrigin, dir, center, radius * radius, t)) {
            if (t >= 0.0f && t < minDist) {
                minDist = t;
                picked = obj;
            }
        }
    }
    return picked;
}


void SceneManager::initGrid(int gridSize, float spacing) {
    std::vector<glm::vec3> gridVertices;

    for (int i = -gridSize; i <= gridSize; i++) {
        gridVertices.push_back(glm::vec3(i * spacing, 0.0f, -gridSize * spacing));
        gridVertices.push_back(glm::vec3(i * spacing, 0.0f,  gridSize * spacing));

        gridVertices.push_back(glm::vec3(-gridSize * spacing, 0.0f, i * spacing));
        gridVertices.push_back(glm::vec3( gridSize * spacing, 0.0f, i * spacing));
    }

    gridVertexCount = gridVertices.size();

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(glm::vec3),
                 gridVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
}

void SceneManager::drawGrid(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    if (gridVAO == 0) return;

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

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

    if (!j.contains("objects") || !j["objects"].is_array()) {
        std::cerr << "[loadScene] No 'objects' array in scene\n";
        return;
    }

    for (const auto& objJson : j["objects"]) {
        std::string type = objJson.value("type", "Cube");
        std::string name = objJson.value("name", type);

        glm::vec3 pos(0.0f), rot(0.0f), scl(1.0f);
        if (objJson.contains("position") && objJson["position"].size() == 3) {
            pos = glm::vec3(objJson["position"][0], objJson["position"][1], objJson["position"][2]);
        }
        if (objJson.contains("rotation") && objJson["rotation"].size() == 3) {
            rot = glm::vec3(objJson["rotation"][0], objJson["rotation"][1], objJson["rotation"][2]);
        }
        if (objJson.contains("scale") && objJson["scale"].size() == 3) {
            scl = glm::vec3(objJson["scale"][0], objJson["scale"][1], objJson["scale"][2]);
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