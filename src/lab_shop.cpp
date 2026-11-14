#include "lab_shop.h"

void LabShopPhase::Init() {
    activeTab = 0;
    equippedCapsuleIdx = -1;
    equippedTankIdx    = -1;
    equippedEngineIdx  = -1;
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

    // Catálogo de la Tienda (todas empiezan con purchased = false)
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
    // Si pruebas esta pantalla sin pasar antes por el jurado, asigna $120M de prueba
    if (missionData.awardedBudget <= 0.0f) {
        missionData.awardedBudget = 120.0f;
    }
    missionData.remainingBudget = missionData.awardedBudget;
    RecalculateRocketStats(missionData);
}

void LabShopPhase::RecalculateRocketStats(SharedMissionData& missionData) {
    RocketStats& r = missionData.rocket;
    r = RocketStats{}; // Reiniciar estadísticas antes de sumar las piezas equipadas

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
}

void LabShopPhase::HandleShopInteraction(std::vector<ShopItem>& currentList, int& equippedIdx, SharedMissionData& missionData) {
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
    Vector2 mousePos = GetMousePosition();

    for (size_t i = 0; i < currentList.size(); ++i) {
        float cardY = 175.0f + i * 155.0f;

        // Botón principal de la tarjeta (Comprar o Equipar)
        Rectangle mainActionBtn = { 590.0f, cardY + 25.0f, 195.0f, 48.0f };
        // Botón secundario para devolver/vender pieza comprada
        Rectangle refundBtn     = { 590.0f, cardY + 82.0f, 195.0f, 34.0f };

        if (CheckCollisionPointRec(mousePos, mainActionBtn)) {
            if (!currentList[i].purchased) {
                // Lógica de COMPRA
                float price = currentList[i].part.costMillions;
                if (missionData.remainingBudget >= price) {
                    missionData.remainingBudget -= price;
                    currentList[i].purchased = true;
                    equippedIdx = static_cast<int>(i); // Se equipa automáticamente al comprar
                    RecalculateRocketStats(missionData);

                    shopFeedback = "Comprado y equipado: " + currentList[i].part.name;
                    feedbackColor = GREEN;
                } else {
                    shopFeedback = "Presupuesto insuficiente para " + currentList[i].part.name;
                    feedbackColor = RED;
                }
            } else {
                // Lógica de EQUIPAR (si ya estaba comprada)
                equippedIdx = static_cast<int>(i);
                RecalculateRocketStats(missionData);
                shopFeedback = "Pieza montada en el cohete: " + currentList[i].part.name;
                feedbackColor = SKYBLUE;
            }
        }
        else if (currentList[i].purchased && CheckCollisionPointRec(mousePos, refundBtn)) {
            // Lógica de VENTA / REEMBOLSO
            missionData.remainingBudget += currentList[i].part.costMillions;
            currentList[i].purchased = false;
            if (equippedIdx == static_cast<int>(i)) {
                equippedIdx = -1;
            }
            RecalculateRocketStats(missionData);
            shopFeedback = "Pieza devuelta. Reembolsados $" + std::to_string((int)currentList[i].part.costMillions) + "M";
            feedbackColor = ORANGE;
        }
    }
}

