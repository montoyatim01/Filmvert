#include "window.h"
#include <imgui.h>

//--- Shortcuts Popup ---//
/*
    Popup for displaying all available
    hotkeys in the application.
*/
void mainWindow::shortcutsPopup() {
    if (shortPopTrig)
        ImGui::OpenPopup("Keyboard Shortcuts");

    // Pin width to 420, let height auto-fit content
    ImGui::SetNextWindowSizeConstraints(ImVec2(420, 0), ImVec2(420, FLT_MAX));
    if (ImGui::BeginPopupModal("Keyboard Shortcuts", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        std::string s_cmd = "";
        std::string s_opt = "";
        #ifdef __APPLE__
        s_cmd = "⌘";
        s_opt = "⌥";
        #else
        s_cmd = "Ctrl";
        s_opt = "Alt";
        #endif

        // Helper: one shortcut row — label left, shortcut right-aligned
        auto row = [&](const char* label, const std::string& shortcut) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label);
            ImGui::TableSetColumnIndex(1);
            float w = ImGui::CalcTextSize(shortcut.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - w);
            ImGui::Text("%s", shortcut.c_str());
        };

        // Helper: separator spanning both columns
        auto sep = [&]() {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Separator();
            ImGui::TableSetColumnIndex(1);
            ImGui::Separator();
        };

        constexpr ImGuiTableFlags tFlags = ImGuiTableFlags_SizingFixedFit;
        constexpr float kRightColW = 160.0f;

        if (ImGui::BeginTabBar("ShortcutsTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("General")) {
                if (ImGui::BeginTable("general_sc", 2, tFlags)) {
                    ImGui::TableSetupColumn("label",    ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("shortcut", ImGuiTableColumnFlags_WidthFixed, kRightColW);

                    row("Open Image(s)",           s_cmd + " + O");
                    row("Open Roll(s)",            s_cmd + " + Shift + O");
                    sep();
                    row("Save Image",              s_cmd + " + S");
                    row("Save Roll",               s_cmd + " + Shift + S");
                    row("Save All Rolls",          s_opt + " + Shift + S");
                    sep();
                    row("Close Selected Image(s)", s_cmd + " + W");
                    row("Close Roll",              s_cmd + " + Shift + W");
                    sep();
                    row("Undo",                    s_cmd + " + Z");
                    row("Redo",                    s_cmd + " + Shift + Z");
                    sep();
                    row("Select All",              s_cmd + " + A");
                    row("Copy",                    s_cmd + " + C");
                    row("Paste",                   s_cmd + " + V");

                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Viewer")) {
                if (ImGui::BeginTable("viewer_sc", 2, tFlags)) {
                    ImGui::TableSetupColumn("label",    ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("shortcut", ImGuiTableColumnFlags_WidthFixed, kRightColW);

                    row("Reset view to fit image",          "H");
                    row("Zoom in/out of image",             s_opt + " + Scroll");
                    row("Pan image",                        "Shift + Scroll");
                    sep();
                    row("Previous Image",                   s_opt + " + Left Arrow");
                    row("Next Image",                       s_opt + " + Right Arrow");
                    sep();
                    row("Refresh Image",                    s_cmd + " + R");
                    row("Toggle Base Color Selection",      s_cmd + " + B");
                    sep();
                    row("Isolate a color channel", "R, G, B");
                    row("Show clipping", "K");
                    row("Disable grades", "D");

                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Image")) {
                if (ImGui::BeginTable("image_sc", 2, tFlags)) {
                    ImGui::TableSetupColumn("label",    ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("shortcut", ImGuiTableColumnFlags_WidthFixed, kRightColW);

                    row("Edit Image Metadata", s_cmd + " + E");
                    row("Edit Roll Metadata",  s_cmd + " + G");
                    sep();
                    row("Rate Image 0-5",      s_opt + " + Num");
                    sep();
                    row("Rotate Left",         s_cmd + " + [");
                    row("Rotate Right",        s_cmd + " + ]");
                    row("Flip Horizontally",   s_opt + " + H");
                    row("Flip Vertically",     s_opt + " + V");

                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Okay")) {
            shortPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            shortPopTrig = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}
