#include "GameMain.hpp"

GameMain::GameMain() {
    scene = SceneManager();
}

void GameMain::Start() {
    Light dirLight(LightType::Directional, glm::vec3(1,1,1), 1.0f);
    dirLight.direction = glm::vec3(-0.2f,-1.0f,-0.3f);
    scene.addLight(dirLight);

    floor.initPlane(100, 100);
    floor.position = glm::vec3(0.0f, -1.0f, 0.0f);
    floor.texture("textures/peppa.png");
    scene.addObject(floor);

    cube1.initCube(1.0f);
    cube1.position = glm::vec3(-2.0f, 0.0f, 5.0f);
    cube1.texture("textures/peppa.png");
    scene.addObject(cube1);

    sphere1.initSphere(1.0f, 15, 15);
    sphere1.position = glm::vec3(-7.0f,0,5.0f);
    sphere1.texture("textures/yoda.png");
    scene.addObject(sphere1);
}

void GameMain::Update(float dt) {
    cube1.rotation.y += 30.f * dt;
}