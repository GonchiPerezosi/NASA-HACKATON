#include "lab_shop.h"
#include <cmath>
#include <algorithm>

void LabShopPhase::Init() {
    activeTab = 0;
    scrollOffset = 0.0f;
    equippedCapsuleIdx = -1;
    equippedLanderIdx  = -1;
    equippedTankIdx    = -1;
    equippedEngineIdx  = -1;
    equippedBoosterIdx = 0;
    scanAnimTimer      = 0.0f;
    estimatedDeltaV    = 0.0f;

    shopFeedback = "Usa la rueda del raton en la lista para ver las 30 piezas.";
    feedbackColor = LIGHTGRAY;

    // 1. CAPSULAS (6 opciones)
    capsuleItems = {
        { { 101, "Computadora Sputnik-X",  PartCategory::CAPSULE,  7.0f, 0.6f, 0.0f, 0.0f, "Vuelo autonomo sin tripulacion" }, false, { 0 }, LIGHTGRAY },
        { { 102, "Capsula Mercury-Lite",   PartCategory::CAPSULE, 14.0f, 1.5f, 0.0f, 0.0f, "1 astronauta - Proyecto Mercury" }, false, { 0 }, WHITE },
        { { 103, "Capsula Gemini-2",       PartCategory::CAPSULE, 22.0f, 3.0f, 0.0f, 0.0f, "2 astronautas - Maniobras orbitales" }, false, { 0 }, SKYBLUE },
        { { 104, "Modulo Apolo CSM",       PartCategory::CAPSULE, 32.0f, 5.0f, 0.0f, 0.0f, "3 astronautas - Programa Apolo" }, false, { 0 }, WHITE },
        { { 105, "Capsula Dragon-Lunar",   PartCategory::CAPSULE, 42.0f, 6.4f, 0.0f, 0.0f, "4 astronautas - Reutilizable" }, false, { 0 }, Color{ 220, 235, 255, 255 } },
        { { 106, "Capsula Orion MPCV",     PartCategory::CAPSULE, 54.0f, 8.2f, 0.0f, 0.0f, "Espacio profundo - Programa Artemis" }, false, { 0 }, GOLD }
    };

    // 2. MODULO LUNAR / CARGA UTIL (6 opciones)
    landerItems = {
        { { 201, "CubeSat Espectrometro",  PartCategory::LANDER,  6.0f, 0.4f, 0.0f, 0.0f, "Micro-satelite de orbita lunar" }, false, { 0 }, SKYBLUE },
        { { 202, "Sonda Lunar Ranger",     PartCategory::LANDER, 12.0f, 1.2f, 0.0f, 0.0f, "Impactador y mapeo fotografico" }, false, { 0 }, LIGHTGRAY },
        { { 203, "Rover Lunokhod Mini",    PartCategory::LANDER, 19.0f, 2.3f, 0.0f, 0.0f, "Vehiculo robotico de exploracion" }, false, { 0 }, ORANGE },
        { { 204, "Modulo Lunar Eagle",     PartCategory::LANDER, 28.0f, 3.8f, 0.0f, 0.0f, "Alunizaje tripulado Apollo 11" }, false, { 0 }, GOLD },
        { { 205, "Laboratorio VIPER-Lab",  PartCategory::LANDER, 37.0f, 5.1f, 0.0f, 0.0f, "Perforacion de hielo polar lunar" }, false, { 0 }, WHITE },
        { { 206, "Aterrizador Artemis HLS",PartCategory::LANDER, 48.0f, 6.8f, 0.0f, 0.0f, "Base lunar de gran capacidad" }, false, { 0 }, Color{ 200, 240, 255, 255 } }
    };

    // 3. TANQUES DE PROPELENTE (6 opciones)
    tankItems = {
        { { 301, "Tanque Vanguard Mini",   PartCategory::TANK,    10.0f, 1.0f, 13.0f, 0.0f, "Propelente ligero RP-1" }, false, { 0 }, LIGHTGRAY },
        { { 302, "Tanque Delta-II",        PartCategory::TANK,    18.0f, 2.0f, 24.0f, 0.0f, "Tanque medio estandar" }, false, { 0 }, SKYBLUE },
        { { 303, "Etapa Centaur LH2",      PartCategory::TANK,    26.0f, 2.6f, 35.0f, 0.0f, "Alta eficiencia criogenica" }, false, { 0 }, WHITE },
        { { 304, "Etapa S-IVB Saturno",    PartCategory::TANK,    36.0f, 4.0f, 49.0f, 0.0f, "Inyeccion translunar clasica" }, false, { 0 }, WHITE },
        { { 305, "Tanque Falcon Heavy",    PartCategory::TANK,    46.0f, 5.3f, 65.0f, 0.0f, "Alta densidad RP-1 subenfriado" }, false, { 0 }, Color{ 230, 230, 240, 255 } },
        { { 306, "Tanque Gigante Core SLS",PartCategory::TANK,    58.0f, 6.8f, 86.0f, 0.0f, "Maxima capacidad Hidrogeno Liquido" }, false, { 0 }, ORANGE }
    };

    // 4. MOTORES PRINCIPALES (6 opciones)
    engineItems = {
        { { 401, "Motor Kestrel Vacio",    PartCategory::ENGINE,  12.0f, 0.6f, 0.0f,  480.0f, "Motor ligero para sondas (480 kN)" }, false, { 0 }, LIGHTGRAY },
        { { 402, "Motor Merlin-1D",        PartCategory::ENGINE,  20.0f, 1.0f, 0.0f,  920.0f, "Fiable y economico (920 kN)" }, false, { 0 }, WHITE },
        { { 403, "Motor RL-10B Criogenico",PartCategory::ENGINE,  29.0f, 1.3f, 0.0f, 1250.0f, "Tobera extensible (1250 kN)" }, false, { 0 }, SKYBLUE },
        { { 404, "Motor J-2 Hidrolox",     PartCategory::ENGINE,  38.0f, 1.8f, 0.0f, 1600.0f, "Motor lunar Saturno V (1600 kN)" }, false, { 0 }, WHITE },
        { { 405, "Motor Raptor-Vac Metano",PartCategory::ENGINE,  49.0f, 2.4f, 0.0f, 2050.0f, "Ciclo escalonado (2050 kN)" }, false, { 0 }, Color{ 210, 235, 255, 255 } },
        { { 406, "Cluster 4x RS-25D",      PartCategory::ENGINE,  62.0f, 3.6f, 0.0f, 2650.0f, "Potencia maxima SLS (2650 kN)" }, false, { 0 }, GOLD }
    };

    // 5. BOOSTERS LATERALES (6 opciones)
    boosterItems = {
        { { 501, "Sin Boosters (Solo Core)",PartCategory::BOOSTER,  0.0f, 0.0f,  0.0f,    0.0f, "Configuracion limpia sin acoples" }, true,  { 0 }, GRAY },
        { { 502, "2x Micro-Boosters Castor",PartCategory::BOOSTER, 10.0f, 0.8f,  6.0f,  360.0f, "Ayuda extra en plataforma" }, false, { 0 }, LIGHTGRAY },
        { { 503, "2x Boosters GEM-63",      PartCategory::BOOSTER, 18.0f, 1.5f, 12.0f,  680.0f, "Aceleradores solidos Atlas" }, false, { 0 }, WHITE },
        { { 504, "2x Boosters Liquidos URM",PartCategory::BOOSTER, 26.0f, 2.2f, 18.0f,  980.0f, "Propulsion lateral Kerolox" }, false, { 0 }, SKYBLUE },
        { { 505, "2x SRB Transbordador",    PartCategory::BOOSTER, 35.0f, 3.0f, 25.0f, 1300.0f, "Cohetes solidos de gran empuje" }, false, { 0 }, WHITE },
        { { 506, "2x SRB SLS 5-Segmentos",  PartCategory::BOOSTER, 45.0f, 3.8f, 33.0f, 1650.0f, "Los boosters mas potentes creados" }, false, { 0 }, ORANGE }
    };

    texLabBg = LoadTexture("assets/sprites/Laboratory.png");
}

