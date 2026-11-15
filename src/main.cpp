#include "raylib.h"
#include "games_types.h"
#include "save_manager.h"
#include "comitee.h"
#include "lab_shop.h"

#include <algorithm>
#include <string>
#include <vector>

struct Star {
    float x;
    float y;
    float size;
    unsigned char alpha;
};

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MOONSHOT · NASA Artemis & Apollo Engineering");
    LoadGameFont();
    SetTargetFPS(60);

    SharedMissionData missionData{};
    missionData.Reset();
    missionData.lang = Language::ES;
    missionData.activeSaveSlot = 1;

    int selectedSlot = 1;
    GameState currentState = GameState::MENU;

    CommitteePhase committee;
    committee.Init(missionData.lang);

    LabShopPhase labShop;
    labShop.Init();

    std::vector<Star> stars;
    stars.reserve(140);
    for (int i = 0; i < 140; ++i) {
        stars.push_back({
            static_cast<float>((i * 197 + 43) % SCREEN_WIDTH),
            static_cast<float>((i * 311 + 89) % SCREEN_HEIGHT),
            (i % 3 == 0) ? 2.5f : 1.5f,
            static_cast<unsigned char>(120 + (i * 29) % 135)
        });
    }

    auto launchSlot = [&](int slot) {
        Language keepLang = missionData.lang;
        SaveSlotSummary summary = SaveManager::GetSlotSummary(slot);

        if (summary.occupied) {
            GameState loadedPhase = GameState::COMMITTEE;
            if (SaveManager::LoadGame(slot, missionData, loadedPhase)) {
                missionData.lang = keepLang;
                missionData.activeSaveSlot = slot;
                if (loadedPhase == GameState::HANGAR) {
                    labShop.RestoreFromSave(missionData);
                    currentState = GameState::HANGAR;
                } else if (loadedPhase == GameState::FLIGHT) {
                    labShop.RestoreFromSave(missionData);
                    currentState = GameState::FLIGHT;
                } else {
                    committee.Init(missionData.lang);
                    currentState = GameState::COMMITTEE;
                }
            }
        } else {
            missionData.Reset();
            missionData.lang = keepLang;
            missionData.activeSaveSlot = slot;
            committee.Init(missionData.lang);
            currentState = GameState::COMMITTEE;
        }
    };

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // ==================== UPDATE ====================
        if (currentState == GameState::MENU) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                // Boton discreto de idioma arriba a la izquierda
                Rectangle btnLang = { 24.0f, 20.0f, 115.0f, 34.0f };
                if (CheckCollisionPointRec(mouse, btnLang)) {
                    missionData.lang = (missionData.lang == Language::ES) ? Language::EN : Language::ES;
                }

                // Ranuras a la izquierda
                for (int slot = 1; slot <= 3; ++slot) {
                    int idx = slot - 1;
                    Rectangle slotBox = { 90.0f, 225.0f + idx * 130.0f, 690.0f, 112.0f };
                    Rectangle delBtn  = { slotBox.x + slotBox.width - 110.0f, slotBox.y + 62.0f, 92.0f, 34.0f };

                    SaveSlotSummary summary = SaveManager::GetSlotSummary(slot);

                    if (summary.occupied && CheckCollisionPointRec(mouse, delBtn)) {
                        SaveManager::DeleteSlot(slot);
                        selectedSlot = slot;
                        break;
                    }

                    if (CheckCollisionPointRec(mouse, slotBox)) {
                        selectedSlot = slot;
                        launchSlot(slot);
                        break;
                    }
                }

                // Panel derecho: boton grande NUEVA PARTIDA / CONTINUAR PARTIDA
                Rectangle rightMainBtn = { 835.0f, 465.0f, 330.0f, 56.0f };
                Rectangle rightDelBtn  = { 835.0f, 535.0f, 330.0f, 40.0f };
                SaveSlotSummary selSummary = SaveManager::GetSlotSummary(selectedSlot);

                if (CheckCollisionPointRec(mouse, rightMainBtn)) {
                    launchSlot(selectedSlot);
                } else if (selSummary.occupied && CheckCollisionPointRec(mouse, rightDelBtn)) {
                    SaveManager::DeleteSlot(selectedSlot);
                }
            }
        } else if (currentState == GameState::COMMITTEE) {
            GameState next = committee.Update(missionData);
            if (next == GameState::HANGAR) {
                labShop.OnEnter(missionData);
                SaveManager::SaveGame(missionData.activeSaveSlot, missionData, GameState::HANGAR);
            }
            currentState = next;
        } else if (currentState == GameState::HANGAR) {
            GameState next = labShop.Update(missionData);
            if (next == GameState::FLIGHT) {
                // Auto-Save obligatorio antes de pasar a la Fase 3
                SaveManager::SaveGame(missionData.activeSaveSlot, missionData, GameState::FLIGHT);
            }
            currentState = next;
        } else if (currentState == GameState::FLIGHT) {
            if (IsKeyPressed(KEY_M)) currentState = GameState::MENU;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Rectangle btnBackHangar = { 310.0f, 545.0f, 310.0f, 50.0f };
                Rectangle btnBackMenu   = { 660.0f, 545.0f, 310.0f, 50.0f };
                if (CheckCollisionPointRec(mouse, btnBackHangar)) currentState = GameState::HANGAR;
                if (CheckCollisionPointRec(mouse, btnBackMenu))   currentState = GameState::MENU;
            }
        } else {
            currentState = GameState::MENU;
        }

        // ==================== DRAW ====================
        BeginDrawing();
        ClearBackground(Color{ 8, 12, 24, 255 });

        if (currentState == GameState::MENU) {
            for (const auto& s : stars) {
                DrawCircleV({ s.x, s.y }, s.size, Color{ 230, 240, 255, s.alpha });
            }

            // Idioma discreto arriba a la izquierda
            Rectangle btnLang = { 24.0f, 20.0f, 115.0f, 34.0f };
            DrawRectangleRec(btnLang, Color{ 20, 30, 54, 255 });
            DrawRectangleLinesEx(btnLang, 1.5f, SKYBLUE);
            DrawUIText(missionData.lang == Language::ES ? "ESPAÑOL" : "ENGLISH",
                       btnLang.x + 18.0f, btnLang.y + 8.0f, 16.0f, WHITE);

            DrawUIText("MOONSHOT", 455.0f, 48.0f, 64.0f, GOLD);
            DrawUIText(Tr(missionData.lang,
                "SIMULADOR DE INGENIERÍA AEROESPACIAL · PROGRAMA ARTEMIS / APOLO",
                "AEROSPACE ENGINEERING SIMULATOR · ARTEMIS / APOLLO PROGRAM"),
                315.0f, 122.0f, 20.0f, SKYBLUE);

            DrawUIText(Tr(missionData.lang,
                "RANURAS DE MISIÓN (SAFE PART):",
                "MISSION SAVE SLOTS (SAFE PART):"),
                90.0f, 185.0f, 22.0f, WHITE);

            // Columna izquierda: 3 Ranuras
            for (int slot = 1; slot <= 3; ++slot) {
                int idx = slot - 1;
                Rectangle slotBox = { 90.0f, 225.0f + idx * 130.0f, 690.0f, 112.0f };
                bool hover = CheckCollisionPointRec(mouse, slotBox);
                if (hover) selectedSlot = slot;

                SaveSlotSummary summary = SaveManager::GetSlotSummary(slot);

                DrawRectangleRec(slotBox, hover ? Color{ 26, 42, 76, 255 } : Color{ 15, 23, 44, 245 });
                DrawRectangleLinesEx(slotBox, (selectedSlot == slot) ? 3.0f : 2.0f,
                                     summary.occupied ? GREEN : GOLD);

                DrawUIText(TextFormat("SAFE PART %d", slot), slotBox.x + 22.0f, slotBox.y + 16.0f, 26.0f, GOLD);

                if (summary.occupied) {
                    const char* phaseLabel = Tr(missionData.lang, "Fase 1: Comité NASA", "Phase 1: NASA Committee");
                    if (summary.savedPhase == GameState::HANGAR) {
                        phaseLabel = Tr(missionData.lang, "Fase 2: Hangar de Cohetes", "Phase 2: Rocket Hangar");
                    } else if (summary.savedPhase == GameState::FLIGHT) {
                        phaseLabel = Tr(missionData.lang, "Fase 3: Listo para Lanzamiento", "Phase 3: Ready for Launch");
                    }

                    DrawUIText(phaseLabel, slotBox.x + 22.0f, slotBox.y + 52.0f, 19.0f, SKYBLUE);
                    DrawUIText(TextFormat(Tr(missionData.lang, "Presupuesto: $%.0fM", "Budget: $%.0fM"), summary.awardedBudget),
                               slotBox.x + 22.0f, slotBox.y + 78.0f, 17.0f, LIGHTGRAY);

                    // A la derecha de la ranura: CONTINUAR PARTIDA
                    DrawUIText(Tr(missionData.lang, "CONTINUAR PARTIDA >", "CONTINUE GAME >"),
                               slotBox.x + 445.0f, slotBox.y + 20.0f, 20.0f, GREEN);

                    Rectangle delBtn = { slotBox.x + slotBox.width - 110.0f, slotBox.y + 62.0f, 92.0f, 34.0f };
                    DrawRectangleRec(delBtn, Color{ 135, 38, 38, 255 });
                    DrawRectangleLinesEx(delBtn, 1.5f, WHITE);
                    DrawUIText(Tr(missionData.lang, "BORRAR", "DELETE"), delBtn.x + 14.0f, delBtn.y + 8.0f, 15.0f, WHITE);
                } else {
                    DrawUIText(Tr(missionData.lang, "Ranura disponible sin datos", "Available empty save slot"),
                               slotBox.x + 22.0f, slotBox.y + 58.0f, 19.0f, LIGHTGRAY);

                    // A la derecha de la ranura: NUEVA PARTIDA
                    DrawUIText(Tr(missionData.lang, "NUEVA PARTIDA >", "NEW GAME >"),
                               slotBox.x + 485.0f, slotBox.y + 42.0f, 21.0f, GOLD);
                }
            }

            // Panel derecho dedicado con NUEVA PARTIDA / CONTINUAR PARTIDA
            SaveSlotSummary selSummary = SaveManager::GetSlotSummary(selectedSlot);
            Rectangle rightPanel = { 810.0f, 225.0f, 380.0f, 372.0f };
            DrawRectangleRec(rightPanel, Color{ 13, 20, 38, 248 });
            DrawRectangleLinesEx(rightPanel, 2.5f, selSummary.occupied ? GREEN : GOLD);

            DrawUIText(TextFormat("SAFE PART %d", selectedSlot), 835.0f, 248.0f, 28.0f, GOLD);
            DrawLine(835, 286, 1165, 286, Color{ 50, 75, 120, 255 });

            if (selSummary.occupied) {
                DrawUIText(Tr(missionData.lang, "ESTADO: PARTIDA GUARDADA", "STATUS: SAVED MISSION"),
                           835.0f, 304.0f, 19.0f, GREEN);

                const char* phaseName = Tr(missionData.lang, "Fase 1 · Comité NASA", "Phase 1 · NASA Committee");
                if (selSummary.savedPhase == GameState::HANGAR) {
                    phaseName = Tr(missionData.lang, "Fase 2 · Hangar y Tienda", "Phase 2 · Hangar & Shop");
                } else if (selSummary.savedPhase == GameState::FLIGHT) {
                    phaseName = Tr(missionData.lang, "Fase 3 · Pre-Lanzamiento", "Phase 3 · Pre-Launch");
                }

                DrawUIText(phaseName, 835.0f, 338.0f, 19.0f, SKYBLUE);
                DrawUIText(TextFormat(Tr(missionData.lang, "Fondos Totales: $%.0fM", "Total Funds: $%.0fM"), selSummary.awardedBudget),
                           835.0f, 372.0f, 19.0f, WHITE);
                DrawUIText(TextFormat(Tr(missionData.lang, "Fondos Libres: $%.0fM", "Remaining Funds: $%.0fM"), selSummary.remainingBudget),
                           835.0f, 402.0f, 19.0f, LIGHTGRAY);

                Rectangle rightMainBtn = { 835.0f, 465.0f, 330.0f, 56.0f };
                DrawRectangleRec(rightMainBtn, Color{ 38, 135, 68, 255 });
                DrawRectangleLinesEx(rightMainBtn, 2.0f, WHITE);
                const char* btnTxt = Tr(missionData.lang, "CONTINUAR PARTIDA", "CONTINUE GAME");
                float tw = MeasureUIText(btnTxt, 22.0f);
                DrawUIText(btnTxt, rightMainBtn.x + (rightMainBtn.width - tw) * 0.5f, rightMainBtn.y + 16.0f, 22.0f, WHITE);

                Rectangle rightDelBtn = { 835.0f, 535.0f, 330.0f, 40.0f };
                DrawRectangleRec(rightDelBtn, Color{ 125, 36, 36, 255 });
                DrawRectangleLinesEx(rightDelBtn, 1.5f, WHITE);
                const char* delTxt = Tr(missionData.lang, "BORRAR PARTIDA", "DELETE SAVE");
                float dw = MeasureUIText(delTxt, 18.0f);
                DrawUIText(delTxt, rightDelBtn.x + (rightDelBtn.width - dw) * 0.5f, rightDelBtn.y + 10.0f, 18.0f, WHITE);
            } else {
                DrawUIText(Tr(missionData.lang, "ESTADO: RANURA VACÍA", "STATUS: EMPTY SLOT"),
                           835.0f, 304.0f, 19.0f, GOLD);
                DrawUITextBoxed(Tr(missionData.lang,
                    "Inicia una nueva campaña espacial desde la audiencia presupuestaria del Comité de la NASA.",
                    "Start a brand-new space campaign from the NASA Committee budget hearing."),
                    { 835.0f, 342.0f, 330.0f, 100.0f }, 18.0f, 4.0f, LIGHTGRAY);

                Rectangle rightMainBtn = { 835.0f, 465.0f, 330.0f, 56.0f };
                DrawRectangleRec(rightMainBtn, Color{ 35, 85, 155, 255 });
                DrawRectangleLinesEx(rightMainBtn, 2.0f, GOLD);
                const char* btnTxt = Tr(missionData.lang, "NUEVA PARTIDA", "NEW GAME");
                float tw = MeasureUIText(btnTxt, 22.0f);
                DrawUIText(btnTxt, rightMainBtn.x + (rightMainBtn.width - tw) * 0.5f, rightMainBtn.y + 16.0f, 22.0f, WHITE);
            }
        } else if (currentState == GameState::COMMITTEE) {
            committee.Draw(missionData);
        } else if (currentState == GameState::HANGAR) {
            labShop.Draw(missionData);
        } else if (currentState == GameState::FLIGHT) {
            for (const auto& s : stars) {
                DrawCircleV({ s.x, s.y }, s.size, Color{ 230, 240, 255, s.alpha });
            }

            DrawRectangle(190, 95, 900, 530, Color{ 12, 18, 34, 248 });
            DrawRectangleLinesEx({ 190.0f, 95.0f, 900.0f, 530.0f }, 3.0f, GOLD);

            // Banner de Auto-Save realizado
            DrawRectangle(230, 122, 820, 42, Color{ 22, 82, 46, 255 });
            DrawRectangleLinesEx({ 230.0f, 122.0f, 820.0f, 42.0f }, 2.0f, GREEN);
            DrawUIText(TextFormat(Tr(missionData.lang,
                "AUTO-GUARDADO COMPLETADO EN SAFE PART %d · COHETE LISTO PARA FASE 3",
                "AUTO-SAVE COMPLETED IN SAFE PART %d · ROCKET READY FOR PHASE 3"),
                missionData.activeSaveSlot),
                255.0f, 133.0f, 18.0f, WHITE);

            DrawUIText(Tr(missionData.lang,
                "FASE 3 · SIMULACIÓN DE LANZAMIENTO LUNAR (PRÓXIMAMENTE)",
                "PHASE 3 · LUNAR LAUNCH SIMULATION (COMING NEXT)"),
                230.0f, 186.0f, 26.0f, GOLD);

            DrawUITextBoxed(Tr(missionData.lang,
                "Tu progreso y el diseño completo de tu cohete han quedado guardados automáticamente antes del lanzamiento.\n"
                "Aquí se desarrollará la Fase 3 de vuelo orbital y alunizaje. Puedes volver al Hangar para comprar/vender piezas o salir al menú.",
                "Your progress and complete rocket assembly have been auto-saved prior to launch.\n"
                "Phase 3 orbital flight and lunar landing will be built here. You can return to the Hangar to buy/sell parts or exit to menu."),
                { 230.0f, 230.0f, 820.0f, 90.0f }, 19.0f, 5.0f, LIGHTGRAY);

            // Resumen tecnico del cohete guardado
            DrawRectangle(230, 332, 820, 185, Color{ 18, 27, 48, 255 });
            DrawRectangleLinesEx({ 230.0f, 332.0f, 820.0f, 185.0f }, 1.5f, SKYBLUE);

            DrawUIText(Tr(missionData.lang, "CONFIGURACIÓN DE COHETE AUTO-GUARDADA:", "AUTO-SAVED ROCKET CONFIGURATION:"),
                       250.0f, 346.0f, 19.0f, SKYBLUE);

            DrawUIText(TextFormat("• %s   |   • %s",
                       missionData.rocket.capsule.name.c_str(),
                       missionData.rocket.lander.name.c_str()),
                       250.0f, 380.0f, 18.0f, WHITE);

            DrawUIText(TextFormat("• %s   |   • %s",
                       missionData.rocket.tank.name.c_str(),
                       missionData.rocket.engine.name.c_str()),
                       250.0f, 410.0f, 18.0f, WHITE);

            DrawUIText(TextFormat(Tr(missionData.lang,
                       "Coste Total: $%.0fM   |   Masa Seca: %.1f t   |   Combustible: %.1f t",
                       "Total Cost: $%.0fM   |   Dry Mass: %.1f t   |   Propellant: %.1f t"),
                       missionData.rocket.totalCost, missionData.rocket.dryMassTons, missionData.rocket.maxFuelTons),
                       250.0f, 446.0f, 18.0f, GOLD);

            DrawUIText(TextFormat("Empuje: %.0f kN   |   TWR: %.2f   |   Delta-V: %.0f m/s",
                       missionData.rocket.thrustKN, missionData.rocket.twr, missionData.rocket.deltaV),
                       250.0f, 476.0f, 18.0f, GREEN);

            Rectangle btnBackHangar = { 310.0f, 545.0f, 310.0f, 50.0f };
            DrawRectangleRec(btnBackHangar, Color{ 38, 82, 145, 255 });
            DrawRectangleLinesEx(btnBackHangar, 2.0f, WHITE);
            DrawUIText(Tr(missionData.lang, "< VOLVER AL HANGAR", "< BACK TO HANGAR"), 360.0f, 560.0f, 20.0f, WHITE);

            Rectangle btnBackMenu = { 660.0f, 545.0f, 310.0f, 50.0f };
            DrawRectangleRec(btnBackMenu, Color{ 38, 125, 62, 255 });
            DrawRectangleLinesEx(btnBackMenu, 2.0f, WHITE);
            DrawUIText(Tr(missionData.lang, "MENÚ PRINCIPAL", "MAIN MENU"), 735.0f, 560.0f, 20.0f, WHITE);
        }

        EndDrawing();
    }

    committee.Unload();
    labShop.Unload();
    UnloadGameFont();
    CloseWindow();
    return 0;
}