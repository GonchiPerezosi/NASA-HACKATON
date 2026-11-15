#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "raylib.h"

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

enum class Language {
    ES,
    EN
};

inline const char* Tr(Language lang, const char* es, const char* en) {
    return (lang == Language::ES) ? es : en;
}

// ============================================================================
// TIPOGRAFÍA GRUESA DE ALTA LEGIBILIDAD (BOLD + SOMBRA SUAVE)
// ============================================================================
inline Font g_uiFont = { 0 };
inline bool g_hasCustomFont = false;

inline void LoadGameFont() {
    std::vector<int> codepoints;
    for (int cp = 32; cp <= 126; ++cp) codepoints.push_back(cp);
    for (int cp = 160; cp <= 255; ++cp) codepoints.push_back(cp); // ¡ ¿ Á É Í Ó Ú Ü Ñ á é í ó ú ü ñ

    // Priorizamos fuentes en NEGRITA (Bold) para que el trazo sea grueso y legible
    const char* candidates[] = {
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/calibrib.ttf",
        "C:/Windows/Fonts/verdanab.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
    };

    for (const char* path : candidates) {
        if (FileExists(path)) {
            g_uiFont = LoadFontEx(path, 38, codepoints.data(), static_cast<int>(codepoints.size()));
            if (g_uiFont.texture.id > 0) {
                SetTextureFilter(g_uiFont.texture, TEXTURE_FILTER_BILINEAR);
                g_hasCustomFont = true;
                return;
            }
        }
    }

    g_uiFont = GetFontDefault();
    g_hasCustomFont = false;
}

inline void UnloadGameFont() {
    if (g_hasCustomFont && g_uiFont.texture.id > 0) {
        UnloadFont(g_uiFont);
        g_hasCustomFont = false;
    }
}

inline void DrawUIText(const char* text, float x, float y, float fontSize, Color color, bool shadow = true) {
    if (g_hasCustomFont) {
        if (shadow && (color.r > 50 || color.g > 50 || color.b > 50)) {
            DrawTextEx(g_uiFont, text, { x + 1.5f, y + 1.5f }, fontSize, 1.0f, Color{ 0, 0, 0, 185 });
        }
        DrawTextEx(g_uiFont, text, { x, y }, fontSize, 1.0f, color);
    } else {
        DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(fontSize), color);
    }
}

inline float MeasureUIText(const char* text, float fontSize) {
    if (g_hasCustomFont) {
        return MeasureTextEx(g_uiFont, text, fontSize, 1.0f).x;
    }
    return static_cast<float>(MeasureText(text, static_cast<int>(fontSize)));
}

inline void DrawUITextBoxed(const char* text, Rectangle bounds, float fontSize, float lineSpacing, Color color, bool shadow = false) {
    std::string raw(text);
    std::istringstream paragraphStream(raw);
    std::string paragraph;
    float curY = bounds.y;

    while (std::getline(paragraphStream, paragraph, '\n')) {
        if (paragraph.empty()) {
            curY += fontSize + lineSpacing;
            continue;
        }

        std::istringstream wordStream(paragraph);
        std::string word;
        std::string currentLine;

        while (wordStream >> word) {
            std::string candidate = currentLine.empty() ? word : (currentLine + " " + word);
            if (MeasureUIText(candidate.c_str(), fontSize) <= bounds.width) {
                currentLine = candidate;
            } else {
                if (!currentLine.empty()) {
                    if (curY + fontSize <= bounds.y + bounds.height) {
                        DrawUIText(currentLine.c_str(), bounds.x, curY, fontSize, color, shadow);
                    }
                    curY += fontSize + lineSpacing;
                }
                currentLine = word;
            }
        }

        if (!currentLine.empty()) {
            if (curY + fontSize <= bounds.y + bounds.height) {
                DrawUIText(currentLine.c_str(), bounds.x, curY, fontSize, color, shadow);
            }
            curY += fontSize + lineSpacing;
        }
    }
}

// Interpolación suave de color: Rojo (0.0) -> Ámbar/Amarillo (0.5) -> Verde (1.0)
inline Color GetApprovalColor(float value) {
    float t = std::clamp(value, 0.0f, 1.0f);
    Color red    = { 225, 48, 48, 255 };
    Color yellow = { 245, 200, 45, 255 };
    Color green  = { 46, 215, 95, 255 };

    if (t < 0.5f) {
        float k = t / 0.5f;
        return Color{
            static_cast<unsigned char>(red.r + (yellow.r - red.r) * k),
            static_cast<unsigned char>(red.g + (yellow.g - red.g) * k),
            static_cast<unsigned char>(red.b + (yellow.b - red.b) * k),
            255
        };
    } else {
        float k = (t - 0.5f) / 0.5f;
        return Color{
            static_cast<unsigned char>(yellow.r + (green.r - yellow.r) * k),
            static_cast<unsigned char>(yellow.g + (green.g - yellow.g) * k),
            static_cast<unsigned char>(yellow.b + (green.b - yellow.b) * k),
            255
        };
    }
}

// ============================================================================
// ESTADOS Y ESTRUCTURAS DE MISIÓN
// ============================================================================
enum class GameState {
    MENU,
    LOADING,
    COMMITTEE,
    HANGAR,
    FLIGHT
};

enum class PartCategory {
    CAPSULE,
    LANDER,
    TANK,
    ENGINE,
    BOOSTER
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
    RocketPart lander;
    RocketPart tank;
    RocketPart engine;
    RocketPart booster;

    float totalCost = 0.0f;
    float dryMassTons = 0.0f;
    float maxFuelTons = 0.0f;
    float thrustKN = 0.0f;
    float twr = 0.0f;
    float deltaV = 0.0f;
};

struct SharedMissionData {
    Language lang = Language::ES;
    int activeSaveSlot = 1;
    GameState savedPhase = GameState::COMMITTEE;

    float awardedBudget = 0.0f;
    float remainingBudget = 0.0f;

    int eqCapsuleIdx = -1;
    int eqLanderIdx  = -1;
    int eqTankIdx    = -1;
    int eqEngineIdx  = -1;
    int eqBoosterIdx = 24;

    RocketStats rocket;
    bool missionSuccess = false;
    std::string endReason = "";

    void Reset() {
        savedPhase = GameState::COMMITTEE;
        awardedBudget = 0.0f;
        remainingBudget = 0.0f;
        eqCapsuleIdx = -1;
        eqLanderIdx  = -1;
        eqTankIdx    = -1;
        eqEngineIdx  = -1;
        eqBoosterIdx = 24;
        rocket = RocketStats{};
        missionSuccess = false;
        endReason.clear();
    }
};