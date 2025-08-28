#define STB_IMAGE_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <iostream>
#include "glad/glad.h"
#include "nsm/math.hpp"

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

using namespace NMATH;

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
    game.scene.initLightGizmo();
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

        Mat4 view = lookAt(inputHandler.cameraPos,
                                    inputHandler.cameraPos + inputHandler.cameraFront,
                                    inputHandler.cameraUp);
        Mat4 projection = perspective(radians(45.0f),
                                                (float)viewportWidth / (float)viewportHeight,
                                                0.1f, 100.0f);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view.value_ptr());
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection.value_ptr());

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

                Vec4d rayClip(ndcX, ndcY, -1.0f, 1.0f);
                Vec4d rayClipFar(ndcX, ndcY, 1.0f, 1.0f);

                Mat4 invVP = (projection * view).inverse();
                Vec4d rayStart = invVP * rayClip; rayStart /= rayStart.w;
                Vec4d rayEnd   = invVP * rayClipFar; rayEnd /= rayEnd.w;

                Vec3d rayOrigin = Vec3d(rayStart);
                Vec3d rayDir = Vec3d(rayEnd - rayStart).normalized();

                if(game.scene.pickGizmoAxis(rayOrigin, rayDir, game.scene.grabbedAxis)) {
                    game.scene.axisGrabbed = true;
                    game.scene.objectDrag = false;
                    // picking gizmo implies an object is selected; clear light selection
                    game.scene.selectedLightIndex = -1;
                } else {
                    // Object pick (free drag)
                    game.scene.selectedObject = game.scene.pickObject(rayOrigin, rayDir);
                    game.scene.axisGrabbed = false;
                    if (game.scene.selectedObject) {
                        // clear light selection when selecting an object
                        game.scene.selectedLightIndex = -1;
                        // create a drag plane perpendicular to camera front at the object's position
                        game.scene.objectDrag = true;
                        game.scene.dragPlaneNormal = inputHandler.cameraFront.normalized();
                        // intersect ray with plane (plane at object's current position)
                        Vec3d planePoint = game.scene.selectedObject->position;
                        // plane: (p - planePoint) dot N = 0
                        float denom = rayDir.dot(game.scene.dragPlaneNormal);
                        if (fabs(denom) > 1e-6f) {
                            float t = (planePoint - rayOrigin).dot(game.scene.dragPlaneNormal) / denom;
                            Vec3d hitPoint = rayOrigin + rayDir * t;
                            game.scene.dragInitialPoint = hitPoint;
                            game.scene.dragInitialObjPos = game.scene.selectedObject->position;
                        } else {
                            game.scene.dragInitialPoint = game.scene.selectedObject->position;
                            game.scene.dragInitialObjPos = game.scene.selectedObject->position;
                        }
            } else {
                        // try picking a light
                        int lightIdx = -1;
                        if (game.scene.pickLight(rayOrigin, rayDir, lightIdx, 0.6f)) {
                game.scene.selectedLightIndex = lightIdx;
                // clear object selection when a light is selected
                game.scene.selectedObject = nullptr;
                            game.scene.objectDrag = true; // reuse plane drag for lights
                            game.scene.dragPlaneNormal = inputHandler.cameraFront.normalized();
                            Vec3d planePoint = game.scene.lights[lightIdx].position;
                            float denom = rayDir.dot(game.scene.dragPlaneNormal);
                            if (fabs(denom) > 1e-6f) {
                                float t = (planePoint - rayOrigin).dot(game.scene.dragPlaneNormal) / denom;
                                Vec3d hitPoint = rayOrigin + rayDir * t;
                                game.scene.dragInitialPoint = hitPoint;
                                game.scene.dragInitialObjPos = game.scene.lights[lightIdx].position;
                            } else {
                                game.scene.dragInitialPoint = game.scene.lights[lightIdx].position;
                                game.scene.dragInitialObjPos = game.scene.lights[lightIdx].position;
                            }
                        } else {
                            // clicked empty space: deselect both object and light
                            game.scene.objectDrag = false;
                            game.scene.selectedLightIndex = -1;
                            game.scene.selectedObject = nullptr;
                        }
                    }
                }

                // game.scene.selectedObject = game.scene.pickObject(rayOrigin, rayDir);
            }

            inputHandler.leftMouseClicked = false;
        }

        if(game.scene.axisGrabbed){
            float vx = (float)inputHandler.mouseX - viewportX;
            float vy = (float)inputHandler.mouseY - viewportY;

            // If mouse is outside the viewport, skip dragging
                if (vx < 0 || vx > viewportWidth || vy < 0 || vy > viewportHeight) {
            } else {
                float ndcX = (2.0f * vx) / viewportWidth - 1.0f;
                float ndcY = 1.0f - (2.0f * vy) / viewportHeight;

                Vec4d rayClipNear(ndcX, ndcY, -1.0f, 1.0f);
                Vec4d rayClipFar(ndcX, ndcY, 1.0f, 1.0f);
                Mat4 invVP = (projection * view).inverse();
                Vec4d nearWorld = invVP * rayClipNear; nearWorld /= nearWorld.w;
                Vec4d farWorld  = invVP * rayClipFar;  farWorld  /= farWorld.w;

                Vec3d rayOriginDrag = Vec3d(nearWorld);
                Vec3d rayDirDrag = Vec3d(farWorld - nearWorld).normalized();

                if (game.scene.objectDrag && game.scene.selectedObject) {
                    // intersect ray with drag plane
                    Vec3d N = game.scene.dragPlaneNormal;
                    Vec3d planeP = game.scene.dragInitialPoint;
                    float denom = rayDirDrag.dot(N);
                    if (fabs(denom) > 1e-6f) {
                        float t = (planeP - rayOriginDrag).dot(N) / denom;
                        Vec3d hit = rayOriginDrag + rayDirDrag * t;
                        Vec3d delta = hit - game.scene.dragInitialPoint;
                        Vec3d newPos = game.scene.dragInitialObjPos + delta;
                        if (game.scene.selectedObject) {
                            game.scene.selectedObject->position = newPos;
                        } else if (game.scene.selectedLightIndex >= 0) {
                            game.scene.lights[game.scene.selectedLightIndex].position = newPos;
                        }
                    }
                } else {
                    game.scene.dragSelectedObject(rayOriginDrag, rayDirDrag);
                }
            }
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
