#pragma once

#include "game_types.h"
#include "raylib.h"
#include <vector>
#include <string>

struct ShopItem {
    RocketPart part;
    bool purchased = false;
};

class LabShopPhase {
public:
    void Init();
    void Unload();
    void OnEnter(SharedMissionData& missionData);
    GameState Update(SharedMissionData& missionData);
    void Draw(const SharedMissionData& missionData) const;

private:
    std::vector<ShopItem> capsuleItems;
    std::vector<ShopItem> tankItems;
    std::vector<ShopItem> engineItems;

    int activeTab = 0;

    int equippedCapsuleIdx = -1;
    int equippedTankIdx    = -1;
    int equippedEngineIdx  = -1;

    // Variables para las mejoras visuales y físicas
    float scanAnimTimer   = 0.0f; // Temporizador del láser de escaneo al montar pieza
    float estimatedDeltaV = 0.0f; // Delta-V calculado con la ecuación de Tsiolkovsky

    std::string shopFeedback = "Compra una Cápsula, un Tanque y un Motor con tu presupuesto.";
    Color feedbackColor = LIGHTGRAY;

    // --- HUECOS PARA TUS SPRITES ---
    Texture2D texLabBg    = { 0 };
    Texture2D texCapsule  = { 0 };
    Texture2D texTank     = { 0 };
    Texture2D texEngine   = { 0 };

    void HandleShopInteraction(std::vector<ShopItem>& currentList, int& equippedIdx, SharedMissionData& missionData);
    void DrawShopList(const std::vector<ShopItem>& currentList, int equippedIdx, float budget) const;
    void RecalculateRocketStats(SharedMissionData& missionData);
};
