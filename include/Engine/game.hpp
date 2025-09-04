#ifndef GAME_HPP
#define GAME_HPP

#include <Engine/sceneManager.hpp>

class Game {

public:
    virtual void Start() {}
    virtual void Update(float dt) {}
};

#endif