GameState LabShopPhase::Update(SharedMissionData& missionData) {
    Vector2 mousePos = GetMousePosition();

    // 1. Cambio de pestañas de la tienda
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int t = 0; t < 3; ++t) {
            Rectangle tabRect = { 35.0f + t * 260.0f, 105.0f, 245.0f, 48.0f };
            if (CheckCollisionPointRec(mousePos, tabRect)) {
                activeTab = t;
            }
        }
    }

    // 2. Interacción con los artículos de la pestaña activa
    if (activeTab == 0)      HandleShopInteraction(capsuleItems, equippedCapsuleIdx, missionData);
    else if (activeTab == 1) HandleShopInteraction(tankItems,    equippedTankIdx,    missionData);
    else if (activeTab == 2) HandleShopInteraction(engineItems,  equippedEngineIdx,  missionData);

    // 3. Validación para pasar a la Fase 3 (Misión)
    bool allPartsMounted = (equippedCapsuleIdx >= 0) && (equippedTankIdx >= 0) && (equippedEngineIdx >= 0);
    bool validTWR        = missionData.rocket.twr > 1.05f;
    bool canLaunch       = allPartsMounted && validTWR;

    Rectangle launchBtn = { 865.0f, 595.0f, 375.0f, 75.0f };
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

        // Miniatura / Hueco de sprite de la pieza en la tienda
        Rectangle iconBox = { 52.0f, cardY + 24.0f, 90.0f, 90.0f };
        DrawRectangleRec(iconBox, Color{ 16, 20, 30, 255 });
        DrawRectangleLinesEx(iconBox, 1.0f, DARKGRAY);

        const Texture2D* activeTex = nullptr;
        if (item.part.category == PartCategory::CAPSULE && texCapsule.id > 0) activeTex = &texCapsule;
        if (item.part.category == PartCategory::TANK    && texTank.id > 0)    activeTex = &texTank;
        if (item.part.category == PartCategory::ENGINE  && texEngine.id > 0)  activeTex = &texEngine;

        if (activeTex != nullptr) {
            DrawTexturePro(*activeTex,
                           { 0, 0, (float)activeTex->width, (float)activeTex->height },
                           { iconBox.x + 45.0f, iconBox.y + 45.0f, 70.0f, 70.0f },
                           { 35.0f, 35.0f }, 0.0f, WHITE);
        } else {
            DrawText("IMG", (int)iconBox.x + 28, (int)iconBox.y + 36, 18, GRAY);
        }

        // Datos técnicos de la pieza
        DrawText(item.part.name.c_str(), 160, (int)cardY + 16, 22, WHITE);
        DrawText(item.part.nasaRef.c_str(), 160, (int)cardY + 44, 15, SKYBLUE);

        DrawText(TextFormat("Masa Seca: %.1f t", item.part.dryMassTons), 160, (int)cardY + 75, 17, LIGHTGRAY);

        if (item.part.fuelMassTons > 0.0f) {
            DrawText(TextFormat("Capacidad Combustible: %.0f t", item.part.fuelMassTons), 160, (int)cardY + 100, 17, ORANGE);
        } else if (item.part.thrustKN > 0.0f) {
            DrawText(TextFormat("Empuje Motor: %.0f kN", item.part.thrustKN), 160, (int)cardY + 100, 17, ORANGE);
        } else {
            DrawText("Soporte vital y telemetría lunar", 160, (int)cardY + 100, 17, LIGHTGRAY);
        }

        // Botones de acción a la derecha de la tarjeta
        Rectangle mainBtn   = { 590.0f, cardY + 25.0f, 195.0f, 48.0f };
        Rectangle refundBtn = { 590.0f, cardY + 82.0f, 195.0f, 34.0f };
        bool hoverMain      = CheckCollisionPointRec(mousePos, mainBtn);

        if (!item.purchased) {
            bool canAfford = (budget >= item.part.costMillions);
            Color btnCol = canAfford ? (hoverMain ? DARKGREEN : GREEN) : Color{ 75, 35, 35, 255 };
            DrawRectangleRec(mainBtn, btnCol);
            DrawRectangleLinesEx(mainBtn, 1.5f, WHITE);
            DrawText(TextFormat("COMPRAR $%.0fM", item.part.costMillions),
                     (int)mainBtn.x + 22, (int)mainBtn.y + 14, 19, canAfford ? BLACK : LIGHTGRAY);
        } else {
            Color eqCol = isEquipped ? GOLD : (hoverMain ? BLUE : SKYBLUE);
            DrawRectangleRec(mainBtn, eqCol);
            DrawRectangleLinesEx(mainBtn, 1.5f, WHITE);
            DrawText(isEquipped ? "MONTADO" : "MONTAR",
                     (int)mainBtn.x + 52, (int)mainBtn.y + 14, 19, BLACK);

            // Botón devolver
            bool hoverRef = CheckCollisionPointRec(mousePos, refundBtn);
            DrawRectangleRec(refundBtn, hoverRef ? MAROON : Color{ 60, 30, 30, 255 });
            DrawRectangleLinesEx(refundBtn, 1.0f, GRAY);
            DrawText(TextFormat("Vender (+$%.0fM)", item.part.costMillions),
                     (int)refundBtn.x + 32, (int)refundBtn.y + 8, 16, WHITE);
        }
    }
}

