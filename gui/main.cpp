#include "board.hpp"
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"
#include "textures.h"

#include <memory>
/*
 * Creates an invisible window to which the other imgui windows
 * can dock.
 */
void drawViewport() {
    // Fullscreen invisible window for DockSpace
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostWindowFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    // Remove all window padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpaceHost", nullptr, hostWindowFlags);
    ImGuiID dockspaceID = ImGui::GetID("Dockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
    ImGui::PopStyleVar(); // Restore original padding
}
int main() {
    int screenWidth = 800, screenHeight = 600;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Gideon's Chess Engine (v0.2.0)");
    loadPieceTextures();
    SetTargetFPS(60);

    rlImGuiSetup(true);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    auto board = std::make_unique<Board>();
    ImVec4 imguiBgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    Color bgColor = {static_cast<unsigned char>(imguiBgColor.x * 255.0f),
                     static_cast<unsigned char>(imguiBgColor.y * 255.0f),
                     static_cast<unsigned char>(imguiBgColor.z * 255.0f),
                     static_cast<unsigned char>(imguiBgColor.w * 255.0f)};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bgColor);
        rlImGuiBegin();
        drawViewport();
        board->render();
        rlImGuiEnd();
        EndDrawing();
    }

    unloadPieceTextures();
}