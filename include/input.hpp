#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class EditorInput {

public:
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    EditorInput(SDL_Window* window);
    void processKeyboard(float deltaTime);
    void processMouse(SDL_Event& e);
};

#endif