void LabShopPhase::Draw(const SharedMissionData& missionData) const {
    if (texLabBg.id > 0) {
        DrawTexturePro(texLabBg,
                       { 0, 0, (float)texLabBg.width, (float)texLabBg.height },
                       { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT },
                       { 0, 0 }, 0.0f, WHITE);
    }

    // Cabecera superior con Presupuesto
    DrawRectangle(0, 0, SCREEN_WIDTH, 75, Color{ 12, 16, 26, 245 });
    DrawText("FASE 2: TIENDA DE SUMINISTROS Y LABORATORIO", 35, 23, 25, RAYWHITE);

    DrawRectangle(SCREEN_WIDTH - 410, 15, 375, 45, Color{ 22, 34, 28, 255 });
    DrawRectangleLines(SCREEN_WIDTH - 410, 15, 375, 45, GREEN);
    DrawText(TextFormat("FONDOS DISPONIBLES: $%.1fM", missionData.remainingBudget),
             SCREEN_WIDTH - 392, 26, 22, GREEN);

    // =========================================================
    // ZONA IZQUIERDA: TIENDA DE PIEZAS
    // =========================================================
    const char* tabNames[3] = { "1. CÁPSULAS", "2. TANQUES", "3. MOTORES" };
    for (int t = 0; t < 3; ++t) {
        Rectangle tabRect = { 35.0f + t * 260.0f, 105.0f, 245.0f, 48.0f };
        bool isActive = (activeTab == t);
        DrawRectangleRec(tabRect, isActive ? Color{ 45, 75, 115, 255 } : Color{ 24, 29, 42, 255 });
        DrawRectangleLinesEx(tabRect, isActive ? 2.5f : 1.0f, isActive ? GOLD : GRAY);
        DrawText(tabNames[t], (int)tabRect.x + 48, (int)tabRect.y + 14, 20, isActive ? GOLD : WHITE);
    }

    if (activeTab == 0)      DrawShopList(capsuleItems, equippedCapsuleIdx, missionData.remainingBudget);
    else if (activeTab == 1) DrawShopList(tankItems,    equippedTankIdx,    missionData.remainingBudget);
    else if (activeTab == 2) DrawShopList(engineItems,  equippedEngineIdx,  missionData.remainingBudget);

    // Barra inferior de notificaciones de la tienda
    DrawRectangle(35, 645, 775, 42, Color{ 18, 22, 33, 240 });
    DrawRectangleLines(35, 645, 775, 42, DARKGRAY);
    DrawText(shopFeedback.c_str(), 50, 656, 18, feedbackColor);

    // =========================================================
    // ZONA DERECHA: LABORATORIO DE MONTAJE DEL COHETE
    // =========================================================
    DrawRectangle(845, 95, 405, 592, Color{ 20, 25, 38, 245 });
    DrawRectangleLinesEx({ 845, 95, 405, 592 }, 2, SKYBLUE);
    DrawText("ZONA DE MONTAJE", 955, 110, 20, SKYBLUE);

    float centerX = 1047.0f;

    // Ranura 1: Cápsula
    if (equippedCapsuleIdx >= 0) {
        if (texCapsule.id > 0) {
            DrawTexturePro(texCapsule, { 0, 0, (float)texCapsule.width, (float)texCapsule.height },
                           { centerX, 180.0f, 70.0f, 65.0f }, { 35.0f, 32.5f }, 0.0f, WHITE);
        } else {
            DrawTriangle({ centerX, 148.0f }, { centerX - 35.0f, 212.0f }, { centerX + 35.0f, 212.0f }, WHITE);
        }
    } else {
        DrawRectangleLines((int)centerX - 35, 152, 70, 60, DARKGRAY);
        DrawText("SIN CÁPSULA", (int)centerX - 42, 175, 12, GRAY);
    }

    // Ranura 2: Tanque
    if (equippedTankIdx >= 0) {
        if (texTank.id > 0) {
            DrawTexturePro(texTank, { 0, 0, (float)texTank.width, (float)texTank.height },
                           { centerX, 278.0f, 70.0f, 125.0f }, { 35.0f, 62.5f }, 0.0f, WHITE);
        } else {
            DrawRectangle((int)centerX - 35, 215, 70, 125, LIGHTGRAY);
            DrawText("NASA", (int)centerX - 22, 268, 16, BLUE);
        }
    } else {
        DrawRectangleLines((int)centerX - 35, 215, 70, 125, DARKGRAY);
        DrawText("SIN TANQUE", (int)centerX - 38, 270, 12, GRAY);
    }

    // Ranura 3: Motor
    if (equippedEngineIdx >= 0) {
        if (texEngine.id > 0) {
            DrawTexturePro(texEngine, { 0, 0, (float)texEngine.width, (float)texEngine.height },
                           { centerX, 365.0f, 56.0f, 45.0f }, { 28.0f, 22.5f }, 0.0f, WHITE);
        } else {
            DrawTriangle({ centerX, 340.0f }, { centerX - 28.0f, 385.0f }, { centerX + 28.0f, 385.0f }, ORANGE);
        }
    } else {
        DrawRectangleLines((int)centerX - 28, 343, 56, 42, DARKGRAY);
        DrawText("SIN MOTOR", (int)centerX - 34, 358, 12, GRAY);
    }

    // Resumen técnico de la nave ensamblada
    const RocketStats& rk = missionData.rocket;
    float wetMass = rk.dryMassTons + rk.maxFuelTons;

    DrawLine(865, 405, 1230, 405, DARKGRAY);
    DrawText(TextFormat("Masa Total:        %.1f t", wetMass), 870, 420, 18, WHITE);
    DrawText(TextFormat("Combustible:       %.1f t", rk.maxFuelTons), 870, 448, 18, ORANGE);
    DrawText(TextFormat("Empuje Total:     %.0f kN", rk.thrustKN), 870, 476, 18, SKYBLUE);

    bool allPartsMounted = (equippedCapsuleIdx >= 0) && (equippedTankIdx >= 0) && (equippedEngineIdx >= 0);
    bool validTWR        = rk.twr > 1.05f;

    DrawText(TextFormat("Empuje/Peso (TWR): %.2f", rk.twr), 870, 506, 19, validTWR ? GREEN : RED);

    if (!allPartsMounted) {
        DrawText("Faltan piezas por comprar/montar.", 870, 548, 17, YELLOW);
    } else if (!validTWR) {
        DrawText("TWR insuficiente (necesitas > 1.05).", 870, 548, 17, RED);
    } else {
        DrawText("COHETE LISTO PARA LANZAMIENTO.", 870, 548, 17, GREEN);
    }

    // Botón Lanzar Misión
    bool canLaunch = allPartsMounted && validTWR;
    Rectangle launchBtn = { 865.0f, 595.0f, 375.0f, 75.0f };
    bool hoverLaunch = CheckCollisionPointRec(GetMousePosition(), launchBtn);

    DrawRectangleRec(launchBtn, canLaunch ? (hoverLaunch ? DARKGREEN : GREEN) : DARKGRAY);
    DrawRectangleLinesEx(launchBtn, 2, WHITE);
    DrawText("IR A PLATAFORMA", (int)launchBtn.x + 68, (int)launchBtn.y + 25, 23, canLaunch ? BLACK : GRAY);
}
