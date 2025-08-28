#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>

#include "nsm/math.hpp"

#include "imgui.h"

using namespace NMATH;

class EditorInput {

public:
    Vec3d cameraPos;
    Vec3d cameraFront;
    Vec3d cameraUp;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    bool leftMouseClicked = false;
    bool leftMouseHeld = false;
    float mouseX = 0, mouseY = 0;

    bool rightMouseHeld = false;

    EditorInput(SDL_Window* window);
    void processKeyboard(float deltaTime);

    void handleEvent(SDL_Event& e, SDL_Window* window);
    void processMouse(SDL_Event& e);
};

#endif