#include "Engine/editor.hpp"
namespace fs = std::filesystem;

Editor::Editor(SDL_Window* w, GameMain* g, float& width)
    : window(w), game(g), editorWidth(width) {

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
}

void Editor::MainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {
                const char* newProjName = "MyProject";
                fs::create_directory(newProjName);
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
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Editor::EditorGUI() {
    int wW, wH; SDL_GetWindowSize(window, &wW, &wH);
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoCollapse);

    Object* obj = game->scene.selectedObject;
    if (obj) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            pos[0]=obj->position.x; pos[1]=obj->position.y; pos[2]=obj->position.z;
            rot[0]=obj->rotation.x; rot[1]=obj->rotation.y; rot[2]=obj->rotation.z;
            scale = obj->scale.x;

            ImGui::InputFloat3("Position", pos, "%.2f");
            ImGui::InputFloat3("Rotation", rot, "%.2f");
            ImGui::InputFloat("Scale", &scale, 0.0f, 0.0f, "%.2f");

            obj->position = glm::vec3(pos[0],pos[1],pos[2]);
            obj->rotation = glm::vec3(rot[0],rot[1],rot[2]);
            obj->scale = glm::vec3(scale);
        }

        if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputText("Path", texPath, IM_ARRAYSIZE(texPath));
            if (ImGui::Button("Apply")) {
                if (fs::exists(texPath)) obj->texture(texPath);
            }
            if (obj->textureID) ImGui::Image((void*)(intptr_t)obj->textureID, ImVec2(128,128));
        }
    }

    editorWidth = ImGui::GetWindowWidth();
    ImGui::End();
}

void Editor::SceneGUI() {
    ImGui::Begin("Scene");

    for (size_t i = 0; i < game->scene.objects.size(); i++) {
        Object* obj = game->scene.objects[i];
        ImGui::PushID((int)i);
        bool selected = (game->scene.selectedObject == obj);

        if (selected && renaming) {
            strcpy(nameBuffer, obj->name.c_str());
            if (ImGui::InputText("##rename", nameBuffer, IM_ARRAYSIZE(nameBuffer),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                obj->name = std::string(nameBuffer);
                renaming = false;
            }
        } else {
            if (ImGui::Selectable(obj->name.c_str(), selected)) game->scene.selectedObject = obj;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                renaming = true;
        }

        ImGui::PopID();
    }

    if (ImGui::BeginPopupContextWindow("SceneContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::BeginMenu("New Object")) {
            if (ImGui::MenuItem("Cube")) game->scene.addObject("Cube", "Cube_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Sphere")) game->scene.addObject("Sphere", "Sphere_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Plane")) game->scene.addObject("Plane", "Plane_" + std::to_string(objectCount++));
            if (ImGui::MenuItem("Pyramid")) game->scene.addObject("Pyramid", "Pyramid_" + std::to_string(objectCount++));
            ImGui::EndMenu();
        }
        if (game->scene.selectedObject && ImGui::MenuItem("Delete Object")) {
            game->scene.removeObject(game->scene.selectedObject);
            game->scene.selectedObject = nullptr;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

std::string selectedFolder;
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
            std::string name = entry.path().filename().string();

            // Tree node
            ImGuiTreeNodeFlags node_flags = ((selectedFolder == entry.path()) ? ImGuiTreeNodeFlags_Selected : 0);
            bool node_open = ImGui::TreeNodeEx(name.c_str(), node_flags);
            if (ImGui::IsItemClicked()) selectedFolder = entry.path();
            if (node_open) {
                drawTree(entry.path());
                ImGui::TreePop();
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("New Folder")) {
                    fs::create_directory(entry.path() / "NewFolder");
                }
                if (ImGui::MenuItem("New C++ Class")) {
                    std::string className = "NewClass";
                    std::ofstream header(entry.path() / (className + ".hpp"));
                    header << "#pragma once\n\nclass " << className << " {};\n";
                    header.close();
                    std::ofstream source(entry.path() / (className + ".cpp"));
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
            std::string name = entry.path().filename().string();
            bool isDir = entry.is_directory();

            if (isDir) ImGui::TextDisabled("[Folder] %s", name.c_str());
            else ImGui::Selectable(name.c_str(), selectedFile == entry.path().string());

            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("DND_FILE", entry.path().string().c_str(),
                                           entry.path().string().size() + 1);
                ImGui::Text("%s", name.c_str());
                ImGui::EndDragDropSource();
            }

            if (isDir && ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_FILE")) {
                    const char* srcPath = (const char*)payload->Data;
                    fs::path dest = entry.path() / fs::path(srcPath).filename();
                    try { fs::rename(srcPath, dest); } 
                    catch (std::exception& e) { std::cerr << "Error moving file: " << e.what() << std::endl; }
                }
                ImGui::EndDragDropTarget();
            }

            if (!isDir && ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete File")) fs::remove(entry.path());
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
    game->scene.saveScene(currentProjectPath+"/scene.gscene");
}
void Editor::LoadScene() {
    game->scene.loadScene(currentProjectPath+"/scene.gscene");
}
