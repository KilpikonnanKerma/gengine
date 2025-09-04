#include "Engine/editor.hpp"

Editor::Editor(SDL_Window* w, GameMain* g, float& width)
    : window(w), game(g), editorWidth(width), viewportTexture(0), viewportTexW(0), viewportTexH(0),
      viewportHovered(false), viewportMouseU(0.0f), viewportMouseV(0.0f), viewportMouseDown(false),
      currentProjectPath(""), selectedFile(""), objectCount(0), renaming(false),
      showBuildWindow(false), sceneFiles(), sceneSel(), buildMessage(), invokeCMakeBuild(true), selectedFolder()
{
    // initialize arrays
    pos[0]=pos[1]=pos[2]=0.0f;
    rot[0]=rot[1]=rot[2]=0.0f;
    texPath[0] = '\0';
    nameBuffer[0] = '\0';

    glShaderType = 0;
}

Editor::~Editor() {}

void Editor::Update() {
    int wW, wH; SDL_GetWindowSize(window, &wW, &wH);
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y+15), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y-15));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpaceParent", nullptr, dockspace_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    MainMenu();
    SceneGUI();
    EditorGUI();
    ProjectGUI();

    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleVar();

    ImGui::BeginChild("SceneToolbar", ImVec2(0, 28), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        if (ImGui::Button("Play")) {
            std::string tmpScene;
            if (currentProjectPath.empty()) {
                char cwd[4096];
                if (getcwd(cwd, sizeof(cwd)) != NULL) tmpScene = std::string(cwd) + "/tmp_play_scene.gscene";
                else tmpScene = std::string("./tmp_play_scene.gscene");
            } else {
                tmpScene = currentProjectPath + std::string("/tmp_play_scene.gscene");
            }
            game->scene->saveScene(tmpScene);
            printf("[Editor] Saved play-scene to '%s'\n", tmpScene.c_str());

            // Candidate executable names (prefer installed/packaged name first)
            const char* candidates[] = {"GENGINE", "gengine", "gengine-player", "./GENGINE", "./gengine", NULL};
            std::string exePath;
            char* base = SDL_GetBasePath();
            if (base) {
                for (int i = 0; candidates[i]; ++i) {
                    std::string cand = std::string(base) + candidates[i];
                    if (fs::exists(cand)) { exePath = cand; break; }
                }
                SDL_free(base);
            }
            if (exePath.empty()) {
                for (int i = 0; candidates[i]; ++i) {
                    std::string cand = std::string(candidates[i]);
                    if (fs::exists(cand)) { exePath = cand; break; }
                }
            }
            if (exePath.empty()) exePath = std::string("./GENGINE");

            std::string cmd = std::string("\"") + exePath + "\" --game --scene \"" + tmpScene + "\"";
            printf("[Editor] Launching: %s\n", cmd.c_str());
            int r = system(cmd.c_str());
            printf("[Editor] launch returned %d\n", r);
            (void)r;
        }
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::SameLine();
        if (ImGui::Button("lit")) {
            glShaderType = 0;
            std::cerr << "[Editor] Shader mode switched to: LIT" << std::endl;
        }

        ImGui::SameLine(); 
        if (ImGui::Button("unlit")) {
            glShaderType = 1;
            std::cerr << "[Editor] Shader mode switched to: UNLIT" << std::endl;
        }
    }
    ImGui::EndChild();

    if (viewportTexture != 0) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float aspect = (viewportTexW > 0 && viewportTexH > 0) ? (float)viewportTexW / (float)viewportTexH : (16.0f/9.0f);
        float width = avail.x;
        float height = width / aspect;
        if (height > avail.y) { height = avail.y; width = height * aspect; }
            ImVec2 uv0 = ImVec2(0.0f, 1.0f);
            ImVec2 uv1 = ImVec2(1.0f, 0.0f);
            ImGui::Image((ImTextureID)(intptr_t)viewportTexture, ImVec2(width, height), uv0, uv1);

            ImGuiIO& io = ImGui::GetIO();
            ImVec2 imgMin = ImGui::GetItemRectMin();
            ImVec2 imgMax = ImGui::GetItemRectMax();
            ImVec2 mousePos = io.MousePos;
            // Check hover
            viewportHovered = (mousePos.x >= imgMin.x && mousePos.x <= imgMax.x && mousePos.y >= imgMin.y && mousePos.y <= imgMax.y);
            if (viewportHovered) {
                float localX = mousePos.x - imgMin.x;
                float localY = mousePos.y - imgMin.y;
                float w = imgMax.x - imgMin.x;
                float h = imgMax.y - imgMin.y;
                viewportMouseU = localX / w;
                viewportMouseV = 1.0f - (localY / h);
                viewportMouseDown = ImGui::IsMouseDown(0);
            } else {
                viewportMouseDown = false;
            }
    } else {
        ImGui::Text("No viewport texture available");
    }
    ImGui::End();
}

