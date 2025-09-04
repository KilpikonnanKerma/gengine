#include "GameMain.hpp"
#include <cstdio>
#include "Engine/objects/shapegen.hpp"

GameMain::GameMain() {
    scene = new SceneManager();

    Light dirLight(LightType::Directional, Vec3d(1,1,1), 1.0f);
    dirLight.direction = Vec3d(-0.2f,-1.0f,-0.3f);
    scene->addLight(dirLight);

    cube1 = scene->addObject("Player", "Cube_1");
    cube1->position = Vec3d(-5.f, 0.f, 0.f);
    cube1->texture("textures/peppa.png");

    // Two ways of adding objects to a scene from c++
    sphere1 = scene->addObject("Sphere", "Sphere_1");
    sphere1->position = Vec3d(-5.f, 0.f, 5.f);
    sphere1->scale = Vec3d(1.f);
    sphere1->name = "Player";
    sphere1->texture("textures/yoda.png");

    Object* cylinder = new Object();
    cylinder->initCylinder(0.5f, 2.0f, 16);
    cylinder->name = "cylinderi";
    cylinder->position = Vec3d(-2.f, 0.f, 2.f);
	cylinder->texture("textures/yoda.png");
    scene->objects.push_back(cylinder);

    Light pointLight(LightType::Point, Vec3d(1,1,1), 1.0f);
    pointLight.position = Vec3d(5,0,0);
    scene->addLight(pointLight);

    player = new Player();
}

GameMain::~GameMain()
{
    delete player;

    if (scene) {
        delete scene;
        scene = NULL;
    }
}

void GameMain::Start()
{
    // Log object count for debugging
    int objCount = (int)scene->objects.size();
    printf("[GameMain] Scene has %d objects\n", objCount);

    //for(int i = 0; i < objCount; i++) {
    //    Object* obj = scene->objects[i];
    //    if(obj->name == "Player") {
    //        player->playerObject = obj;
    //        break;
    //    }
    //}

    player->playerObject = sphere1;
}

void GameMain::Update(float dt)
{
    if (sphere1) sphere1->rotation.y += 30.f * dt;
    player->Update(dt);
}