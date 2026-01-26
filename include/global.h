#pragma once

// std
#include <string>
#include <vector>
#include <utility>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <set>
#include <fstream>
#include <iostream>
#include <chrono>

// Json
#include <nlohmann/json.hpp>

// Windows explorer
#include <shobjidl.h>
#include <windows.h>
#include <shlobj.h>

// Local
#include "Shell.h"
#include "Window.h"
#include "Projector.h"

using json = nlohmann::json;
namespace fs = std::filesystem;
/*
 * main:
 *  - render loop
 *  - starts UISetup
 *  UISetup:
 *  - general setup functions for entire UI
 *  - uses UIManager functions
 *  UIManager:
 *  - some remaining or dynamic eventlisteners
 *  - more complicated initialization logic
 *  - refresh methods
 *  - uses Utility function
 *  - uses ResourceHandler
 *  - uses EventListener.h
 *  Utilities:
 *  - general utility functions
 *  ResourceHandler:
 *  - access to project resources
 *  global.h:
 *  - globally used variables
 *  - currently loaded save
 *  - globally used structs
 *  EventListener.h
 *  - clickhandlers for context menus etc.
 */

// General Project data
struct st_save_data {
    std::string name;
    int projector_amount = -1; // [1-6] //TODO: check for -1 error values
    std::string description;
    std::filesystem::path path; // where project is located
    std::string version; // e.g. 1.2.3, (also in VERSION const)
};

// Data per scene
struct st_scene_data {
    std::string name;
    std::vector<std::string> sources; // [0] would be the path of the resource of the first projector //TODO: use ID //TODO: make array
    std::vector<std::string> split_sources; // for displaying split sources in the preview //TODO: replace by rendering half textures //TODO: make array
    std::vector<SplitInfo> split_info; // for projectors //TODO: make array
    std::vector<int> connections; //TODO: make array

    st_scene_data() = default;

    st_scene_data(std::string  name, std::vector<std::string> src)
        : name(std::move(name)), sources(std::move(src)) {}
};

// TODO: replace with vector in ST_saveData
struct st_scene_manager {
    std::vector<st_scene_data> scenes;
};

// Get main app window //TODO: make utility function
inline Window& get_window() {
    static Window window(1920,1080);
    return window;
}

// Shared Functions from Eventlisteners //TODO: must be possible in a better way, maybe add the functions directly to like ST_Scene_Data
// TODO: fix from indices to IDs
// TODO: for consitency make it use eiter rename and show rename dialog in scene and resources or not in both
// Scene context menu
void rename_scene(int index);
void delete_scene(int index);
void duplicate_scene(int index);
void select_scene(int index);
void show_scene_rename_dialog(int index);

// Resource context menu
void show_resource_rename_dialog(int id);
void delete_resource(int id);
void select_resource(int index);

// Projectors
void connect_projectors(int index);
void disconnect_projectors(int index);
void show_projector_resource_selection(int index);

// For switching in UI helper functions //TODO: could be possible in a better way
void set_startup_event_listeners();
void set_interface_event_listeners();

// Global Variables
inline st_save_data save_data;
inline st_scene_manager scene_manager;
inline bool create_recent_path = false; // creates a recent.path file in the saves dir so that the last used project is opened automatically when the program is restared
inline fs::path current_project_path; // path of currently loaded path //TODO: note if that is with proj name or not, i guess not but find out

inline int active_scene_index = 0; // index of selected scene
inline int active_resource_index = -1; // -1 means no resource selected //TODO: is resource selection necessary?

inline std::vector<Projector*> projectors; // currently active projectors