void LabShopPhase::Unload() {
    if (texLabBg.id > 0) UnloadTexture(texLabBg);

    std::vector<unsigned int> unloadedIds;
    auto safeUnloadList = [&](std::vector<ShopItem>& list) {
        for (auto& item : list) {
            if (item.texture.id > 0 && std::find(unloadedIds.begin(), unloadedIds.end(), item.texture.id) == unloadedIds.end()) {
                unloadedIds.push_back(item.texture.id);
                UnloadTexture(item.texture);
            }
        }
    };

    safeUnloadList(capsuleItems);
    safeUnloadList(landerItems);
    safeUnloadList(tankItems);
    safeUnloadList(engineItems);
    safeUnloadList(boosterItems);
}

void LabShopPhase::OnEnter(SharedMissionData& missionData) {
    if (missionData.awardedBudget <= 0.0f) {
        missionData.awardedBudget = 145.0f;
    }
    missionData.remainingBudget = missionData.awardedBudget;
    RecalculateRocketStats(missionData);
}

void LabShopPhase::RecalculateRocketStats(SharedMissionData& missionData) {
    RocketStats& r = missionData.rocket;
    r = RocketStats{};

    RocketPart pCap = (equippedCapsuleIdx >= 0) ? capsuleItems[equippedCapsuleIdx].part : RocketPart{};
    RocketPart pLan = (equippedLanderIdx >= 0)  ? landerItems[equippedLanderIdx].part   : RocketPart{};
    RocketPart pTnk = (equippedTankIdx >= 0)    ? tankItems[equippedTankIdx].part       : RocketPart{};
    RocketPart pEng = (equippedEngineIdx >= 0)  ? engineItems[equippedEngineIdx].part   : RocketPart{};
    RocketPart pBst = (equippedBoosterIdx >= 0) ? boosterItems[equippedBoosterIdx].part : RocketPart{};

    r.capsule = pCap;
    r.lander  = pLan;
    r.tank    = pTnk;
    r.engine  = pEng;
    r.booster = pBst;

    r.totalCost   = pCap.costMillions + pLan.costMillions + pTnk.costMillions + pEng.costMillions + pBst.costMillions;
    r.dryMassTons = pCap.dryMassTons  + pLan.dryMassTons  + pTnk.dryMassTons  + pEng.dryMassTons  + pBst.dryMassTons;
    r.maxFuelTons = pTnk.fuelMassTons + pBst.fuelMassTons;
    r.thrustKN    = pEng.thrustKN     + pBst.thrustKN;

    float totalWetMass  = r.dryMassTons + r.maxFuelTons;
    float weightEarthKN = totalWetMass * 9.81f;
    r.twr = (weightEarthKN > 0.0f) ? (r.thrustKN / weightEarthKN) : 0.0f;

    if (r.dryMassTons > 0.0f && r.maxFuelTons > 0.0f && r.thrustKN > 0.0f) {
        float isp = 315.0f + (pEng.thrustKN / 2650.0f) * 140.0f;
        estimatedDeltaV = isp * 9.81f * std::log(totalWetMass / r.dryMassTons);
    } else {
        estimatedDeltaV = 0.0f;
    }
}

