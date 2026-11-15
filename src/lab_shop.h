#pragma once

#include "games_types.h"
#include <string>
#include <vector>

class LabShopPhase {
public:
    void Init();
    void OnEnter(SharedMissionData& data);
    void RestoreFromSave(SharedMissionData& data);
    GameState Update(SharedMissionData& data);
    void Draw(const SharedMissionData& data);
    void Unload();

private:
    Texture2D texLabBg{};
    bool bgLoaded = false;

    // PNGs del cohete cargados automaticamente desde assets/sprites/
    std::vector<Texture2D> rocketPngs;
    std::vector<std::string> rocketPngNames;

    int selectedFilter = 0; // 0 = TODAS, 1 = CAPSULE, 2 = LANDER, 3 = TANK, 4 = ENGINE, 5 = BOOSTER
    float scrollY = 0.0f;
    float maxScrollY = 0.0f;
    bool draggingScrollbar = false;

    std::vector<RocketPart> parts;
    float saveTimer = 0.0f;

    void LoadRocketSprites();
    void RecalculateRocket(SharedMissionData& data);
    std::vector<int> GetFilteredPartIndices() const;
    int GetActiveRocketSpriteIndex(const SharedMissionData& data) const;
    bool CanReachLunarOrbit(const SharedMissionData& data, std::string& outReason) const;
};