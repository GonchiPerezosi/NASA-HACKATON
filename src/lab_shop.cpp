#include "lab_shop.h"
#include <cmath>
#include <algorithm>

void LabShopPhase::Init() {
    activeTab = 0;
    equippedCapsuleIdx = -1;
    equippedTankIdx    = -1;
    equippedEngineIdx  = -1;
    scanAnimTimer      = 0.0f;
    estimatedDeltaV    = 0.0f;

    shopFeedback = "Compra una Cápsula, un Tanque y un Motor con tu presupuesto.";
    feedbackColor = LIGHTGRAY;

    // ========================================================================
    // >>> PEGA AQUÍ TUS SPRITES DEL LABORATORIO Y PIEZAS (Quita las //) <<<
    // ========================================================================
    // texLabBg   = LoadTexture("assets/fondo_lab.png");
    // texCapsule = LoadTexture("assets/capsula.png");
    // texTank    = LoadTexture("assets/tanque.png");
    // texEngine  = LoadTexture("assets/motor.png");
    // ========================================================================

    capsuleItems = {
        { { 1, "Cápsula Mercury-Lite", PartCategory::CAPSULE, 15.0f, 1.5f, 0.0f, 0.0f, "Proyecto Mercury (1961)" }, false },
        { { 2, "Módulo Apolo CSM",     PartCategory::CAPSULE, 35.0f, 5.5f, 0.0f, 0.0f, "Programa Apolo (1969)" },   false },
        { { 3, "Cápsula Orión MPCV",   PartCategory::CAPSULE, 55.0f, 8.5f, 0.0f, 0.0f, "Programa Artemis (2024)" }, false }
    };

    tankItems = {
        { { 4, "Tanque Delta-II",      PartCategory::TANK,    20.0f, 2.0f, 22.0f, 0.0f, "RP-1 / LOX Compacto" },     false },
        { { 5, "Etapa S-IVB Saturno",  PartCategory::TANK,    40.0f, 4.0f, 44.0f, 0.0f, "Saturno V Tercera Etapa" }, false },
        { { 6, "Tanque Core SLS",      PartCategory::TANK,    65.0f, 6.5f, 75.0f, 0.0f, "Space Launch System LH2" }, false }
    };

    engineItems = {
        { { 7, "Motor Merlin-1D Vac",  PartCategory::ENGINE,  25.0f, 1.0f, 0.0f,  850.0f, "Empuje ligero (850 kN)" },     false },
        { { 8, "Motor J-2 Hidrolox",   PartCategory::ENGINE,  45.0f, 1.8f, 0.0f, 1450.0f, "Propulsión lunar (1450 kN)" }, false },
        { { 9, "Clúster RS-25D",       PartCategory::ENGINE,  70.0f, 3.5f, 0.0f, 2400.0f, "Motores SLS (2400 kN)" },      false }
    };
}

void LabShopPhase::Unload() {
    if (texLabBg.id > 0)   UnloadTexture(texLabBg);
    if (texCapsule.id > 0) UnloadTexture(texCapsule);
    if (texTank.id > 0)    UnloadTexture(texTank);
    if (texEngine.id > 0)  UnloadTexture(texEngine);
}

void LabShopPhase::OnEnter(SharedMissionData& missionData) {
    if (missionData.awardedBudget <= 0.0f) {
        missionData.awardedBudget = 120.0f;
    }
    missionData.remainingBudget = missionData.awardedBudget;
    RecalculateRocketStats(missionData);
}

void LabShopPhase::RecalculateRocketStats(SharedMissionData& missionData) {
    RocketStats& r = missionData.rocket;
    r = RocketStats{};

    if (equippedCapsuleIdx >= 0) r.capsule = capsuleItems[equippedCapsuleIdx].part;
    if (equippedTankIdx >= 0)    r.tank    = tankItems[equippedTankIdx].part;
    if (equippedEngineIdx >= 0)  r.engine  = engineItems[equippedEngineIdx].part;

    r.totalCost   = r.capsule.costMillions + r.tank.costMillions + r.engine.costMillions;
    r.dryMassTons = r.capsule.dryMassTons  + r.tank.dryMassTons  + r.engine.dryMassTons;
    r.maxFuelTons = r.tank.fuelMassTons;
    r.thrustKN    = r.engine.thrustKN;

    float totalWetMass  = r.dryMassTons + r.maxFuelTons;
    float weightEarthKN = totalWetMass * 9.81f;
    r.twr = (weightEarthKN > 0.0f) ? (r.thrustKN / weightEarthKN) : 0.0f;

    // Cálculo de Delta-V real (Ecuación de Tsiolkovsky: dV = Isp * g0 * ln(m0 / mf))
    if (r.dryMassTons > 0.0f && r.maxFuelTons > 0.0f && r.thrustKN > 0.0f) {
        float isp = 310.0f + (r.thrustKN / 2400.0f) * 140.0f; // Impulso específico estimado (s)
        estimatedDeltaV = isp * 9.81f * std::log(totalWetMass / r.dryMassTons);
    } else {
        estimatedDeltaV = 0.0f;
    }
}

