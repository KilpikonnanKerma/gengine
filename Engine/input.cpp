#include "input.hpp"

EditorInput::EditorInput(SDL_Window* window) {
    cameraPos   = Vec3d(2.0f, 2.0f, 3.0f);
    cameraFront = Vec3d(-0.5f, -0.5f, -1.0f);
    cameraUp    = Vec3d(0.0f, 1.0f, 0.0f);

    yaw = -90.0f;
    pitch = 0.0f;
    speed = 2.5f;
    sensitivity = 0.1f;

    // SDL_HideCursor();
    // SDL_SetWindowRelativeMouseMode(window, true);
}

void EditorInput::processKeyboard(float deltaTime) {
    const bool* state = SDL_GetKeyboardState(NULL);
    float cameraSpeed = speed * deltaTime;

    if (state[SDL_SCANCODE_W]) cameraPos += cameraFront * cameraSpeed;
    if (state[SDL_SCANCODE_S]) cameraPos -= cameraFront * cameraSpeed;
    if (state[SDL_SCANCODE_A]) cameraPos -= cameraFront.cross(cameraUp).normalized() * cameraSpeed;
    if (state[SDL_SCANCODE_D]) cameraPos += cameraFront.cross(cameraUp).normalized() * cameraSpeed;
    if (state[SDL_SCANCODE_SPACE]) cameraPos += cameraUp * cameraSpeed;
    if (state[SDL_SCANCODE_LSHIFT]) cameraPos -= cameraUp * cameraSpeed;
}

void EditorInput::handleEvent(SDL_Event& e, SDL_Window* window) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_RIGHT) {
        rightMouseHeld = true;
        SDL_HideCursor();
        SDL_SetWindowRelativeMouseMode(window, true);
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_RIGHT) {
        rightMouseHeld = false;
        SDL_ShowCursor();
        SDL_SetWindowRelativeMouseMode(window, false);
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        leftMouseClicked = true;
        leftMouseHeld = true;
        SDL_GetMouseState(&mouseX, &mouseY);
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
        leftMouseHeld = false;
        SDL_GetMouseState(&mouseX, &mouseY);
    }
    if (rightMouseHeld && e.type == SDL_EVENT_MOUSE_MOTION) {
        processMouse(e);
    }
    if ((rightMouseHeld && e.type == SDL_EVENT_MOUSE_MOTION) || (leftMouseHeld && e.type == SDL_EVENT_MOUSE_MOTION)) {
        // Update mouse coords for dragging or camera control
        SDL_GetMouseState(&mouseX, &mouseY);
    }
}

void EditorInput::processMouse(SDL_Event& e) {
    if (e.type == SDL_EVENT_MOUSE_MOTION) {
        float xpos = (float)e.motion.xrel;
        float ypos = (float)e.motion.yrel;

        yaw   += xpos * sensitivity;
        pitch -= ypos * sensitivity;

        if(pitch > 89.0f) pitch = 89.0f;
        if(pitch < -89.0f) pitch = -89.0f;

        Vec3d front;
        front.x = cos(radians(yaw)) * cos(radians(pitch));
        front.y = sin(radians(pitch));
        front.z = sin(radians(yaw)) * cos(radians(pitch));
        cameraFront = front.normalized();
    }
}
