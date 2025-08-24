#define STB_IMAGE_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <iostream>
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderc.hpp"
#include "input.hpp"
#include "Engine/sceneManager.hpp"
#include "util/shapegen.hpp"

#include "GameMain.hpp"

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("GENGINE",
                                          1024, 768,
                                          SDL_WINDOW_OPENGL);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shaderc ShaderCompiler;
    GLuint shaderProgram = ShaderCompiler.loadShader("shaders/basic.vert", "shaders/basic.frag");

    EditorInput inputHandler(window);

    Uint64 NOW = SDL_GetTicks();
    Uint64 LAST = 0;
    float deltaTime = 0.016f;

    bool running = true;
    SDL_Event event;

    GameMain game;
    game.Start();
    while (running) {
        LAST = NOW;
        NOW = SDL_GetTicks();
        deltaTime = (NOW - LAST) / 1000.0f;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            inputHandler.processMouse(event);
        }

        inputHandler.processKeyboard(deltaTime);

        game.Update(deltaTime);

        glClearColor(0.1f,0.0f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(inputHandler.cameraPos,inputHandler.cameraPos+inputHandler.cameraFront,inputHandler.cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),1024.0f/768.0f,0.1f,100.0f);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        glClearColor(0.1f,0.0f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        game.scene.render(shaderProgram, view, projection);

        SDL_GL_SwapWindow(window);
    }

    SDL_Quit();
    return 0;
}