void LabShopPhase::HandleShopInteraction(std::vector<ShopItem>& currentList, int& equippedIdx, SharedMissionData& missionData) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    Vector2 mousePos = GetMousePosition();

    for (size_t i = 0; i < currentList.size(); ++i) {
        float cardY = 175.0f + i * 155.0f;
        Rectangle mainActionBtn = { 590.0f, cardY + 25.0f, 195.0f, 48.0f };
        Rectangle refundBtn     = { 590.0f, cardY + 82.0f, 195.0f, 34.0f };

        if (CheckCollisionPointRec(mousePos, mainActionBtn)) {
            if (!currentList[i].purchased) {
                float price = currentList[i].part.costMillions;
                if (missionData.remainingBudget >= price) {
                    missionData.remainingBudget -= price;
                    currentList[i].purchased = true;
                    equippedIdx = static_cast<int>(i);
                    scanAnimTimer = 1.0f; // Activar escáner láser
                    RecalculateRocketStats(missionData);

                    shopFeedback = "Comprado y montado: " + currentList[i].part.name;
                    feedbackColor = GREEN;
                } else {
                    shopFeedback = "Saldo insuficiente para " + currentList[i].part.name;
                    feedbackColor = RED;
                }
            } else {
                equippedIdx = static_cast<int>(i);
                scanAnimTimer = 1.0f; // Activar escáner láser
                RecalculateRocketStats(missionData);
                shopFeedback = "Pieza equipada: " + currentList[i].part.name;
                feedbackColor = SKYBLUE;
            }
        }
        else if (currentList[i].purchased && CheckCollisionPointRec(mousePos, refundBtn)) {
            missionData.remainingBudget += currentList[i].part.costMillions;
            currentList[i].purchased = false;
            if (equippedIdx == static_cast<int>(i)) {
                equippedIdx = -1;
            }
            RecalculateRocketStats(missionData);
            shopFeedback = "Pieza vendida. Reembolso: +$" + std::to_string(static_cast<int>(currentList[i].part.costMillions)) + "M";
            feedbackColor = ORANGE;
        }
    }
}

GameState LabShopPhase::Update(SharedMissionData& missionData) {
    Vector2 mousePos = GetMousePosition();

    // Actualizar animación del escáner láser
    if (scanAnimTimer > 0.0f) {
        scanAnimTimer = std::max(0.0f, scanAnimTimer - GetFrameTime() * 1.6f);
    }

    // Cambio de pestañas de la tienda
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int t = 0; t < 3; ++t) {
            Rectangle tabRect = { 35.0f + t * 260.0f, 105.0f, 245.0f, 48.0f };
            if (CheckCollisionPointRec(mousePos, tabRect)) {
                activeTab = t;
            }
        }
    }

    if (activeTab == 0)      HandleShopInteraction(capsuleItems, equippedCapsuleIdx, missionData);
    else if (activeTab == 1) HandleShopInteraction(tankItems,    equippedTankIdx,    missionData);
    else if (activeTab == 2) HandleShopInteraction(engineItems,  equippedEngineIdx,  missionData);

    bool allPartsMounted = (equippedCapsuleIdx >= 0) && (equippedTankIdx >= 0) && (equippedEngineIdx >= 0);
    bool validTWR        = missionData.rocket.twr > 1.05f;
    bool canLaunch       = allPartsMounted && validTWR;

    Rectangle launchBtn = { 865.0f, 605.0f, 375.0f, 70.0f };
    bool clickedLaunch  = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, launchBtn);

    if (canLaunch && (clickedLaunch || IsKeyPressed(KEY_ENTER))) {
        return GameState::FLIGHT;
    }

    return GameState::HANGAR;
}