void Editor::setViewportTexture(GLuint tex, int w, int h) {
    viewportTexture = tex;
    viewportTexW = w;
    viewportTexH = h;
}

void Editor::MainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                const char* newProjName = "MyProject";
                fs::create_directory((std::string)newProjName);
                std::ofstream projFile(newProjName + std::string("/project.gproject"));
                projFile << "name=" << newProjName << "\n";
                projFile.close();
                currentProjectPath = newProjName;
            }
            if (ImGui::MenuItem("Load Project")) {
                std::string path = "MyProject";
                if (fs::exists(path + "/project.gproject")) currentProjectPath = path;
            }
            if (ImGui::MenuItem("Save Scene")) SaveScene();
            if (ImGui::MenuItem("Load Scene")) LoadScene();

            // Build entry
            if (ImGui::MenuItem("Build")) {
                showBuildWindow = true;
            }

            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();

    if (showBuildWindow) {
        ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Build Game", &showBuildWindow)) {
            if (currentProjectPath.empty() || !fs::exists(currentProjectPath)) {
                ImGui::TextWrapped("No project loaded. Set or create a project to select scenes for build.");
                if (ImGui::Button("Close")) { showBuildWindow = false; }
                ImGui::End();
                return;
            }

            // Add current scene button (saves current scene into project/scenes)
            if (ImGui::Button("Add Current Scene to Project")) {
                fs::path scenesDir = fs::path(currentProjectPath) / "scenes";
                fs::create_directories(scenesDir);
                // choose a name
                std::string base = "scene";
                int i = 0;
                std::string out;
                do {
                    out = (scenesDir / (base + "_" + std::to_string(i) + ".gscene")).string();
                    ++i;
                } while (fs::exists(out));
                game->scene->saveScene(out);
                buildMessage = "Saved current scene into: " + out;
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh Scenes")) {
                // intentionally falls through to refresh list below
            }

            ImGui::Separator();

            // gather .gscene files under project/scenes (non-recursive)
            sceneFiles.clear();
            fs::path scenesDirPath = fs::path(currentProjectPath) / "scenes";
            if (fs::exists(scenesDirPath)) {
                for (auto &entry : fs::directory_iterator(scenesDirPath)) {
                    if (!entry.is_regular_file()) continue;
                    std::string s = entry.path_().string();
                    if (s.size() >= 7 && s.substr(s.size() - 7) == ".gscene") {
                        sceneFiles.push_back(s);
                    }
                }
            }
            // also allow top-level .gscene in project root
            if (fs::exists(currentProjectPath)) {
                for (auto &entry : fs::directory_iterator(currentProjectPath)) {
                    if (!entry.is_regular_file()) continue;
                    std::string s = entry.path_().string();
                    if (s.size() >= 7 && s.substr(s.size() - 7) == ".gscene") {
                        // avoid duplicates
                        bool dup = false;
                        for (size_t k = 0; k < sceneFiles.size(); ++k) if (sceneFiles[k] == s) { dup = true; break; }
                        if (!dup) sceneFiles.push_back(s);
                    }
                }
            }

            // sync selection vector
            if (sceneSel.size() != sceneFiles.size()) {
                sceneSel.assign(sceneFiles.size(), 0);
            }

            ImGui::Text("Scenes in project:");
            ImGui::BeginChild("ScenesList", ImVec2(0, 220), true);
            for (size_t i = 0; i < sceneFiles.size(); ++i) {
                ImGui::PushID((int)i);
                ImGui::Checkbox(sceneFiles[i].c_str(), (bool*) & sceneSel[i]);
                ImGui::PopID();
            }
            ImGui::EndChild();

            ImGui::Checkbox("Invoke CMake build for player target (cmake --build . --target GENGINE_PLAYER)", &invokeCMakeBuild);

            ImGui::Separator();
            if (ImGui::Button("Build")) {
                // prepare output build folder
                time_t now = time(NULL);
                char tsbuf[32]; snprintf(tsbuf, sizeof(tsbuf), "%lld", (long long)now);
                fs::path outDir = fs::path(currentProjectPath) / "Builds" / ("Build_" + std::string(tsbuf));
                fs::create_directories(outDir);

                // copy selected scenes
                fs::path scenesOut = outDir / "scenes";
                fs::create_directories(scenesOut);
                int copied = 0;
                for (size_t i = 0; i < sceneFiles.size(); ++i) {
                    if (!sceneSel[i]) continue;
                    fs::path src = sceneFiles[i];
                    fs::path dst = scenesOut / src.filename();
                    try {
                        fs::copy_file(src, dst, fs::copy_options::copy_options_overwrite_existing);
                        ++copied;
                    } catch (std::exception& e) {
                        std::cerr << "Failed to copy scene ";
                    }
                }

                // copy shaders and textures (project-level or engine-level)
                try {
                    fs::path shadersSrc = fs::path("shaders");
                    fs::path texturesSrc = fs::path("textures");
                    if (fs::exists(shadersSrc)) fs::copy(shadersSrc, outDir / "shaders", fs::copy_options_recursive | fs::copy_options_overwrite_existing);
                    if (fs::exists(texturesSrc)) fs::copy(texturesSrc, outDir / "textures", fs::copy_options_recursive | fs::copy_options_overwrite_existing);
                } catch (std::exception& e) {
                    std::cerr << "Asset copy error: " << e.what() << std::endl;
                }

                buildMessage = "Packaged " + std::to_string(copied) + " scenes to " + outDir.string();

                // optionally invoke cmake build for the player target
                if (invokeCMakeBuild) {
                    // note: this assumes current working dir is the configured build dir, or cmake was run from here
                    std::string buildCmd = "cmake --build . --target GENGINE_PLAYER --config Release";
                    int r = system(buildCmd.c_str());
                    if (r == 0) {
                        // try to find the executable in several likely locations and copy it beside the package
                        std::vector<fs::path> lookPaths = {
                            fs::path(fs::current_path()) / "GENGINE_PLAYER",
                            fs::path(fs::current_path()) / "GENGINE_PLAYER.exe",
                            fs::path(fs::current_path()) / "GENGINE_PLAYER.exe",
                            fs::path(fs::current_path()) / "Release" / "GENGINE_PLAYER.exe",
                            fs::path(fs::current_path()) / "Release" / "GENGINE_PLAYER",
                            fs::path(fs::current_path()) / "bin" / "Release" / "GENGINE_PLAYER.exe",
                            fs::path(fs::current_path()) / "bin" / "Release" / "GENGINE_PLAYER"
                        };
                        fs::path foundExe;
                        for (auto &p : lookPaths) {
                            if (fs::exists(p)) { foundExe = p; break; }
                        }
                        if (foundExe.string().empty()) {
                            // try scanning current directory for files starting with GENGINE_PLAYER
                            for (auto &entry : fs::directory_iterator(fs::current_path())) {
                                std::string name = entry.path_().filename().string();
                                if (name.find("GENGINE_PLAYER") != std::string::npos) {
                                    foundExe = entry.path_();
                                    break;
                                }
                            }
                        }
                        if (!foundExe.string().empty()) {
                            try {
                                fs::copy(foundExe, outDir / foundExe.filename(), fs::copy_options_overwrite_existing);
                                buildMessage += " — built exe copied: " + (outDir / foundExe.filename()).string();
                            } catch (std::exception& e) {
                                buildMessage += " — exe found but copy failed: ";
                                buildMessage += e.what();
                            }
                        } else {
                            buildMessage += " — built but exe not found automatically; check your build output.";
                        }
                    } else {
                        buildMessage += " — cmake build failed (see console output).";
                    }
                }
            }

            ImGui::Separator();
            ImGui::TextWrapped("%s", buildMessage.c_str());

            if (ImGui::Button("Close")) {
                showBuildWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Open Builds Folder")) {
                fs::path buildsRoot = fs::path(currentProjectPath) / "Builds";
                if (!fs::exists(buildsRoot)) fs::create_directories(buildsRoot);
#if defined(_WIN32)
                std::string cmd = "explorer \"" + buildsRoot.string() + "\"";
#elif defined(__APPLE__)
                std::string cmd = "open \"" + buildsRoot.string() + "\"";
#else
                std::string cmd = "xdg-open \"" + buildsRoot.string() + "\"";
#endif
                system(cmd.c_str());
            }

            ImGui::End();
        }
    }
}

