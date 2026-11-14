#pragma once

#include <string>
#include <vector>
#include "raylib.h"

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

enum class GameState {
    COMMITTEE,  // Fase 1: Audiencia de presupuesto con los jueces
    HANGAR,     // Fase 2: Tienda y laboratorio de ensamblaje
    FLIGHT      // Fase 3: Misión de lanzamiento hacia la Luna
};

enum class PartCategory {
    CAPSULE,
    TANK,
    ENGINE
};

struct RocketPart {
    int id = 0;
    std::string name;
    PartCategory category = PartCategory::CAPSULE;
    float costMillions = 0.0f;
    float dryMassTons = 0.0f;
    float fuelMassTons = 0.0f;
    float thrustKN = 0.0f;
    std::string nasaRef;
};

struct RocketStats {
    RocketPart capsule;
    RocketPart tank;
    RocketPart engine;

    float totalCost = 0.0f;
    float dryMassTons = 0.0f;
    float maxFuelTons = 0.0f;
    float thrustKN = 0.0f;
    float twr = 0.0f;
};

struct SharedMissionData {
    float awardedBudget = 0.0f;    // Presupuesto aprobado en millones (ej. 5500.0f = $5.5B)
    float remainingBudget = 0.0f;
    RocketStats rocket;
    bool missionSuccess = false;
    std::string endReason = "";

    void Reset() {
        awardedBudget = 0.0f;
        remainingBudget = 0.0f;
        rocket = RocketStats{};
        missionSuccess = false;
        endReason.clear();
    }
};
