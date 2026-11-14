#include "raylib.h"
#include "game_types.h"
#include "committee.h"
#include "lab_shop.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Moonshot - NASA Space Apps Challenge");
    SetTargetFPS(60);

    GameState currentState = GameState::COMMITTEE;
    SharedMissionData missionData;

    CommitteePhase committeePhase;
    LabShopPhase labShopPhase;

    committeePhase.Init();
    labShopPhase.Init();

    while (!WindowShouldClose()) {
        switch (currentState) {
            case GameState::COMMITTEE:
                if (committeePhase.Update(missionData) == GameState::HANGAR) {
                    // Escalamos el presupuesto (de millones reales $3800-$6500M a la escala de la tienda)
                    missionData.awardedBudget = missionData.awardedBudget / 38.0f;
                    labShopPhase.OnEnter(missionData);
                    currentState = GameState::HANGAR;
                }
                break;

            case GameState::HANGAR:
                if (labShopPhase.Update(missionData) == GameState::FLIGHT) {
                    // Aquí conectaremos la Fase 3 (Misión Lunar)
                    currentState = GameState::FLIGHT;
                }
                break;

            case GameState::FLIGHT:
                if (IsKeyPressed(KEY_R)) {
                    missionData.Reset();
                    committeePhase.Init();
                    labShopPhase.Init();
                    currentState = GameState::COMMITTEE;
                }
                break;
        }

        BeginDrawing();
        ClearBackground(Color{ 20, 26, 51, 255 });

        switch (currentState) {
            case GameState::COMMITTEE: committeePhase.Draw(missionData); break;
            case GameState::HANGAR:    labShopPhase.Draw(missionData);   break;
            case GameState::FLIGHT:
                DrawText("FASE 3: LANZAMIENTO LUNAR (Listo para conectar)", 310, 340, 26, GREEN);
                DrawText("Pulsa [R] para volver a empezar", 460, 390, 20, LIGHTGRAY);
                break;
        }

        EndDrawing();
    }

    committeePhase.Unload();
    labShopPhase.Unload();
    CloseWindow();
    return 0;
}
