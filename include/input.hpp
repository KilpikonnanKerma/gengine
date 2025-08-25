#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"

class EditorInput {

public:
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    bool leftMouseClicked = false;
    float mouseX = 0, mouseY = 0;

    bool rightMouseHeld = false;

    EditorInput(SDL_Window* window);
    void processKeyboard(float deltaTime);

    void handleEvent(SDL_Event& e, SDL_Window* window);
    void processMouse(SDL_Event& e);
};

#endif