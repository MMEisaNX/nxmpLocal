#include "netflixUI.h"
#include "folderPicker.h"
#include "gui.h"
#include "appwindows.h"
#include "localLibrary.h"
#include "iniparser.h"
#include "SQLiteDB.h"
#include "logger.h"

#include <cmath>

namespace Windows {

static int activeCategoryIdx = 0;
static int focusedItemIdx = 0;
static bool netflixUIInitialized = false;

void InitNetflixUI() {
    if (!localLibManager) {
        localLibManager = new LocalLib::LocalLibraryManager();
    }
    localLibManager->setRootPath(configini->getStartPath());
    localLibManager->scanLibrary();
    activeCategoryIdx = 0;
    focusedItemIdx = 0;
    netflixUIInitialized = true;
}

void NetflixUIWindow(bool *focus, bool *first_item) {
    if (!netflixUIInitialized) {
        InitNetflixUI();
    }

    Windows::SetupWindow();

    // Top status indicators (clock & battery)
    GUI::cloktimeText(ImVec2((1180.0f * multiplyRes) - ImGui::CalcTextSize(nxmpstats->currentTime).x - (10.0f * multiplyRes), 5.0f * multiplyRes), true, nxmpstats->currentTime);
    GUI::newbatteryIcon(ImVec2(1180.0f * multiplyRes, 5.0f * multiplyRes), true, batteryPercent, 40 * multiplyRes, 20 * multiplyRes, true);

    auto &categories = localLibManager->getCategories();

    // If no categories or videos found, show a clean prompt to pick a folder
    if (categories.empty()) {
        if (ImGui::Begin("NXMP Local", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::SetCursorPosY(180.0f * multiplyRes);
            ImGui::SetCursorPosX((1280.0f * multiplyRes - ImGui::CalcTextSize("No local media found in current path:").x) / 2.0f);
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "No local media found in current path:");

            std::string curPath = configini->getStartPath();
            ImGui::SetCursorPosX((1280.0f * multiplyRes - ImGui::CalcTextSize(curPath.c_str()).x) / 2.0f);
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%s", curPath.c_str());

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.89f, 0.04f, 0.08f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.15f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.05f, 1.0f));

            float btnW = 320.0f * multiplyRes;
            float btnH = 50.0f * multiplyRes;
            ImGui::SetCursorPosX((1280.0f * multiplyRes - btnW) / 2.0f);
            if (ImGui::Button(" Select SD Library Folder ", ImVec2(btnW, btnH))) {
                InitFolderPicker(configini->getStartPath());
                item.state = MENU_STATE_FOLDER_PICKER;
            }
            ImGui::PopStyleColor(3);
        }
        Windows::ExitWindow();
        return;
    }

    if (activeCategoryIdx >= static_cast<int>(categories.size())) {
        activeCategoryIdx = 0;
    }

    auto &currentCat = categories[activeCategoryIdx];

    if (ImGui::Begin("NXMP Netflix", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        
        // -------------------------------------------------------------
        // 1. TOP HEADER BAR: Netflix Brand, Categories, Actions
        // -------------------------------------------------------------
        ImGui::SetCursorPos(ImVec2(20.0f * multiplyRes, 8.0f * multiplyRes));
        // Bold Red Netflix-style logo
        ImGui::TextColored(ImVec4(0.89f, 0.04f, 0.08f, 1.0f), "NXMP");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "LOCAL");

        // Action buttons on top-right
        ImGui::SameLine(750.0f * multiplyRes);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * multiplyRes);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.89f, 0.04f, 0.08f, 0.9f));

        if (ImGui::Button(" 📁 Folder ", ImVec2(100.0f * multiplyRes, 28.0f * multiplyRes))) {
            InitFolderPicker(configini->getStartPath());
            item.state = MENU_STATE_FOLDER_PICKER;
        }

        ImGui::SameLine();
        if (ImGui::Button(" ⚙ Settings ", ImVec2(100.0f * multiplyRes, 28.0f * multiplyRes))) {
            item.state = MENU_STATE_SETTINGS;
            settingsview_open = true;
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        // Category Tabs Bar
        ImGui::SetCursorPosY(42.0f * multiplyRes);
        ImGui::SetCursorPosX(20.0f * multiplyRes);

        for (int c = 0; c < static_cast<int>(categories.size()); c++) {
            if (c > 0) ImGui::SameLine(0, 15.0f * multiplyRes);

            bool isSelected = (c == activeCategoryIdx);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.89f, 0.04f, 0.08f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.14f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            }
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.89f, 0.04f, 0.08f, 0.7f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f * multiplyRes);

            std::string catLabel = "  " + categories[c].name + " (" + std::to_string(categories[c].items.size()) + ")  ";
            if (ImGui::Button(catLabel.c_str(), ImVec2(0, 28.0f * multiplyRes))) {
                if (activeCategoryIdx != c) {
                    localLibManager->releaseCategoryTextures(categories[activeCategoryIdx], Renderer);
                    activeCategoryIdx = c;
                    focusedItemIdx = 0;
                }
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
        }

        ImGui::SetCursorPosY(76.0f * multiplyRes);
        ImGui::Separator();

        // -------------------------------------------------------------
        // 2. HERO SPOTLIGHT BANNER: Featured / Hovered Item Info
        // -------------------------------------------------------------
        float heroH = 175.0f * multiplyRes;
        ImGui::BeginChild("##HeroSpotlight", ImVec2(1240.0f * multiplyRes, heroH), false, ImGuiWindowFlags_NoScrollbar);

        if (!currentCat.items.empty() && focusedItemIdx < static_cast<int>(currentCat.items.size())) {
            auto &fItem = currentCat.items[focusedItemIdx];
            
            // Ensure poster texture is loaded
            localLibManager->loadItemTexture(fItem, imgloader);

            // Left side: Poster Thumbnail
            float pWidth = 115.0f * multiplyRes;
            float pHeight = 160.0f * multiplyRes;

            ImVec2 imgPos = ImGui::GetCursorScreenPos();
            if (fItem.textureLoaded && fItem.posterTexture.id != static_cast<DkResHandle>(-1)) {
                ImGui::Image((void*)(intptr_t)fItem.posterTexture.id, ImVec2(pWidth, pHeight));
            } else {
                // Stylish fallback placeholder
                ImDrawList *draw = ImGui::GetWindowDrawList();
                draw->AddRectFilled(imgPos, ImVec2(imgPos.x + pWidth, imgPos.y + pHeight), IM_COL32(35, 35, 40, 255), 6.0f);
                draw->AddRect(imgPos, ImVec2(imgPos.x + pWidth, imgPos.y + pHeight), IM_COL32(80, 80, 90, 255), 6.0f);
                draw->AddText(ImVec2(imgPos.x + 15.0f * multiplyRes, imgPos.y + pHeight / 2.0f - 10.0f * multiplyRes), IM_COL32(200, 200, 200, 255), "▶ VIDEO");
                ImGui::Dummy(ImVec2(pWidth, pHeight));
            }

            // Right side: Info and Play button
            ImGui::SameLine(0, 25.0f * multiplyRes);
            ImGui::BeginGroup();
            
            // Category badge
            ImGui::TextColored(ImVec4(0.89f, 0.04f, 0.08f, 1.0f), "CATEGORY: %s", currentCat.name.c_str());
            
            // Large Title
            ImGui::SetWindowFontScale(1.25f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", fItem.title.c_str());
            ImGui::SetWindowFontScale(1.0f);

            // File size & Type
            ImGui::Spacing();
            std::string sizeStr = LocalLib::LocalLibraryManager::formatFileSize(fItem.fileSize);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Size: %s  |  Type: %s", sizeStr.c_str(), fItem.isDirectory ? "Folder / Series" : "Direct Video");

            // Play Button
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.89f, 0.04f, 0.08f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.02f, 0.05f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * multiplyRes);

            if (ImGui::Button("  ▶  PLAY NOW  ", ImVec2(180.0f * multiplyRes, 38.0f * multiplyRes))) {
                if (!fItem.videoPath.empty()) {
                    if (filebrowser) filebrowser->clearChecked();
                    libmpv->loadFile(fItem.videoPath);
                    if (configini->getDbActive(true) && sqlitedb) {
                        int resumeTime = sqlitedb->getResume(fItem.videoPath);
                        if (resumeTime > 0) {
                            libmpv->getFileInfo()->resume = resumeTime;
                            item.popupstate = POPUP_STATE_RESUME;
                        }
                    }
                }
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            ImGui::EndGroup();
        } else {
            ImGui::SetCursorPosY(50.0f * multiplyRes);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No items in this category.");
        }
        ImGui::EndChild();

        ImGui::Separator();

        // -------------------------------------------------------------
        // 3. POSTER GRID (Scrollable card gallery)
        // -------------------------------------------------------------
        float gridH = 400.0f * multiplyRes;
        ImGui::BeginChild("##PosterGrid", ImVec2(1240.0f * multiplyRes, gridH), true);

        float cardW = 145.0f * multiplyRes;
        float cardH = 205.0f * multiplyRes;
        float cardSpacingX = 18.0f * multiplyRes;
        float cardSpacingY = 22.0f * multiplyRes;

        float availW = ImGui::GetContentRegionAvail().x;
        int cols = static_cast<int>(availW / (cardW + cardSpacingX));
        if (cols < 1) cols = 1;

        for (size_t i = 0; i < currentCat.items.size(); i++) {
            auto &itemRef = currentCat.items[i];
            localLibManager->loadItemTexture(itemRef, imgloader);

            int col = i % cols;
            if (col > 0) {
                ImGui::SameLine(0, cardSpacingX);
            } else if (i > 0) {
                ImGui::Dummy(ImVec2(0, cardSpacingY));
            }

            ImGui::PushID(static_cast<int>(i));
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            bool isFocused = (focusedItemIdx == static_cast<int>(i));

            // Selectable card backing for navigation
            bool selected = ImGui::Selectable("##card", isFocused, 0, ImVec2(cardW, cardH + 28.0f * multiplyRes));
            if (ImGui::IsItemHovered() || ImGui::IsItemFocused()) {
                focusedItemIdx = static_cast<int>(i);
            }

            if (selected) {
                if (!itemRef.videoPath.empty()) {
                    if (filebrowser) filebrowser->clearChecked();
                    libmpv->loadFile(itemRef.videoPath);
                    if (configini->getDbActive(true) && sqlitedb) {
                        int resumeTime = sqlitedb->getResume(itemRef.videoPath);
                        if (resumeTime > 0) {
                            libmpv->getFileInfo()->resume = resumeTime;
                            item.popupstate = POPUP_STATE_RESUME;
                        }
                    }
                }
            }

            // Draw Poster Image or Fallback Card
            ImDrawList *draw = ImGui::GetWindowDrawList();
            ImVec2 p0 = screenPos;
            ImVec2 p1 = ImVec2(p0.x + cardW, p0.y + cardH);

            if (itemRef.textureLoaded && itemRef.posterTexture.id != static_cast<DkResHandle>(-1)) {
                draw->AddImageRounded((void*)(intptr_t)itemRef.posterTexture.id, p0, p1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 6.0f);
            } else {
                // Modern dark placeholder
                draw->AddRectFilled(p0, p1, IM_COL32(28, 28, 32, 255), 6.0f);
                draw->AddRect(p0, p1, IM_COL32(50, 50, 58, 255), 6.0f);
                std::string shortTitle = itemRef.title.length() > 14 ? (itemRef.title.substr(0, 12) + "..") : itemRef.title;
                draw->AddText(ImVec2(p0.x + 8.0f * multiplyRes, p0.y + cardH / 2.0f - 8.0f * multiplyRes), IM_COL32(190, 190, 200, 255), shortTitle.c_str());
            }

            // Highlight border if focused (Netflix Red glow)
            if (isFocused) {
                draw->AddRect(ImVec2(p0.x - 2.0f, p0.y - 2.0f), ImVec2(p1.x + 2.0f, p1.y + 2.0f), IM_COL32(229, 9, 20, 255), 6.0f, 0, 3.5f);
            }

            // Title beneath the card
            std::string displayTitle = itemRef.title;
            if (displayTitle.length() > 16) {
                displayTitle = displayTitle.substr(0, 14) + "..";
            }
            draw->AddText(ImVec2(p0.x + 2.0f, p1.y + 4.0f * multiplyRes), isFocused ? IM_COL32(255, 255, 255, 255) : IM_COL32(170, 170, 170, 255), displayTitle.c_str());

            ImGui::PopID();
        }

        ImGui::EndChild();

        // -------------------------------------------------------------
        // 4. FOOTER CONTROLS BAR (Gamepad Guides)
        // -------------------------------------------------------------
        ImGui::SetCursorPosY(720.0f * multiplyRes - 30.0f * multiplyRes);
        ImGui::SetCursorPosX(20.0f * multiplyRes);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(A) Play   (B) Back   (L/R) Switch Category   (Y) Change Folder   (X) Settings");
    }

    Windows::ExitWindow();
}

} // namespace Windows