void Editor::EditorGUI() {
    int wW, wH; SDL_GetWindowSize(window, &wW, &wH);
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoCollapse);

    Object* obj = game->scene->selectedObject;
    if (obj) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            pos[0]=obj->position.x; pos[1]=obj->position.y; pos[2]=obj->position.z;
            rot[0]=obj->rotation.x; rot[1]=obj->rotation.y; rot[2]=obj->rotation.z;
            scale[0] = obj->scale.x; scale[1] = obj->scale.y; scale[2] = obj->scale.z;

            ImGui::InputFloat3("Position", pos, "%.2f");
            ImGui::InputFloat3("Rotation", rot, "%.2f");
            ImGui::InputFloat3("Scale", scale, "%.2f");

            obj->position = Vec3d(pos[0],pos[1],pos[2]);
            obj->rotation = Vec3d(rot[0],rot[1],rot[2]);
            obj->scale = Vec3d(scale[0],scale[1],scale[2]);
        }

        if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputText("Path", texPath, IM_ARRAYSIZE(texPath));
            if (ImGui::Button("Apply")) {
                if (fs::exists((std::string)texPath)) obj->texture(texPath);
            }
            if (obj->textureID) ImGui::Image((void*)(intptr_t)obj->textureID, ImVec2(128,128));
        }
    } 
    if (game->scene->selectedLightIndex >= 0) {
        Light& l = game->scene->lights[game->scene->selectedLightIndex];
        float pos[3] = { l.position.x, l.position.y, l.position.z };
        float col[3] = { l.color.x, l.color.y, l.color.z };
        float intensity = l.intensity;
        if (ImGui::CollapsingHeader("Light Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat3("Position", pos);
            ImGui::ColorEdit3("Color", col);
            ImGui::InputFloat("Intensity", &intensity);
            l.position = Vec3d(pos[0], pos[1], pos[2]);
            l.color = Vec3d(col[0], col[1], col[2]);
            l.intensity = intensity;
        }
    }

    editorWidth = ImGui::GetWindowWidth();
    ImGui::End();
}