void LabShopPhase::HandleShopInteraction(std::vector<ShopItem>& currentList, int& equippedIdx, bool isOptional, SharedMissionData& missionData) {
    Vector2 mousePos = GetMousePosition();

    if (mousePos.x >= 35.0f && mousePos.x <= 815.0f && mousePos.y >= 165.0f && mousePos.y <= 635.0f) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            float totalContentHeight = currentList.size() * 138.0f;
            float visibleHeight = 470.0f;
            float maxScroll = std::max(0.0f, totalContentHeight - visibleHeight);
            scrollOffset = std::clamp(scrollOffset - wheel * 42.0f, 0.0f, maxScroll);
        }
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    if (mousePos.y < 165.0f || mousePos.y > 635.0f) return;

    for (size_t i = 0; i < currentList.size(); ++i) {
        float cardY = 172.0f + i * 138.0f - scrollOffset;
        Rectangle mainActionBtn = { 580.0f, cardY + 20.0f, 195.0f, 44.0f };
        Rectangle refundBtn     = { 580.0f, cardY + 72.0f, 195.0f, 32.0f };

        if (CheckCollisionPointRec(mousePos, mainActionBtn)) {
            if (!currentList[i].purchased) {
                float price = currentList[i].part.costMillions;
                if (missionData.remainingBudget >= price) {
                    missionData.remainingBudget -= price;
                    currentList[i].purchased = true;
                    equippedIdx = static_cast<int>(i);
                    scanAnimTimer = 1.0f;
                    RecalculateRocketStats(missionData);

                    shopFeedback = "Comprado y montado: " + currentList[i].part.name;
                    feedbackColor = GREEN;
                } else {
                    shopFeedback = "Saldo insuficiente para " + currentList[i].part.name;
                    feedbackColor = RED;
                }
            } else {
                equippedIdx = static_cast<int>(i);
                scanAnimTimer = 1.0f;
                RecalculateRocketStats(missionData);
                shopFeedback = "Pieza equipada: " + currentList[i].part.name;
                feedbackColor = SKYBLUE;
            }
        }
        else if (currentList[i].purchased && currentList[i].part.costMillions > 0.0f && CheckCollisionPointRec(mousePos, refundBtn)) {
            missionData.remainingBudget += currentList[i].part.costMillions;
            currentList[i].purchased = false;
            if (equippedIdx == static_cast<int>(i)) {
                equippedIdx = isOptional ? 0 : -1;
            }
            RecalculateRocketStats(missionData);
            shopFeedback = "Pieza vendida. Reembolso: +$" + std::to_string(static_cast<int>(currentList[i].part.costMillions)) + "M";
            feedbackColor = ORANGE;
        }
    }
}

