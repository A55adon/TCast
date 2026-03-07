#include "Window.h"
#include "Shell.h"
#ifdef NDEBUG
#else
#include "RmlUi/Debugger.h"
#endif
#include "RmlUi_Backend.h"

#include <iostream>
//#ifdef NDEBUG
//#else
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
//#endif
#include "global.h"
#include "ResourceHandler.h"
#include "UIManager.h"
#include "Utilities.h"

static bool showDebugWindow = false;

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    Utilities::handleFileDragNDrop(count, paths);
}

Window::Window(const int window_width, const int window_height): document(nullptr)
{
    if (!ShellRml::Initialize())
        exit(1);


    if (!Backend::Initialize("TCast", window_width, window_height, true)) {
        std::cout << "[Window] backendShutdown" << std::endl;
        ShellRml::Shutdown();
        exit(1);
    }

    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());
    Rml::Initialise();




    glfwInit();

    context = Rml::CreateContext("main", Rml::Vector2i(window_width, window_height));
    if (!context) {
        Rml::Shutdown();
        Backend::Shutdown();
        ShellRml::Shutdown();
        exit(1);
    }
#ifdef NDEBUG
#else
    Rml::Debugger::Initialise(context);
#endif

    const std::vector<ShellRml::FontFace> font_faces = {
        {"LatoLatin-Regular.ttf", false},
        {"ComicSans-Regular.ttf", true}
    };


    ShellRml::LoadFonts(font_faces);

    glfwMaximizeWindow(Backend::GetWindow());
#ifdef NDEBUG
#else
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(Backend::GetWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    glfwSetDropCallback(Backend::GetWindow(), drop_callback);

    running = true;
}



Window::~Window()
{
    Rml::Shutdown();
    Backend::Shutdown();
    ShellRml::Shutdown();
}

