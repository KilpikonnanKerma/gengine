#include "Player.hpp"
#include <SDL2/SDL.h>
#include <string>

void Player::Update(float dt)
{
    if (!playerObject) return;
    const Uint8* state = SDL_GetKeyboardState(NULL);
    float speed = 5.0f * dt;
    
    NMATH::Vec3d forward = NMATH::Vec3d(0,0,1);
    NMATH::Vec3d right = NMATH::Vec3d(1,0,0);

    if (state[SDL_SCANCODE_W]) playerObject->position = playerObject->position - forward * speed;
    if (state[SDL_SCANCODE_S]) playerObject->position = playerObject->position + forward * speed;
    if (state[SDL_SCANCODE_A]) playerObject->position = playerObject->position - right * speed;
    if (state[SDL_SCANCODE_D]) playerObject->position = playerObject->position + right * speed;
}
