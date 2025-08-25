#ifndef GAME_MAIN_HPP
#define GAME_MAIN_HPP

#include <Engine/game.hpp>
#include <Engine/sceneManager.hpp>

class GameMain : Game {
public:

    GameMain();

    void Start();
    void Update(float dt);

    SceneManager scene;

    Object* floor;
    Object* cube1;
    Object* sphere1;
};

#endif