GameState LabShopPhase::Update(SharedMissionData& missionData) {
    Vector2 mousePos = GetMousePosition();

    if (scanAnimTimer > 0.0f) {
        scanAnimTimer = std::max(0.0f, scanAnimTimer - GetFrameTime() * 1.6f);
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int t = 0; t < 5; ++t) {
            Rectangle tabRect = { 35.0f + t * 156.0f, 105.0f, 150.0f, 48.0f };
            if (CheckCollisionPointRec(mousePos, tabRect)) {
                activeTab = t;
                scrollOffset = 0.0f;
            }
        }
    }

    if (activeTab == 0)      HandleShopInteraction(capsuleItems, equippedCapsuleIdx, false, missionData);
    else if (activeTab == 1) HandleShopInteraction(landerItems,  equippedLanderIdx,  false, missionData);
    else if (activeTab == 2) HandleShopInteraction(tankItems,    equippedTankIdx,    false, missionData);
    else if (activeTab == 3) HandleShopInteraction(engineItems,  equippedEngineIdx,  false, missionData);
    else if (activeTab == 4) HandleShopInteraction(boosterItems, equippedBoosterIdx, true,  missionData);

    bool coreMounted = (equippedCapsuleIdx >= 0) && (equippedLanderIdx >= 0) &&
                       (equippedTankIdx >= 0)    && (equippedEngineIdx >= 0);
    bool validTWR    = missionData.rocket.twr > 1.05f;
    bool canLaunch   = coreMounted && validTWR;

    Rectangle launchBtn = { 865.0f, 605.0f, 375.0f, 70.0f };
    bool clickedLaunch  = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, launchBtn);

    if (canLaunch && (clickedLaunch || IsKeyPressed(KEY_ENTER))) {
        return GameState::FLIGHT;
    }

    return GameState::HANGAR;
}