void Window::update()
{
    // Update main window
    glfwMakeContextCurrent(Backend::GetWindow());
    running = Backend::ProcessEvents(context, &ShellRml::ProcessKeyDownShortcuts, false);
    context->Update();
    Backend::BeginFrame();
    context->Render();
#ifdef NDEBUG
#else
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        showDebugWindow = !showDebugWindow;
    }
    // ====== YOUR DEBUG WINDOW CODE GOES HERE ======
    if (showDebugWindow) {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Debug Window", &showDebugWindow);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Close")) showDebugWindow = false;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Tab bar
        if (ImGui::BeginTabBar("DebugTabs")) {

            // Save Data Tab
            if (ImGui::BeginTabItem("Save Data")) {
                ImGui::SeparatorText("Project Information");
                ImGui::Text("Project Name: %s", save_data.name.c_str());
                ImGui::Text("Description: %s", save_data.description.c_str());
                ImGui::Text("Projector Count: %d", save_data.projector_amount);
                ImGui::Text("Path: %s", save_data.path.string().c_str());
                ImGui::Text("Version: %s", save_data.version.c_str());

                // Project path
                ImGui::SeparatorText("Current Project Path");
                ImGui::Text("%s", current_project_path.string().c_str());

                ImGui::EndTabItem();
            }

            // Scenes Tab
            if (ImGui::BeginTabItem("Scenes")) {
                ImGui::Text("Active Scene Index: %d", active_scene_index);
                ImGui::Text("Total Scenes: %zu", scene_manager.scenes.size());

                if (ImGui::BeginTable("ScenesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Index");
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Sources");
                    ImGui::TableSetupColumn("Connections");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < scene_manager.scenes.size(); i++) {
                        const auto& scene = scene_manager.scenes[i];
                        ImGui::TableNextRow();

                        // Index
                        ImGui::TableSetColumnIndex(0);
                        if (i == active_scene_index) {
                            ImGui::TextColored(ImVec4(0, 1, 0, 1), "▶ %d", i);
                        } else {
                            ImGui::Text("%d", i);
                        }

                        // Name
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", scene.name.c_str());

                        // Sources count
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%zu", scene.sources.size());

                        // Connections
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%zu", scene.connections.size());

                        // Details button
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            // Show detailed view
                        }
                    }
                    ImGui::EndTable();
                }

                // Scene details for selected scene
                if (active_scene_index >= 0 && active_scene_index < scene_manager.scenes.size()) {
                    ImGui::SeparatorText("Active Scene Details");
                    const auto& scene = scene_manager.scenes[active_scene_index];

                    if (ImGui::BeginTable("SceneSources", 3, ImGuiTableFlags_Borders)) {
                        ImGui::TableSetupColumn("Projector");
                        ImGui::TableSetupColumn("Source");
                        ImGui::TableSetupColumn("Connection");
                        ImGui::TableHeadersRow();

                        for (int i = 0; i < save_data.projector_amount; i++) {
                            ImGui::TableNextRow();

                            // Projector index
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Projector %d", i + 1);

                            // Source
                            ImGui::TableSetColumnIndex(1);
                            if (i < scene.sources.size() && !scene.sources[i].empty()) {
                                ImGui::Text("%s", scene.sources[i].c_str());
                            } else {
                                ImGui::TextDisabled("None");
                            }

                            // Connection
                            ImGui::TableSetColumnIndex(2);
                            if (i < scene.connections.size()) {
                                if (scene.connections[i] == 1) {
                                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected");
                                } else {
                                    ImGui::Text("Disconnected");
                                }
                            }
                        }
                        ImGui::EndTable();
                    }
                }

                ImGui::EndTabItem();
            }

            // Resources Tab
            if (ImGui::BeginTabItem("Resources")) {
                auto resources = ResourceHandler::getResources();
                ImGui::Text("Total Resources: %zu", resources.size());
                ImGui::Text("Active Resource Index: %d", active_resource_index);

                if (ImGui::BeginTable("ResourcesTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("ID");
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("Path");
                    ImGui::TableSetupColumn("Thumbnail ID");
                    ImGui::TableSetupColumn("Exists");
                    ImGui::TableHeadersRow();

                    for (const auto& resource : resources) {
                        ImGui::TableNextRow();

                        // ID
                        ImGui::TableSetColumnIndex(0);
                        if (resource.id == active_resource_index) {
                            ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", resource.id);
                        } else {
                            ImGui::Text("%d", resource.id);
                        }

                        // Name
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", resource.name.c_str());

                        // Type
                        ImGui::TableSetColumnIndex(2);
                        if (resource.is_video) {
                            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Video");
                        } else {
                            ImGui::Text("Image");
                        }

                        // Path
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%s", resource.path.filename().string().c_str());
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", resource.path.string().c_str());
                        }

                        // Thumbnail ID
                        ImGui::TableSetColumnIndex(4);
                        if (resource.thumbnail_id != -1) {
                            ImGui::Text("%d", resource.thumbnail_id);
                        } else {
                            ImGui::TextDisabled("N/A");
                        }

                        // File exists
                        ImGui::TableSetColumnIndex(5);
                        bool exists = std::filesystem::exists(resource.path);
                        if (exists) {
                            ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓");
                        } else {
                            ImGui::TextColored(ImVec4(1, 0, 0, 1), "✗");
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            // Projectors Tab
            if (ImGui::BeginTabItem("Projectors")) {
                ImGui::Text("Total Projectors: %zu", projectors.size());

                // Add a combo box to select view mode
                static int viewMode = 0;
                ImGui::RadioButton("Simple View", &viewMode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Detailed View", &viewMode, 1);

                if (viewMode == 0) {
                    // Simple view - like before but enhanced
                    if (ImGui::BeginTable("ProjectorsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Index");
                        ImGui::TableSetupColumn("Status");
                        ImGui::TableSetupColumn("Running");
                        ImGui::TableSetupColumn("Resources");
                        ImGui::TableSetupColumn("Split");
                        ImGui::TableSetupColumn("Video Progress");
                        ImGui::TableSetupColumn("Actions");
                        ImGui::TableHeadersRow();

                        for (int i = 0; i < projectors.size(); i++) {
                            ImGui::TableNextRow();

                            // Index
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Projector %d", i + 1);

                            // Status
                            ImGui::TableSetColumnIndex(1);
                            if (projectors[i] != nullptr) {
                                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active");
                            } else {
                                ImGui::TextDisabled("Inactive");
                            }

                            // Running
                            ImGui::TableSetColumnIndex(2);
                            if (projectors[i] != nullptr && projectors[i]->isRunning()) {
                                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
                            } else {
                                ImGui::Text("Stopped");
                            }

                            // Resources count
                            ImGui::TableSetColumnIndex(3);
                            if (projectors[i] != nullptr) {
                                auto resources = projectors[i]->getResources();
                                ImGui::Text("%zu", resources.size());
                                if (ImGui::IsItemHovered() && !resources.empty()) {
                                    ImGui::BeginTooltip();
                                    for (const auto& res : resources) {
                                        ImGui::Text("%s", res.c_str());
                                    }
                                    ImGui::EndTooltip();
                                }
                            } else {
                                ImGui::TextDisabled("0");
                            }

                            // Split info
                            ImGui::TableSetColumnIndex(4);
                            if (projectors[i] != nullptr) {
                                SplitInfo split = projectors[i]->getSplitInfo();
                                ImGui::Text("%dx%d", split.start, split.end);
                            } else {
                                ImGui::TextDisabled("N/A");
                            }

                            // Video playback progress
                            ImGui::TableSetColumnIndex(5);
                            if (projectors[i] != nullptr && projectors[i]->isPlayingVideo()) {
                                float progress = projectors[i]->getVideoPlaybackProgress();
                                float currentTime = projectors[i]->getVideoCurrentTime();
                                float duration = projectors[i]->getVideoDuration();

                                // Progress bar
                                ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0));
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("%.1f / %.1f seconds", currentTime, duration);
                                }
                            } else {
                                ImGui::TextDisabled("Not playing");
                            }

                            // Actions
                            ImGui::TableSetColumnIndex(6);
                            if (projectors[i] != nullptr) {
                                if (ImGui::SmallButton("Stop##stop")) {
                                    // Add stop functionality
                                }
                                ImGui::SameLine();
                                if (ImGui::SmallButton("Info##info")) {
                                    // Show detailed info in popup
                                }
                            } else {
                                if (ImGui::SmallButton("Start##start")) {
                                    // Add start functionality
                                }
                            }
                        }
                        ImGui::EndTable();
                    }
                } else {
                    // Detailed view - one projector per section
                    for (int i = 0; i < projectors.size(); i++) {
                        ImGui::PushID(i);

                        if (ImGui::CollapsingHeader(("Projector " + std::to_string(i + 1)).c_str())) {
                            if (projectors[i] != nullptr) {
                                // Status
                                ImGui::SeparatorText("Status");
                                ImGui::Text("Active: %s", projectors[i]->isRunning() ? "Yes" : "No");
                                ImGui::Text("Running: %s", projectors[i]->isRunning() ? "Running" : "Stopped");

                                // Split Info
                                ImGui::SeparatorText("Split Configuration");
                                SplitInfo split = projectors[i]->getSplitInfo();
                                ImGui::Text("Start/End: %d x %d", split.start, split.end);

                                // Resources
                                ImGui::SeparatorText("Resources");
                                auto resources = projectors[i]->getResources();
                                ImGui::Text("Resource Count: %zu", resources.size());
                                if (!resources.empty()) {
                                    if (ImGui::BeginTable("ProjectorResources", 2, ImGuiTableFlags_Borders)) {
                                        ImGui::TableSetupColumn("Resource");
                                        ImGui::TableSetupColumn("Type");
                                        ImGui::TableHeadersRow();

                                        for (const auto& res : resources) {
                                            ImGui::TableNextRow();
                                            ImGui::TableSetColumnIndex(0);
                                            ImGui::Text("%s", res.c_str());
                                            ImGui::TableSetColumnIndex(1);
                                            // You might want to add resource type detection here
                                            if (res.find(".mp4") != std::string::npos ||
                                                res.find(".avi") != std::string::npos) {
                                                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Video");
                                            } else {
                                                ImGui::Text("Image");
                                            }
                                        }
                                        ImGui::EndTable();
                                    }
                                }

                                // Video Playback Info
                                if (projectors[i]->isPlayingVideo()) {
                                    ImGui::SeparatorText("Video Playback");
                                    float progress = projectors[i]->getVideoPlaybackProgress();
                                    float currentTime = projectors[i]->getVideoCurrentTime();
                                    float duration = projectors[i]->getVideoDuration();

                                    // Progress bar with time
                                    ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 20));
                                    ImGui::Text("Time: %.1f / %.1f seconds (%.1f%%)",
                                               currentTime, duration, progress * 100);

                                    // Time controls (if you want to add them)
                                    if (ImGui::Button("Pause")) {
                                        // projectors[i]->pauseVideo();
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::Button("Resume")) {
                                        // projectors[i]->resumeVideo();
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::Button("Restart")) {
                                        // projectors[i]->restartVideo();
                                    }
                                }

                                // Actions
                                ImGui::SeparatorText("Actions");
                                if (ImGui::Button("Stop Projector")) {
                                    // projectors[i]->stop();
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Restart Projector")) {
                                    // projectors[i]->restart();
                                }
                            } else {
                                ImGui::Text("Projector is not initialized");
                                if (ImGui::Button("Initialize Projector")) {
                                    // Initialize projector[i]
                                }
                            }
                        }

                        ImGui::PopID();
                    }
                }

                ImGui::EndTabItem();
            }

            // System Tab
            if (ImGui::BeginTabItem("System")) {
                ImGui::SeparatorText("Window Info");
                int width, height;
                glfwGetWindowSize(Backend::GetWindow(), &width, &height);
                ImGui::Text("Window Size: %d x %d", width, height);
                ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

                ImGui::SeparatorText("Paths");
                ImGui::Text("Project Path: %s", current_project_path.string().c_str());

                ImGui::SeparatorText("Flags");
                ImGui::Checkbox("Create Recent Path", &create_recent_path);
                ImGui::Text("Active Scene Index: %d", active_scene_index);
                ImGui::Text("Active Resource Index: %d", active_resource_index);

                ImGui::SeparatorText("Refresh");
                if (ImGui::Button("Refresh Scenes")) {
                    UIManager::refreshScenes();
                }
                if (ImGui::Button("Refresh Resources")) {
                    UIManager::refreshResourcePanel();
                }
                if (ImGui::Button("Refresh Projectors")) {
                    UIManager::refreshProjectors();
                }

                ImGui::SeparatorText("Popups");
                if (ImGui::Button("Create Debug Popup small")) {
                    Utilities::showInfo("Test message");
                }
                if (ImGui::Button("Create Debug Popup big")) {
                    Utilities::showInfo("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");
                }
                if (ImGui::Button("Create Debug Error Popup small")){
                    Utilities::showError("Test message");
                }
                if (ImGui::Button("Create Debug Error Popup big")) {
                    Utilities::showError("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
    // ====== END DEBUG WINDOW CODE ======

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
        // You might want to block RmlUi input when ImGui is capturing
    }
#endif
    Backend::PresentFrame();
    glfwMakeContextCurrent(Backend::GetWindow());
}
