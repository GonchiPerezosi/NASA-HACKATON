#include "raylib.h"
#include "games_types.h"
#include "comitee.h"
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
        // Atajo F2: salta directo a la Fase 2 (Tienda de 30 piezas) con $145M para probar rapido
        if (currentState == GameState::COMMITTEE && IsKeyPressed(KEY_F2)) {
            missionData.awardedBudget = 145.0f;
            labShopPhase.OnEnter(missionData);
            currentState = GameState::HANGAR;
        }

        switch (currentState) {
            case GameState::COMMITTEE:
                if (committeePhase.Update(missionData) == GameState::HANGAR) {
                    missionData.awardedBudget = missionData.awardedBudget / 38.0f;
                    labShopPhase.OnEnter(missionData);
                    currentState = GameState::HANGAR;
                }
                break;

            case GameState::HANGAR:
                if (labShopPhase.Update(missionData) == GameState::FLIGHT) {
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
                DrawText("FASE 3: COHETE LISTO EN PLATAFORMA", 330, 310, 28, GREEN);
                DrawText(TextFormat("Masa: %.1f t | Combustible: %.1f t | Empuje: %.0f kN",
                         missionData.rocket.dryMassTons,
                         missionData.rocket.maxFuelTons,
                         missionData.rocket.thrustKN), 320, 365, 20, WHITE);
                DrawText("Pulsa [R] para reiniciar desde el Comite", 430, 420, 20, LIGHTGRAY);
                break;
        }

        EndDrawing();
    }

    committeePhase.Unload();
    labShopPhase.Unload();
    CloseWindow();
    return 0;
}