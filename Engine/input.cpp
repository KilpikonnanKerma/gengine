#include "input.hpp"

EditorInput::EditorInput(SDL_Window* window) {
    cameraPos   = glm::vec3(2.0f, 2.0f, 3.0f);
    cameraFront = glm::vec3(-0.5f, -0.5f, -1.0f);
    cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

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

    if (state[SDL_SCANCODE_W]) cameraPos += cameraSpeed * cameraFront;
    if (state[SDL_SCANCODE_S]) cameraPos -= cameraSpeed * cameraFront;
    if (state[SDL_SCANCODE_A]) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (state[SDL_SCANCODE_D]) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (state[SDL_SCANCODE_SPACE]) cameraPos += cameraSpeed * cameraUp;
    if (state[SDL_SCANCODE_LSHIFT]) cameraPos -= cameraSpeed * cameraUp;
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
    if (rightMouseHeld && e.type == SDL_EVENT_MOUSE_MOTION) {
        processMouse(e);
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        leftMouseClicked = true;
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

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}
