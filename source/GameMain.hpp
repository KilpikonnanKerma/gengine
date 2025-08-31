#ifndef GAME_MAIN_HPP
#define GAME_MAIN_HPP

#include <Engine/game.hpp>
#include <Engine/sceneManager.hpp>

class GameMain : public Game {
public:
    GameMain();
    ~GameMain();

    void Start();
    void Update(float dt);

    SceneManager* scene;

    Object* floor;
    Object* cube1;
    Object* sphere1;
};

#endif