void LabShopPhase::DrawShopList(const std::vector<ShopItem>& currentList, int equippedIdx, float budget) const {
    Vector2 mousePos = GetMousePosition();

    BeginScissorMode(35, 165, 785, 470);

    for (size_t i = 0; i < currentList.size(); ++i) {
        const auto& item = currentList[i];
        float cardY = 172.0f + i * 138.0f - scrollOffset;
        Rectangle cardRect = { 35.0f, cardY, 760.0f, 124.0f };

        bool isEquipped = (static_cast<int>(i) == equippedIdx);
        Color borderCol = isEquipped ? GOLD : (item.purchased ? SKYBLUE : GRAY);

        DrawRectangleRec(cardRect, Color{ 25, 31, 46, 240 });
        DrawRectangleLinesEx(cardRect, isEquipped ? 3.0f : 1.5f, borderCol);

        Rectangle iconBox = { 50.0f, cardY + 18.0f, 86.0f, 86.0f };
        DrawRectangleRec(iconBox, Color{ 16, 20, 30, 255 });
        DrawRectangleLinesEx(iconBox, 1.0f, DARKGRAY);

        if (item.texture.id > 0) {
            DrawTexturePro(item.texture,
                           { 0, 0, static_cast<float>(item.texture.width), static_cast<float>(item.texture.height) },
                           { iconBox.x + 43.0f, iconBox.y + 43.0f, 70.0f, 70.0f },
                           { 35.0f, 35.0f }, 0.0f, item.tint);
        } else {
            DrawText(TextFormat("#%d", (int)i + 1), static_cast<int>(iconBox.x + 30), static_cast<int>(iconBox.y + 32), 20, item.tint);
        }

        DrawText(item.part.name.c_str(), 152, static_cast<int>(cardY + 14), 20, WHITE);
        DrawText(item.part.nasaRef.c_str(), 152, static_cast<int>(cardY + 40), 14, SKYBLUE);
        DrawText(TextFormat("Masa: +%.1f t", item.part.dryMassTons), 152, static_cast<int>(cardY + 68), 16, LIGHTGRAY);

        if (item.part.fuelMassTons > 0.0f && item.part.thrustKN > 0.0f) {
            DrawText(TextFormat("Combustible: +%.0ft | Empuje: +%.0fkN", item.part.fuelMassTons, item.part.thrustKN),
                     152, static_cast<int>(cardY + 92), 15, ORANGE);
        } else if (item.part.fuelMassTons > 0.0f) {
            DrawText(TextFormat("Combustible: +%.0f t", item.part.fuelMassTons), 152, static_cast<int>(cardY + 92), 16, ORANGE);
        } else if (item.part.thrustKN > 0.0f) {
            DrawText(TextFormat("Empuje: +%.0f kN", item.part.thrustKN), 152, static_cast<int>(cardY + 92), 16, ORANGE);
        } else {
            DrawText("Modulo de mision lunar", 152, static_cast<int>(cardY + 92), 16, LIGHTGRAY);
        }

        Rectangle mainBtn   = { 580.0f, cardY + 20.0f, 195.0f, 44.0f };
        Rectangle refundBtn = { 580.0f, cardY + 72.0f, 195.0f, 32.0f };
        bool inClipArea     = (mousePos.y >= 165.0f && mousePos.y <= 635.0f);
        bool hoverMain      = inClipArea && CheckCollisionPointRec(mousePos, mainBtn);

        if (!item.purchased) {
            bool canAfford = (budget >= item.part.costMillions);
            Color btnCol = canAfford ? (hoverMain ? DARKGREEN : GREEN) : Color{ 75, 35, 35, 255 };
            DrawRectangleRec(mainBtn, btnCol);
            DrawRectangleLinesEx(mainBtn, 1.5f, WHITE);
            DrawText(TextFormat("COMPRAR $%.0fM", item.part.costMillions),
                     static_cast<int>(mainBtn.x + 22), static_cast<int>(mainBtn.y + 12), 18, canAfford ? BLACK : LIGHTGRAY);
        } else {
            Color eqCol = isEquipped ? GOLD : (hoverMain ? BLUE : SKYBLUE);
            DrawRectangleRec(mainBtn, eqCol);
            DrawRectangleLinesEx(mainBtn, 1.5f, WHITE);
            DrawText(isEquipped ? "MONTADO" : "MONTAR",
                     static_cast<int>(mainBtn.x + 52), static_cast<int>(mainBtn.y + 12), 18, BLACK);

            if (item.part.costMillions > 0.0f) {
                bool hoverRef = inClipArea && CheckCollisionPointRec(mousePos, refundBtn);
                DrawRectangleRec(refundBtn, hoverRef ? MAROON : Color{ 60, 30, 30, 255 });
                DrawRectangleLinesEx(refundBtn, 1.0f, GRAY);
                DrawText(TextFormat("Vender (+$%.0fM)", item.part.costMillions),
                         static_cast<int>(refundBtn.x + 32), static_cast<int>(refundBtn.y + 7), 15, WHITE);
            }
        }
    }

    EndScissorMode();

    float totalContentHeight = currentList.size() * 138.0f;
    float visibleHeight = 470.0f;
    if (totalContentHeight > visibleHeight) {
        DrawRectangle(804, 170, 8, 460, Color{ 20, 24, 35, 255 });
        float thumbHeight = (visibleHeight / totalContentHeight) * 460.0f;
        float maxScroll = totalContentHeight - visibleHeight;
        float thumbY = 170.0f + (scrollOffset / maxScroll) * (460.0f - thumbHeight);
        DrawRectangle(804, static_cast<int>(thumbY), 8, static_cast<int>(thumbHeight), SKYBLUE);
    }
}

