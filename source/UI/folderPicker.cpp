#include "folderPicker.h"
#include "gui.h"
#include "appwindows.h"
#include "localLibrary.h"
#include "iniparser.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>

namespace Windows {

static std::string currentPickerPath = "/";
static std::vector<std::string> currentSubdirs;
static bool pickerInitialized = false;

static void RefreshPickerDirectories() {
    currentSubdirs.clear();
    DIR *dir = opendir(currentPickerPath.c_str());
    if (!dir) {
        // Fallback to root if path failed
        currentPickerPath = "/";
        dir = opendir(currentPickerPath.c_str());
    }

    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_name[0] == '.') continue;
            std::string full = currentPickerPath == "/" ? ("/" + std::string(ent->d_name)) : (currentPickerPath + "/" + ent->d_name);
            struct stat st;
            if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                currentSubdirs.push_back(ent->d_name);
            }
        }
        closedir(dir);
    }
    std::sort(currentSubdirs.begin(), currentSubdirs.end());
}

void InitFolderPicker(const std::string &initialPath) {
    if (!initialPath.empty()) {
        currentPickerPath = initialPath;
    } else {
        currentPickerPath = "/";
    }
    RefreshPickerDirectories();
    pickerInitialized = true;
}

void FolderPickerWindow(bool *focus, bool *first_item) {
    if (!pickerInitialized) {
        InitFolderPicker(configini->getStartPath());
    }

    Windows::SetupWindow();

    // Top status (clock & battery)
    GUI::cloktimeText(ImVec2((1180.0f * multiplyRes) - ImGui::CalcTextSize(nxmpstats->currentTime).x - (10.0f * multiplyRes), 2.0f * multiplyRes), true, nxmpstats->currentTime);
    GUI::newbatteryIcon(ImVec2(1180.0f * multiplyRes, 2.0f * multiplyRes), true, batteryPercent, 40 * multiplyRes, 20 * multiplyRes, true);

    std::string title = "Select Video Library Folder (SD Card)";
    if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        
        // Header instructions
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Choose the main folder containing your videos/categories (e.g. Movies, Anime, Lectures)");
        ImGui::Spacing();

        // Current Path display
        ImGui::Text("Current Path: %s", currentPickerPath.c_str());
        ImGui::Spacing();

        // Selection Action Buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.1f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.05f, 0.1f, 1.0f));

        if (ImGui::Button(" [ Select This Folder as Media Library ] ", ImVec2(400.0f * multiplyRes, 45.0f * multiplyRes))) {
            configini->setStartPath(currentPickerPath);
            if (localLibManager) {
                localLibManager->setRootPath(currentPickerPath);
                localLibManager->scanLibrary();
            }
            item.state = MENU_STATE_NETFLIX;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f * multiplyRes, 45.0f * multiplyRes))) {
            if (localLibManager && localLibManager->hasCategories()) {
                item.state = MENU_STATE_NETFLIX;
            } else {
                item.state = MENU_STATE_HOME;
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Directory list child window
        float listHeight = ImGui::GetContentRegionAvail().y - 20.0f * multiplyRes;
        ImGui::BeginChild("##dirlist", ImVec2(0, listHeight), true);

        // Parent directory ".." option if not at root
        if (currentPickerPath != "/" && !currentPickerPath.empty()) {
            if (ImGui::Selectable(".. (Up One Folder)", false, 0, ImVec2(0, 36.0f * multiplyRes))) {
                size_t lastSlash = currentPickerPath.find_last_of('/');
                if (lastSlash == 0) {
                    currentPickerPath = "/";
                } else if (lastSlash != std::string::npos) {
                    currentPickerPath = currentPickerPath.substr(0, lastSlash);
                }
                RefreshPickerDirectories();
            }
        }

        // Subdirectories
        for (size_t i = 0; i < currentSubdirs.size(); i++) {
            std::string label = "[DIR] " + currentSubdirs[i];
            if (ImGui::Selectable(label.c_str(), false, 0, ImVec2(0, 36.0f * multiplyRes))) {
                if (currentPickerPath == "/") {
                    currentPickerPath = "/" + currentSubdirs[i];
                } else {
                    currentPickerPath = currentPickerPath + "/" + currentSubdirs[i];
                }
                RefreshPickerDirectories();
            }
        }

        if (currentSubdirs.empty() && currentPickerPath == "/") {
            ImGui::TextDisabled("No subdirectories found.");
        }

        ImGui::EndChild();
    }

    Windows::ExitWindow();
}

} // namespace Windows