void Editor::SceneGUI() {
    ImGui::Begin("Scene");

    for (size_t i = 0; i < game->scene->lights.size(); ++i) {
        Light& l = game->scene->lights[i];
        ImGui::PushID((int)(1000 + i));
        bool selected = (game->scene->selectedLightIndex == (int)i);
        if (ImGui::Selectable(std::string("Light_" + std::to_string(i)).c_str(), selected)) {
            game->scene->selectedLightIndex = (int)i;
            game->scene->selectedObject = nullptr;
        }
        ImGui::PopID();
    }

    for (size_t i = 0; i < game->scene->objects.size(); i++) {
        Object* obj = game->scene->objects[i];
        ImGui::PushID((int)i);
        bool selected = (game->scene->selectedObject == obj);

        if (selected && renaming) {
            strcpy(nameBuffer, obj->name.c_str());
            if (ImGui::InputText("##rename", nameBuffer, IM_ARRAYSIZE(nameBuffer),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                obj->name = std::string(nameBuffer);
                renaming = false;
            }
        } else {
            if (ImGui::Selectable(obj->name.c_str(), selected)) {
                game->scene->selectedObject = obj;
                game->scene->selectedLightIndex = -1; // clear any light selection
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                renaming = true;
        }

        ImGui::PopID();
    }

    if (ImGui::BeginPopupContextWindow("SceneContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::BeginMenu("New Object")) {
            if (ImGui::MenuItem("Cube")) game->scene->addObject("Cube", "Cube_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Cylinder")) game->scene->addObject("Cylinder", "Cylinder_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Sphere")) game->scene->addObject("Sphere", "Sphere_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Plane")) game->scene->addObject("Plane", "Plane_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Pyramid")) game->scene->addObject("Pyramid", "Pyramid_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Player Start")) {
                // create a small cube to act as player start and give it a reserved name
                Object* ps = game->scene->addObject("Cube", "PlayerStart");
                if (ps) {
                    ps->scale = Vec3d(0.5f);
                }
            }
            if(ImGui::BeginMenu("Light")) {
                if(ImGui::MenuItem("Point Light")) {
                    Light dirLight(LightType::Point, Vec3d(0,0,0), 1.0f);
                }
                if(ImGui::MenuItem("Directional Light")) {
                    Light dirLight(LightType::Directional, Vec3d(0,0,0), 1.0f);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if ((game->scene->selectedObject || game->scene->selectedLightIndex != -1) && ImGui::MenuItem("Delete Object")) {
            if(game->scene->selectedObject) {
                game->scene->removeObject(game->scene->selectedObject);
                game->scene->selectedObject = nullptr;
            } else {
                game->scene->removeLight(game->scene->selectedLightIndex);
                game->scene->selectedLightIndex = -1;
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Editor::ProjectGUI() {
    ImGui::Begin("Project", nullptr, ImGuiWindowFlags_NoCollapse);

    static float treeWidth = 250.0f; // initial width
    float minWidth = 120.0f;
    float maxWidth = ImGui::GetContentRegionAvail().x - 100.0f;

    ImGui::BeginChild("TreePanel", ImVec2(treeWidth, 0), true);

    if (ImGui::Button("New Folder")) {
        if (!selectedFolder.empty()) {
            std::string newFolder = selectedFolder + "/NewFolder";
            fs::create_directory(newFolder);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("New C++ Class")) {
        if (!selectedFolder.empty()) {
            std::string className = "NewClass";
            std::ofstream header(selectedFolder + "/" + className + ".hpp");
            header << "#pragma once\n\nclass " << className << " {};\n";
            header.close();

            std::ofstream source(selectedFolder + "/" + className + ".cpp");
            source << "#include \"" << className << ".hpp\"\n";
            source.close();
        }
    }

    std::function<void(const fs::path&)> drawTree = [&](const fs::path& dir) {
        for (auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_directory()) continue;
            std::string name = entry.getpath().filename().string();

            // Tree node
            ImGuiTreeNodeFlags node_flags = ((selectedFolder == entry.getpath().string()) ? ImGuiTreeNodeFlags_Selected : 0);
            bool node_open = ImGui::TreeNodeEx(name.c_str(), node_flags);
            if (ImGui::IsItemClicked()) selectedFolder = entry.getpath();
            if (node_open) {
                drawTree(entry.getpath());
                ImGui::TreePop();
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("New Folder")) {
                    fs::create_directory(entry.getpath() / "NewFolder");
                }
                if (ImGui::MenuItem("New C++ Class")) {
                    std::string className = "NewClass";
                    std::ofstream header(entry.getpath() / (className + ".hpp"));
                    header << "#pragma once\n\nclass " << className << " {};\n";
                    header.close();
                    std::ofstream source(entry.getpath() / (className + ".cpp"));
                    source << "#include \"" << className << ".hpp\"\n";
                    source.close();
                }
                ImGui::EndPopup();
            }
        }
    };

    if (!currentProjectPath.empty() && fs::exists(currentProjectPath))
        drawTree(currentProjectPath);

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::PushID("Splitter");
    ImGui::InvisibleButton("##splitter", ImVec2(5, -1));
    if (ImGui::IsItemActive()) {
        treeWidth += ImGui::GetIO().MouseDelta.x;
        if (treeWidth < minWidth) treeWidth = minWidth;
        if (treeWidth > maxWidth) treeWidth = maxWidth;
    }
    ImGui::PopID();
    ImGui::SameLine();

    ImGui::BeginChild("ListPanel", ImVec2(0, 0), true);

    if (!selectedFolder.empty() && fs::exists(selectedFolder)) {
        for (auto& entry : fs::directory_iterator(selectedFolder)) {
            std::string name = entry.getpath().filename().string();
            bool isDir = entry.is_directory();

            if (isDir) ImGui::TextDisabled("[Folder] %s", name.c_str());
            else ImGui::Selectable(name.c_str(), selectedFile == entry.getpath().string());

            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("DND_FILE", entry.getpath().string().c_str(),
                                           entry.getpath().string().size() + 1);
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }

            if (isDir && ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_FILE")) {
                    const char* srcPath = (const char*)payload->Data;
                    fs::path dest = entry.getpath() / fs::path(srcPath).filename();
                    try { fs::rename(srcPath, dest); }
                    catch (std::exception& e) { std::cerr << "Error moving file: " << e.what() << std::endl; }
                }
                ImGui::EndDragDropTarget();
            }

            if (!isDir && ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete File")) fs::remove(entry.getpath());
                ImGui::EndPopup();
            }
        }
    } else {
        ImGui::Text("Select a folder to view its contents...");
    }

    ImGui::EndChild();
    ImGui::End();
}


void Editor::SaveScene() { 
    game->scene->saveScene(currentProjectPath+"/scene.gscene");
}
void Editor::LoadScene() {
    game->scene->loadScene(currentProjectPath+"/scene.gscene");
}
