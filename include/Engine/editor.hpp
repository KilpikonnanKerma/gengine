#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "SDL2/SDL.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Engine/sceneManager.hpp"
#include "GameMain.hpp"
#include "filesystem/filesystem.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

// 0 - lit
// 1 - unlit
extern int glShaderType;

class Editor {
public:
    Editor(SDL_Window* window, GameMain* game, float& editorWidth);
    ~Editor();

    void Update();

    void setViewportTexture(GLuint tex, int w, int h);

    bool isViewportHovered() const { return viewportHovered; }
    void getViewportMouseUV(float& u, float& v) const { u = viewportMouseU; v = viewportMouseV; }
    bool isViewportMouseDown() const { return viewportMouseDown; }


private:
    void MainMenu();
    void EditorGUI();
    void SceneGUI();
    void ProjectGUI();

    void SaveScene();
    void LoadScene();

    SDL_Window* window;
    GameMain* game;
    float editorWidth;

    GLuint viewportTexture;
    int viewportTexW;
    int viewportTexH;

    bool viewportHovered;
    float viewportMouseU;
    float viewportMouseV;
    bool viewportMouseDown;

    std::string currentProjectPath;
    std::string selectedFile;
    std::vector<std::string> projectFiles;

    int objectCount;
    bool renaming;
    char nameBuffer[128];

    float pos[3];
    float rot[3];
    float scale;
    char texPath[256];
};

#endif