void LabShopPhase::Draw(const SharedMissionData& missionData) const {
    if (texLabBg.id > 0) {
        DrawTexturePro(texLabBg,
                       { 0, 0, static_cast<float>(texLabBg.width), static_cast<float>(texLabBg.height) },
                       { 0, 0, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) },
                       { 0, 0 }, 0.0f, Fade(WHITE, 0.35f));
    }

    DrawRectangle(0, 0, SCREEN_WIDTH, 75, Color{ 12, 16, 26, 245 });
    DrawText("FASE 2: TIENDA Y LABORATORIO LUNAR (30 PIEZAS)", 35, 23, 24, RAYWHITE);

    DrawRectangle(SCREEN_WIDTH - 410, 15, 375, 45, Color{ 22, 34, 28, 255 });
    DrawRectangleLines(SCREEN_WIDTH - 410, 15, 375, 45, GREEN);
    DrawText(TextFormat("FONDOS DISPONIBLES: $%.1fM", missionData.remainingBudget),
             SCREEN_WIDTH - 392, 26, 22, GREEN);

    const char* tabNames[5] = { "1. CAPSULA", "2. M. LUNAR", "3. TANQUE", "4. MOTOR", "5. BOOSTERS" };
    for (int t = 0; t < 5; ++t) {
        Rectangle tabRect = { 35.0f + t * 156.0f, 105.0f, 150.0f, 48.0f };
        bool isActive = (activeTab == t);
        DrawRectangleRec(tabRect, isActive ? Color{ 45, 75, 115, 255 } : Color{ 24, 29, 42, 255 });
        DrawRectangleLinesEx(tabRect, isActive ? 2.5f : 1.0f, isActive ? GOLD : GRAY);
        DrawText(tabNames[t], static_cast<int>(tabRect.x + 14), static_cast<int>(tabRect.y + 15), 16, isActive ? GOLD : WHITE);
    }

    if (activeTab == 0)      DrawShopList(capsuleItems, equippedCapsuleIdx, missionData.remainingBudget);
    else if (activeTab == 1) DrawShopList(landerItems,  equippedLanderIdx,  missionData.remainingBudget);
    else if (activeTab == 2) DrawShopList(tankItems,    equippedTankIdx,    missionData.remainingBudget);
    else if (activeTab == 3) DrawShopList(engineItems,  equippedEngineIdx,  missionData.remainingBudget);
    else if (activeTab == 4) DrawShopList(boosterItems, equippedBoosterIdx, missionData.remainingBudget);

    DrawRectangle(35, 645, 777, 42, Color{ 18, 22, 33, 240 });
    DrawRectangleLines(35, 645, 777, 42, DARKGRAY);
    DrawText(shopFeedback.c_str(), 50, 656, 17, feedbackColor);

    DrawRectangle(845, 95, 405, 592, Color{ 20, 25, 38, 245 });
    DrawRectangleLinesEx({ 845, 95, 405, 592 }, 2, SKYBLUE);
    DrawText("ZONA DE MONTAJE", 955, 108, 20, SKYBLUE);

    DrawRectangle(925, 136, 245, 255, Color{ 14, 28, 48, 255 });
    for (int gx = 945; gx < 1170; gx += 25) DrawLine(gx, 136, gx, 391, Fade(SKYBLUE, 0.15f));
    for (int gy = 156; gy < 391; gy += 25) DrawLine(925, gy, 1170, gy, Fade(SKYBLUE, 0.15f));

    float centerX = 1047.0f;

    if (equippedCapsuleIdx >= 0) {
        const auto& item = capsuleItems[equippedCapsuleIdx];
        DrawTriangle({ centerX, 140.0f }, { centerX - 28.0f, 184.0f }, { centerX + 28.0f, 184.0f }, item.tint);
    } else {
        DrawRectangleLines((int)centerX - 28, 142, 56, 40, GRAY);
        DrawText("CAPSULA", (int)centerX - 26, 156, 11, LIGHTGRAY);
    }

    if (equippedLanderIdx >= 0) {
        const auto& item = landerItems[equippedLanderIdx];
        DrawRectangle((int)centerX - 30, 187, 60, 36, item.tint);
        DrawText("LEM", (int)centerX - 13, 198, 14, BLACK);
    } else {
        DrawRectangleLines((int)centerX - 30, 187, 60, 36, GRAY);
        DrawText("M. LUNAR", (int)centerX - 27, 199, 11, LIGHTGRAY);
    }

    if (equippedTankIdx >= 0) {
        const auto& item = tankItems[equippedTankIdx];
        DrawRectangle((int)centerX - 30, 227, 60, 108, item.tint);
        DrawText("NASA", (int)centerX - 20, 273, 15, BLUE);
    } else {
        DrawRectangleLines((int)centerX - 30, 227, 60, 108, GRAY);
        DrawText("TANQUE", (int)centerX - 24, 275, 11, LIGHTGRAY);
    }

    if (equippedBoosterIdx > 0) {
        const auto& item = boosterItems[equippedBoosterIdx];
        DrawRectangle((int)centerX - 50, 242, 16, 95, item.tint);
        DrawRectangle((int)centerX + 34, 242, 16, 95, item.tint);
    }

    if (equippedEngineIdx >= 0) {
        const auto& item = engineItems[equippedEngineIdx];
        DrawTriangle({ centerX, 336.0f }, { centerX - 26.0f, 378.0f }, { centerX + 26.0f, 378.0f }, item.tint);
    } else {
        DrawRectangleLines((int)centerX - 26, 338, 52, 38, GRAY);
        DrawText("MOTOR", (int)centerX - 21, 351, 11, LIGHTGRAY);
    }

    if (scanAnimTimer > 0.0f) {
        float laserY = 138.0f + (1.0f - scanAnimTimer) * 250.0f;
        DrawRectangle(925, static_cast<int>(laserY - 6), 245, 12, Fade(SKYBLUE, 0.35f));
        DrawLineEx({ 925.0f, laserY }, { 1170.0f, laserY }, 3.0f, WHITE);
    }

    const RocketStats& rk = missionData.rocket;
    float wetMass = rk.dryMassTons + rk.maxFuelTons;

    DrawText(TextFormat("Masa Total: %.1f t  |  Empuje: %.0f kN", wetMass, rk.thrustKN), 868, 402, 16, WHITE);
    bool validTWR = rk.twr > 1.05f;
    DrawText(TextFormat("Empuje/Peso (TWR): %.2f", rk.twr), 868, 428, 18, validTWR ? GREEN : RED);

    DrawText(TextFormat("Capacidad Delta-V: %.0f m/s", estimatedDeltaV), 868, 458, 17, SKYBLUE);
    DrawRectangle(868, 482, 360, 18, DARKGRAY);
    float dvRatio = std::clamp(estimatedDeltaV / 6500.0f, 0.0f, 1.0f);
    Color dvColor = (estimatedDeltaV >= 4500.0f) ? GREEN : ((estimatedDeltaV >= 2800.0f) ? YELLOW : ORANGE);
    DrawRectangle(868, 482, static_cast<int>(360.0f * dvRatio), 18, dvColor);
    DrawRectangleLines(868, 482, 360, 18, WHITE);
    DrawText(estimatedDeltaV >= 4500.0f ? "Alcance: ORBITA LUNAR" : "Alcance: SUBORBITAL / BAJO", 868, 506, 15, dvColor);

    bool coreMounted = (equippedCapsuleIdx >= 0) && (equippedLanderIdx >= 0) &&
                       (equippedTankIdx >= 0)    && (equippedEngineIdx >= 0);

    if (!coreMounted) {
        DrawText("Equipa Capsula, M. Lunar, Tanque y Motor.", 868, 555, 16, YELLOW);
    } else if (!validTWR) {
        DrawText("TWR <= 1.05: Anade Boosters o motor mayor.", 868, 555, 16, RED);
    } else {
        DrawText("COHETE LISTO PARA MISION LUNAR.", 868, 555, 17, GREEN);
    }

    bool canLaunch = coreMounted && validTWR;
    Rectangle launchBtn = { 865.0f, 605.0f, 375.0f, 70.0f };
    bool hoverLaunch = CheckCollisionPointRec(GetMousePosition(), launchBtn);

    DrawRectangleRec(launchBtn, canLaunch ? (hoverLaunch ? DARKGREEN : GREEN) : DARKGRAY);
    DrawRectangleLinesEx(launchBtn, 2, WHITE);
    DrawText("IR A PLATAFORMA", static_cast<int>(launchBtn.x + 68), static_cast<int>(launchBtn.y + 23), 23, canLaunch ? BLACK : GRAY);
}