void LabShopPhase::DrawShopList(const std::vector<ShopItem>& currentList, int equippedIdx, float budget) const {
    Vector2 mousePos = GetMousePosition();

    for (size_t i = 0; i < currentList.size(); ++i) {
        const auto& item = currentList[i];
        float cardY = 175.0f + i * 155.0f;
        Rectangle cardRect = { 35.0f, cardY, 775.0f, 138.0f };

        bool isEquipped = (static_cast<int>(i) == equippedIdx);
        Color borderCol = isEquipped ? GOLD : (item.purchased ? SKYBLUE : GRAY);

        DrawRectangleRec(cardRect, Color{ 25, 31, 46, 240 });
        DrawRectangleLinesEx(cardRect, isEquipped ? 3.0f : 1.5f, borderCol);

        // Miniatura / Hueco de sprite
        Rectangle iconBox = { 52.0f, cardY + 24.0f, 90.0f, 90.0f };
        DrawRectangleRec(iconBox, Color{ 16, 20, 30, 255 });
        DrawRectangleLinesEx(iconBox, 1.0f, DARKGRAY);

        const Texture2D* activeTex = nullptr;
        if (item.part.category == PartCategory::CAPSULE && texCapsule.id > 0) activeTex = &texCapsule;
        if (item.part.category == PartCategory::TANK    && texTank.id > 0)    activeTex = &texTank;
        if (item.part.category == PartCategory::ENGINE  && texEngine.id > 0)  activeTex = &texEngine;

        if (activeTex != nullptr) {
            DrawTexturePro(*activeTex,
                           { 0, 0, static_cast<float>(activeTex->width), static_cast<float>(activeTex->height) },
                           { iconBox.x + 45.0f, iconBox.y + 45.0f, 70.0f, 70.0f },
                           { 35.0f, 35.0f }, 0.0f, WHITE);
        } else {
            DrawText("PIEZA", static_cast<int>(iconBox.x + 18), static_cast<int>(iconBox.y + 36), 16, GRAY);
        }

        DrawText(item.part.name.c_str(), 160, static_cast<int>(cardY + 16), 22, WHITE);
        DrawText(item.part.nasaRef.c_str(), 160, static_cast<int>(cardY + 44), 15, SKYBLUE);
        DrawText(TextFormat("Masa Seca: %.1f t", item.part.dryMassTons), 160, static_cast<int>(cardY + 75), 17, LIGHTGRAY);

        if (item.part.fuelMassTons > 0.0f) {
            DrawText(TextFormat("Combustible: %.0f t", item.part.fuelMassTons), 160, static_cast<int>(cardY + 100), 17, ORANGE);
        } else if (item.part.thrustKN > 0.0f) {
            DrawText(TextFormat("Empuje Motor: %.0f kN", item.part.thrustKN), 160, static_cast<int>(cardY + 100), 17, ORANGE);
        } else {
            DrawText("Soporte vital y telemetría lunar", 160, static_cast<int>(cardY + 100), 17, LIGHTGRAY);
        }

        Rectangle mainBtn   = { 590.0f, cardY + 25.0f, 195.0f, 48.0f };
        Rectangle refundBtn = { 590.0f, cardY + 82.0f, 195.0f, 34.0f };
        bool hoverMain      = CheckCollisionPointRec(mousePos, mainBtn);

        if (!item.purchased) {
            bool canAfford = (budget >= item.part.costMillions);
            Color btnCol = canAfford ? (hoverMain ? DARKGREEN : GREEN) : Color{ 75, 35, 35, 255 };
            DrawRectangleRec(mainBtn, btnCol);
            DrawRectangleLinesEx(mainBtn, 1.5f, WHITE);
            DrawText(TextFormat("COMPRAR $%.0fM", item.part.costMillions),
                     static_cast<int>(mainBtn.x + 22), static_cast<int>(mainBtn.y + 14), 19, canAfford ? BLACK : LIGHTGRAY);
        } else {
            Color eqCol = isEquipped ? GOLD : (hoverMain ? BLUE : SKYBLUE);
            DrawRectangleRec(mainBtn, eqCol);
            DrawRectangleLinesEx(mainBtn, 1.5f, WHITE);
            DrawText(isEquipped ? "MONTADO" : "MONTAR",
                     static_cast<int>(mainBtn.x + 52), static_cast<int>(mainBtn.y + 14), 19, BLACK);

            bool hoverRef = CheckCollisionPointRec(mousePos, refundBtn);
            DrawRectangleRec(refundBtn, hoverRef ? MAROON : Color{ 60, 30, 30, 255 });
            DrawRectangleLinesEx(refundBtn, 1.0f, GRAY);
            DrawText(TextFormat("Vender (+$%.0fM)", item.part.costMillions),
                     static_cast<int>(refundBtn.x + 32), static_cast<int>(refundBtn.y + 8), 16, WHITE);
        }
    }
}

