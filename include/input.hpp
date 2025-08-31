#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>

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

    bool leftMouseClicked;
    bool leftMouseHeld;
    int mouseX, mouseY;

    bool rightMouseHeld;

    EditorInput(SDL_Window* window);
    void processKeyboard(float deltaTime);

    void handleEvent(SDL_Event& e, SDL_Window* window);
    void processMouse(SDL_Event& e);

    // Handle clicks/drags/camera while interacting with the editor viewport.
    // This moves the previous main.cpp input handling into input.cpp.
    // viewportX/Y/W/H are in window coordinates.
    void handleViewportInput(class Editor* editor, class GameMain& game,
                             const Mat4& view, const Mat4& projection,
                             int viewportX, int viewportY, int viewportW, int viewportH);

    // Internal state used for edge detection when processing viewport mouse
    bool prevViewportMouseDown;
};

#endif