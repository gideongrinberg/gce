#include "game.hpp"

#include "assets.h"
#include "board.hpp"
#include "info.hpp"
#include "modals.hpp"
#include "rlImGui.h"
#include "textures.h"

#include <memory>
#include <vector>

Game::Game(int width, int height)
    : width(width), height(height), state(NEW_GAME) {
    position = *position_from_fen(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    setup();
    windows.push_back(std::make_unique<Board>(*this));
    windows.push_back(std::make_unique<InfoWindow>(*this));
    windows.push_back(std::make_unique<GameModal>(*this));
#ifndef NDEBUG
    showPst = -1;
#endif
}

void Game::setup() {
#ifndef EMSCRIPTEN
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT |
                   FLAG_WINDOW_HIGHDPI);
#else
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
#endif
    InitWindow(width, height, "Gideon's Chess Engine (v0.2.0)");
#ifndef EMSCRIPTEN
    SetTargetFPS(60);
#endif
    loadPieceTextures();

    rlImGuiSetup(true);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::LoadIniSettingsFromMemory(reinterpret_cast<const char *>(layout_ini),
                                     layout_ini_len);

    ImVec4 imguiBgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    bgColor = {static_cast<unsigned char>(imguiBgColor.x * 255.0f),
               static_cast<unsigned char>(imguiBgColor.y * 255.0f),
               static_cast<unsigned char>(imguiBgColor.z * 255.0f),
               static_cast<unsigned char>(imguiBgColor.w * 255.0f)};
}

void Game::render() {
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

// draw the viewport and menu bar
void Game::drawViewport() {
    ImGuiIO &io = ImGui::GetIO();
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostWindowFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpaceHost", nullptr, hostWindowFlags);
    ImGuiID dockspaceID = ImGui::GetID("Dockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f),
                     ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Game")) {
            if (ImGui::MenuItem("New game")) {
                state = NEW_GAME;
            }
            ImGui::EndMenu();
        }
#ifndef NDEBUG
        if (ImGui::BeginMenu("Debug")) {
            if (ImGui::MenuItem("Save layout")) {
                ImGui::SaveIniSettingsToDisk("layout.ini");
            }

            if (ImGui::BeginMenu("Display PST")) {
                std::array<std::string, 2> colors = {"White", "Black"};
                std::array<std::string, 6> pieces = {"Pawn", "Knight", "Bishop",
                                                     "Rook", "Queen",  "King"};

                static int selectedPiece = -1, selectedColor = -1;
                for (int i = 0; i < colors.size(); i++) {
                    if (ImGui::MenuItem(colors[i].c_str(), nullptr,
                                        selectedColor == i)) {
                        selectedColor = i;
                    }
                }

                ImGui::Separator();

                for (int i = 0; i < pieces.size(); i++) {
                    if (ImGui::MenuItem(pieces[i].c_str(), nullptr,
                                        selectedPiece == i)) {
                        selectedPiece = i;
                    }
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Reset")) {
                    selectedPiece = -1;
                    selectedColor = -1;
                    showPst = -1;
                } else if (selectedPiece != -1 && selectedColor != -1) {
                    showPst = (selectedColor * 8) | selectedPiece;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
#endif
        ImGui::EndMainMenuBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}