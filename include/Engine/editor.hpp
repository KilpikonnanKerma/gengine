#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "Engine/sceneManager.hpp"
#include "GameMain.hpp"
#include "filesystem/filesystem.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

class Editor {
public:
    Editor(SDL_Window* window, GameMain* game, float& editorWidth);
    ~Editor();

    void Update();

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

    std::string currentProjectPath = "";
    std::string selectedFile = "";
    std::vector<std::string> projectFiles;

    // Scene GUI state
    int objectCount = 0;
    bool renaming = false;
    char nameBuffer[128];

    // Editor GUI state
    float pos[3]{0,0,0};
    float rot[3]{0,0,0};
    float scale = 1.0f;
    char texPath[256]{""};
};

#endif
