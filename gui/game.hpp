#ifndef GAME_HPP
#define GAME_HPP
#include "board.hpp"
#include "info.hpp"
#include "position.h"
#include "rlImGui.h"
#include "textures.h"
#include "window.hpp"

#include <memory>
#include <vector>

// forward declarations
void drawViewport();

class Game {
  public:
    Game() {
        auto raw = position_from_fen(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        if (!raw) {
            std::exit(1);
        }
        position = std::make_unique<Position>(*raw);
        setup();
        windows.push_back(std::make_unique<Board>(*position));
        windows.push_back(std::make_unique<InfoWindow>(*position));
    }

    void setup() {
        int screenWidth = 800, screenHeight = 600;
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
        InitWindow(screenWidth, screenHeight, "Gideon's Chess Engine (v0.2.0)");
        SetTargetFPS(60);
        loadPieceTextures();

        rlImGuiSetup(true);
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImVec4 imguiBgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        bgColor = {static_cast<unsigned char>(imguiBgColor.x * 255.0f),
                   static_cast<unsigned char>(imguiBgColor.y * 255.0f),
                   static_cast<unsigned char>(imguiBgColor.z * 255.0f),
                   static_cast<unsigned char>(imguiBgColor.w * 255.0f)};
    }

    void render() {
        BeginDrawing();
        ClearBackground(bgColor);
        rlImGuiBegin();
        drawViewport();
        for (auto &window : windows) {
            window->render();
        }
        rlImGuiEnd();
        EndDrawing();
    }

  private:
    std::unique_ptr<Position> position;
    std::vector<std::unique_ptr<GuiWindow>> windows;
    Color bgColor;
};

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

#endif // GAME_HPP
