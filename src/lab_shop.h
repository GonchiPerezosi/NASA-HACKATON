#pragma once

#include "games_types.h"
#include "raylib.h"
#include <vector>
#include <string>

struct ShopItem {
    RocketPart part;
    bool purchased = false;
    Texture2D texture = { 0 };
    Color tint = WHITE;
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
    std::vector<ShopItem> landerItems;
    std::vector<ShopItem> tankItems;
    std::vector<ShopItem> engineItems;
    std::vector<ShopItem> boosterItems;

    int activeTab = 0;
    float scrollOffset = 0.0f;

    int equippedCapsuleIdx = -1;
    int equippedLanderIdx  = -1;
    int equippedTankIdx    = -1;
    int equippedEngineIdx  = -1;
    int equippedBoosterIdx = 0;

    float scanAnimTimer   = 0.0f;
    float estimatedDeltaV = 0.0f;

    std::string shopFeedback = "Usa la rueda del raton para explorar las 30 piezas disponibles.";
    Color feedbackColor = LIGHTGRAY;

    Texture2D texLabBg = { 0 };

    void HandleShopInteraction(std::vector<ShopItem>& currentList, int& equippedIdx, bool isOptional, SharedMissionData& missionData);
    void DrawShopList(const std::vector<ShopItem>& currentList, int equippedIdx, float budget) const;
    void RecalculateRocketStats(SharedMissionData& missionData);
};