void LabShopPhase::Draw(const SharedMissionData& missionData) const {
    if (texLabBg.id > 0) {
        DrawTexturePro(texLabBg,
                       { 0, 0, static_cast<float>(texLabBg.width), static_cast<float>(texLabBg.height) },
                       { 0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) },
                       { 0, 0 }, 0.0f, WHITE);
    }

    // Cabecera
    DrawRectangle(0, 0, SCREEN_WIDTH, 75, Color{ 12, 16, 26, 245 });
    DrawText("FASE 2: TIENDA DE SUMINISTROS Y LABORATORIO", 35, 23, 25, RAYWHITE);

    DrawRectangle(SCREEN_WIDTH - 410, 15, 375, 45, Color{ 22, 34, 28, 255 });
    DrawRectangleLines(SCREEN_WIDTH - 410, 15, 375, 45, GREEN);
    DrawText(TextFormat("FONDOS DISPONIBLES: $%.1fM", missionData.remainingBudget),
             SCREEN_WIDTH - 392, 26, 22, GREEN);

    // Pestañas izquierda
    const char* tabNames[3] = { "1. CÁPSULAS", "2. TANQUES", "3. MOTORES" };
    for (int t = 0; t < 3; ++t) {
        Rectangle tabRect = { 35.0f + t * 260.0f, 105.0f, 245.0f, 48.0f };
        bool isActive = (activeTab == t);
        DrawRectangleRec(tabRect, isActive ? Color{ 45, 75, 115, 255 } : Color{ 24, 29, 42, 255 });
        DrawRectangleLinesEx(tabRect, isActive ? 2.5f : 1.0f, isActive ? GOLD : GRAY);
        DrawText(tabNames[t], static_cast<int>(tabRect.x + 48), static_cast<int>(tabRect.y + 14), 20, isActive ? GOLD : WHITE);
    }

    if (activeTab == 0)      DrawShopList(capsuleItems, equippedCapsuleIdx, missionData.remainingBudget);
    else if (activeTab == 1) DrawShopList(tankItems,    equippedTankIdx,    missionData.remainingBudget);
    else if (activeTab == 2) DrawShopList(engineItems,  equippedEngineIdx,  missionData.remainingBudget);

    DrawRectangle(35, 645, 775, 42, Color{ 18, 22, 33, 240 });
    DrawRectangleLines(35, 645, 775, 42, DARKGRAY);
    DrawText(shopFeedback.c_str(), 50, 656, 18, feedbackColor);

    // =========================================================
    // ZONA DERECHA: LABORATORIO DE MONTAJE + ESCÁNER
    // =========================================================
    DrawRectangle(845, 95, 405, 592, Color{ 20, 25, 38, 245 });
    DrawRectangleLinesEx({ 845, 95, 405, 592 }, 2, SKYBLUE);
    DrawText("ZONA DE MONTAJE", 955, 108, 20, SKYBLUE);

    // Fondo estilo plano técnico (Blueprint) detrás del cohete
    DrawRectangle(925, 138, 245, 250, Color{ 14, 28, 48, 255 });
    for (int gx = 945; gx < 1170; gx += 25) DrawLine(gx, 138, gx, 388, Fade(SKYBLUE, 0.15f));
    for (int gy = 158; gy < 388; gy += 25) DrawLine(925, gy, 1170, gy, Fade(SKYBLUE, 0.15f));

    float centerX = 1047.0f;

    // 1. Cápsula
    if (equippedCapsuleIdx >= 0) {
        if (texCapsule.id > 0) {
            DrawTexturePro(texCapsule, { 0, 0, static_cast<float>(texCapsule.width), static_cast<float>(texCapsule.height) },
                           { centerX, 178.0f, 70.0f, 65.0f }, { 35.0f, 32.5f }, 0.0f, WHITE);
        } else {
            DrawTriangle({ centerX, 146.0f }, { centerX - 35.0f, 210.0f }, { centerX + 35.0f, 210.0f }, WHITE);
        }
    } else {
        DrawRectangleLines(static_cast<int>(centerX - 35), 150, 70, 60, GRAY);
        DrawText("SIN CÁPSULA", static_cast<int>(centerX - 42), 173, 12, LIGHTGRAY);
    }

    // 2. Tanque
    if (equippedTankIdx >= 0) {
        if (texTank.id > 0) {
            DrawTexturePro(texTank, { 0, 0, static_cast<float>(texTank.width), static_cast<float>(texTank.height) },
                           { centerX, 275.0f, 70.0f, 120.0f }, { 35.0f, 60.0f }, 0.0f, WHITE);
        } else {
            DrawRectangle(static_cast<int>(centerX - 35), 213, 70, 120, LIGHTGRAY);
            DrawText("NASA", static_cast<int>(centerX - 22), 265, 16, BLUE);
        }
    } else {
        DrawRectangleLines(static_cast<int>(centerX - 35), 213, 70, 120, GRAY);
        DrawText("SIN TANQUE", static_cast<int>(centerX - 38), 268, 12, LIGHTGRAY);
    }

    // 3. Motor
    if (equippedEngineIdx >= 0) {
        if (texEngine.id > 0) {
            DrawTexturePro(texEngine, { 0, 0, static_cast<float>(texEngine.width), static_cast<float>(texEngine.height) },
                           { centerX, 358.0f, 56.0f, 45.0f }, { 28.0f, 22.5f }, 0.0f, WHITE);
        } else {
            DrawTriangle({ centerX, 333.0f }, { centerX - 28.0f, 378.0f }, { centerX + 28.0f, 378.0f }, ORANGE);
        }
    } else {
        DrawRectangleLines(static_cast<int>(centerX - 28), 336, 56, 42, GRAY);
        DrawText("SIN MOTOR", static_cast<int>(centerX - 34), 351, 12, LIGHTGRAY);
    }

    // Efecto visual de Escáner Láser al montar pieza
    if (scanAnimTimer > 0.0f) {
        float laserY = 140.0f + (1.0f - scanAnimTimer) * 245.0f;
        DrawRectangle(925, static_cast<int>(laserY - 6), 245, 12, Fade(SKYBLUE, 0.35f));
        DrawLineEx({ 925.0f, laserY }, { 1170.0f, laserY }, 3.0f, WHITE);
    }

    // Telemetría y Barra de Delta-V (Alcance Lunar)
    const RocketStats& rk = missionData.rocket;
    float wetMass = rk.dryMassTons + rk.maxFuelTons;

    DrawText(TextFormat("Masa Total: %.1f t  |  Empuje: %.0f kN", wetMass, rk.thrustKN), 868, 402, 16, WHITE);

    bool validTWR = rk.twr > 1.05f;
    DrawText(TextFormat("Empuje/Peso (TWR): %.2f", rk.twr), 868, 428, 18, validTWR ? GREEN : RED);

    // Barra de Delta-V (Meta lunar recomendada: ~5000 m/s)
    DrawText(TextFormat("Capacidad Delta-V: %.0f m/s", estimatedDeltaV), 868, 458, 17, SKYBLUE);
    DrawRectangle(868, 482, 360, 18, DARKGRAY);
    float dvRatio = std::clamp(estimatedDeltaV / 6500.0f, 0.0f, 1.0f);
    Color dvColor = (estimatedDeltaV >= 4500.0f) ? GREEN : ((estimatedDeltaV >= 2800.0f) ? YELLOW : ORANGE);
    DrawRectangle(868, 482, static_cast<int>(360.0f * dvRatio), 18, dvColor);
    DrawRectangleLines(868, 482, 360, 18, WHITE);
    DrawText(estimatedDeltaV >= 4500.0f ? "Alcance: ÓRBITA LUNAR" : "Alcance: SUBORBITAL / BAJO", 868, 506, 15, dvColor);

    bool allPartsMounted = (equippedCapsuleIdx >= 0) && (equippedTankIdx >= 0) && (equippedEngineIdx >= 0);

    if (!allPartsMounted) {
        DrawText("Faltan piezas por comprar/montar.", 868, 555, 17, YELLOW);
    } else if (!validTWR) {
        DrawText("TWR <= 1.05: Demasiado peso para ese motor.", 868, 555, 16, RED);
    } else {
        DrawText("SISTEMAS VERIFICADOS. LISTO.", 868, 555, 17, GREEN);
    }

    bool canLaunch = allPartsMounted && validTWR;
    Rectangle launchBtn = { 865.0f, 605.0f, 375.0f, 70.0f };
    bool hoverLaunch = CheckCollisionPointRec(GetMousePosition(), launchBtn);

    DrawRectangleRec(launchBtn, canLaunch ? (hoverLaunch ? DARKGREEN : GREEN) : DARKGRAY);
    DrawRectangleLinesEx(launchBtn, 2, WHITE);
    DrawText("IR A PLATAFORMA", static_cast<int>(launchBtn.x + 68), static_cast<int>(launchBtn.y + 23), 23, canLaunch ? BLACK : GRAY);
}
