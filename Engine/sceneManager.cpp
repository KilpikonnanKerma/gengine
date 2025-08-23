#include "Engine/sceneManager.h"
#include <glm/gtc/type_ptr.hpp>

void SceneManager::addObject(const Object& obj) {
    objects.push_back(obj);
}

void SceneManager::addLight(const Light& light){
    lights.push_back(light);
}


void SceneManager::update(float deltaTime) {
    // Example: rotate all objects slowly
    for(auto& obj : objects) {
        obj.rotation.y += 0.5f * deltaTime;
    }
}

void SceneManager::render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);

    // LIGHTS
    int dirCount=0, pointCount=0;
    for(size_t i=0;i<lights.size();i++){
        Light& l = lights[i];
        if(l.type == LightType::Directional){
            std::string base = "dirLights[" + std::to_string(dirCount) + "]";
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".direction").c_str()),1,&l.direction[0]);
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".color").c_str()),1,&l.color[0]);
            glUniform1f(glGetUniformLocation(shaderProgram,(base+".intensity").c_str()),l.intensity);
            dirCount++;
        } else if(l.type == LightType::Point){
            std::string base = "pointLights[" + std::to_string(pointCount) + "]";
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".position").c_str()),1,&l.position[0]);
            glUniform3fv(glGetUniformLocation(shaderProgram,(base+".color").c_str()),1,&l.color[0]);
            glUniform1f(glGetUniformLocation(shaderProgram,(base+".intensity").c_str()),l.intensity);
            pointCount++;
        }
    }

    glUniform1i(glGetUniformLocation(shaderProgram,"numDirLights"), dirCount);
    glUniform1i(glGetUniformLocation(shaderProgram,"numPointLights"), pointCount);


    // OBJECTS
    for(auto& obj : objects) {
        glm::mat4 model = obj.getModelMatrix();
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc  = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc  = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.indices.size(), GL_UNSIGNED_INT, 0);
    }
}
