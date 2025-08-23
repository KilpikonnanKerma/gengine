#include "GameMain.h"

GameMain::GameMain() {
    scene = SceneManager();

    Light dirLight(LightType::Directional, glm::vec3(1,1,1), 1.0f);
    dirLight.direction = glm::vec3(-0.2f,-1.0f,-0.3f);
    scene.addLight(dirLight);

    Object floor;
    floor.initPlane(100, 100);
    floor.position = glm::vec3(0.0f, -1.0f, 0.0f);
    scene.addObject(floor);

    Object cube1;
    cube1.initCube(1.0f);
    cube1.position = glm::vec3(-2.0f, 0.0f, 5.0f);
    scene.addObject(cube1);

    Object sphere1;
    sphere1.initSphere(1.0f, 15, 15);
    sphere1.position = glm::vec3(-7.0f,0,5.0f);
    scene.addObject(sphere1);
}

void GameMain::Start() {

}

void GameMain::Update(float dt) {
    scene.update(dt);
}