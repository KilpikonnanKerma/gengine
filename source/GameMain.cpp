#include "GameMain.hpp"

GameMain::GameMain() {
    scene = SceneManager();

    Light dirLight(LightType::Directional, glm::vec3(1,1,1), 1.0f);
    dirLight.direction = glm::vec3(-0.2f,-1.0f,-0.3f);
    scene.addLight(dirLight);

    cube1 = scene.addObject("Cube", "Cube_1");
    cube1->position = glm::vec3(-5.f, 0.f, 0.f);
    cube1->texture("textures/peppa.png");

    sphere1 = scene.addObject("Sphere", "Sphere_1");
    sphere1->position = glm::vec3(-5.f, 0.f, 5.f);
    sphere1->scale = glm::vec3(1.f);
    sphere1->texture("textures/yoda.png");
}

void GameMain::Start() {

}

void GameMain::Update(float dt) {
    cube1->rotation.y += 30.f * dt;
}