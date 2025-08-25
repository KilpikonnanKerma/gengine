#define STB_IMAGE_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <iostream>
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "shaderc.hpp"
#include "input.hpp"
#include "Engine/sceneManager.hpp"
#include "Engine/editor.hpp"
#include "util/shapegen.hpp"

#include "GameMain.hpp"

int main(int argc, char* argv[]) {

    bool game_mode = false;
    for(int i = 0; i < argc; i++) {
        if(std::string(argv[i]) == "--game") { 
            game_mode = true;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("GENGINE",
                                          1024, 768,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

    float editorWidth = 0.f;

    bool running = true;
    SDL_Event event;

    GameMain game;
    Editor* editor;

    if(game_mode) {
        game.Start();
    } else {
        editor = new Editor(window, &game, editorWidth);
        game.scene.initGrid(50, 1.f);
        game.scene.initGizmo();   
    }

    while (running) {
        LAST = NOW;
        NOW = SDL_GetTicks();
        deltaTime = (NOW - LAST) / 1000.0f;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
            inputHandler.handleEvent(event, window);
        }

        inputHandler.processKeyboard(deltaTime);

        if(game_mode)
            game.Update(deltaTime);

        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        if(!game_mode) {
            editorWidth = 350.f;
            editor->Update();
        }

        float availableWidth = windowWidth - editorWidth;
        float availableHeight = windowHeight;
        float aspect = 16.f/10.f;
        if (game_mode) {
            aspect = (float)windowWidth / (float)windowHeight;
        }
        float viewportWidth = availableWidth;
        float viewportHeight = availableWidth / aspect;
        if (viewportHeight > availableHeight) {
            viewportHeight = availableHeight;
            viewportWidth = viewportHeight * aspect;
        }
        int viewportX = 0;
        int viewportY = (availableHeight - viewportHeight) / 2;

        if (game_mode) {
            viewportX = 0;
            viewportY = 0;
            viewportWidth  = windowWidth;
            viewportHeight = windowHeight;
        }

        glViewport(viewportX, viewportY, (int)viewportWidth, (int)viewportHeight);
        glClearColor(0.1f, 0.0f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(inputHandler.cameraPos,
                                    inputHandler.cameraPos + inputHandler.cameraFront,
                                    inputHandler.cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                (float)viewportWidth / (float)viewportHeight,
                                                0.1f, 100.0f);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        if (inputHandler.leftMouseClicked && !ImGui::GetIO().WantCaptureMouse) {
            const int mx = inputHandler.mouseX;
            const int my = inputHandler.mouseY;

            const bool inside =
                mx >= viewportX && mx < (viewportX + (int)viewportWidth) &&
                my >= viewportY && my < (viewportY + (int)viewportHeight);

            if (inside) {
                float mouseX = (float)inputHandler.mouseX;
                float mouseY = (float)inputHandler.mouseY;

                float vx = mouseX - viewportX;
                float vy = mouseY - viewportY;

                float ndcX =  (2.0f * vx) / viewportWidth  - 1.0f;
                float ndcY = 1.0f - (2.0f * vy) / viewportHeight;

                glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
                glm::vec4 rayClipFar(ndcX, ndcY, 1.0f, 1.0f);

                glm::mat4 invVP = glm::inverse(projection * view);
                glm::vec4 rayStart = invVP * rayClip; rayStart /= rayStart.w;
                glm::vec4 rayEnd   = invVP * rayClipFar; rayEnd /= rayEnd.w;

                glm::vec3 rayOrigin = glm::vec3(rayStart);
                glm::vec3 rayDir = glm::normalize(glm::vec3(rayEnd - rayStart));

                if(game.scene.pickGizmoAxis(inputHandler.cameraPos, rayDir, game.scene.grabbedAxis)) {
                    game.scene.axisGrabbed = true;
                } else {
                    game.scene.selectedObject = game.scene.pickObject(inputHandler.cameraPos, rayDir);
                    game.scene.axisGrabbed = false;
                }

                // game.scene.selectedObject = game.scene.pickObject(rayOrigin, rayDir);
            }

            inputHandler.leftMouseClicked = false;
        }

        if(game.scene.axisGrabbed){
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            float x = (2.0f * inputHandler.mouseX) / winW - 1.0f;
            float y = 1.0f - (2.0f * inputHandler.mouseY) / winH;
            glm::vec4 ray_clip = glm::vec4(x,y,-1.0,1.0);
            glm::mat4 invVP = glm::inverse(projection * view);
            glm::vec4 ray_world = invVP * ray_clip;
            ray_world /= ray_world.w;
            glm::vec3 ray_dir = glm::normalize(glm::vec3(ray_world) - inputHandler.cameraPos);

            game.scene.dragSelectedObject(inputHandler.cameraPos, ray_dir);
        }

        game.scene.render(shaderProgram, view, projection);

        if(!game_mode) {
            game.scene.drawGrid(shaderProgram, view, projection);
            glDisable(GL_DEPTH_TEST);
            game.scene.drawGizmo(shaderProgram, view, projection);
            glEnable(GL_DEPTH_TEST);
        }


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_Quit();
    return 0;
}
