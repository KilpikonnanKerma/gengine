#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Engine/sceneManager.hpp"

class Player {
public:
    void Start();
    void Update(float dt);

    Object* playerObject;

};

#endif
