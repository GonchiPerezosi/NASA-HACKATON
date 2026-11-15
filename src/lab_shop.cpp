#include "lab_shop.h"
#include "save_manager.h"
#include <algorithm>
#include <cmath>
#include <cctype>

static std::string ToLowerStr(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

void LabShopPhase::LoadRocketSprites() {
    if (!rocketPngs.empty() && bgLoaded) return;

    const char* searchRoot = DirectoryExists("assets") ? "assets" : ".";
    FilePathList files = LoadDirectoryFilesEx(searchRoot, nullptr, true);
    std::vector<std::string> candidatePaths;

    for (unsigned int i = 0; i < files.count; ++i) {
        std::string fullPath = files.paths[i];
        std::string ext = ToLowerStr(GetFileExtension(fullPath.c_str()));
        if (ext != ".png") continue;

        std::string fileName = ToLowerStr(GetFileName(fullPath.c_str()));

        if (fileName.find("lab") != std::string::npos) {
            if (!bgLoaded) {
                texLabBg = LoadTexture(fullPath.c_str());
                if (texLabBg.id > 0) bgLoaded = true;
            }
            continue;
        }

        if (fileName == "bg.png" || fileName == "desk.png" || fileName == "hero.png" ||
            fileName == "grey.png" || fileName == "blue.png" || fileName == "slick.png" ||
            fileName.find("espacio") != std::string::npos ||
            fileName.find("science") != std::string::npos) {
            continue;
        }

        candidatePaths.push_back(fullPath);
    }
    UnloadDirectoryFiles(files);

    std::sort(candidatePaths.begin(), candidatePaths.end());

    for (const auto& path : candidatePaths) {
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id > 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
            rocketPngs.push_back(tex);
            rocketPngNames.push_back(GetFileName(path.c_str()));
        }
    }
}

void LabShopPhase::Init() {
    LoadRocketSprites();

    selectedFilter = 0;
    scrollY = 0.0f;
    maxScrollY = 0.0f;
    draggingScrollbar = false;
    saveTimer = 0.0f;

    parts = {
        // CÁPSULAS (1..6)
        { 1, "Cápsula Mercury-Lite", PartCategory::CAPSULE, 120.0f, 2.0f, 0.5f, 0.0f, "NASA Mercury" },
        { 2, "Cápsula Gemini-X", PartCategory::CAPSULE, 210.0f, 3.8f, 1.0f, 0.0f, "NASA Gemini" },
        { 3, "Módulo Mando Apolo", PartCategory::CAPSULE, 320.0f, 5.8f, 2.2f, 0.0f, "Apollo CSM" },
        { 4, "Cápsula Orion MPCV", PartCategory::CAPSULE, 450.0f, 9.3f, 3.0f, 0.0f, "Artemis Orion" },
        { 5, "Cápsula Dragon-Lunar", PartCategory::CAPSULE, 380.0f, 7.5f, 2.5f, 0.0f, "Commercial Crew" },
        { 6, "Módulo Deep-Space", PartCategory::CAPSULE, 520.0f, 11.0f, 4.0f, 0.0f, "Lunar Gateway" },

        // MÓDULOS LUNARES (7..12)
        { 7, "Sonda Impacto Ranger", PartCategory::LANDER, 90.0f, 1.2f, 0.8f, 15.0f, "Ranger Probe" },
        { 8, "Aterrizador Surveyor", PartCategory::LANDER, 160.0f, 2.5f, 2.0f, 30.0f, "Surveyor" },
        { 9, "Módulo Lunar Eagle", PartCategory::LANDER, 340.0f, 4.5f, 10.5f, 45.0f, "Apollo LM" },
        { 10, "Aterrizador Altair", PartCategory::LANDER, 460.0f, 6.8f, 15.0f, 80.0f, "Constellation" },
        { 11, "Módulo Blue-Moon", PartCategory::LANDER, 410.0f, 5.5f, 12.5f, 65.0f, "Artemis HLS" },
        { 12, "Estación Base Móvil", PartCategory::LANDER, 580.0f, 9.0f, 18.0f, 110.0f, "Artemis Base" },

        // TANQUES DE COMBUSTIBLE (13..18)
        { 13, "Tanque Redstone S1", PartCategory::TANK, 140.0f, 4.0f, 45.0f, 0.0f, "Redstone" },
        { 14, "Tanque Titan-II", PartCategory::TANK, 240.0f, 7.5f, 110.0f, 0.0f, "Titan II" },
        { 15, "Etapa S-IVB", PartCategory::TANK, 350.0f, 11.0f, 210.0f, 0.0f, "Saturn S-IVB" },
        { 16, "Tanque Central SLS", PartCategory::TANK, 490.0f, 18.0f, 420.0f, 0.0f, "SLS Core" },
        { 17, "Tanque Saturn S-IC", PartCategory::TANK, 620.0f, 24.0f, 650.0f, 0.0f, "Saturn V S-IC" },
        { 18, "Super-Criogénico H2", PartCategory::TANK, 740.0f, 28.0f, 850.0f, 0.0f, "SLS Block 2" },

        // MOTORES PRINCIPALES (19..24)
        { 19, "Motor RL-10", PartCategory::ENGINE, 150.0f, 1.5f, 0.0f, 650.0f, "Centaur RL-10" },
        { 20, "Motor J-2", PartCategory::ENGINE, 260.0f, 3.2f, 0.0f, 1400.0f, "Saturn J-2" },
        { 21, "Cluster 3x RS-25", PartCategory::ENGINE, 420.0f, 6.5f, 0.0f, 4200.0f, "Space Shuttle / SLS" },
        { 22, "Cluster 4x RS-25", PartCategory::ENGINE, 540.0f, 8.5f, 0.0f, 6800.0f, "Artemis SLS" },
        { 23, "Cluster 5x F-1", PartCategory::ENGINE, 690.0f, 14.0f, 0.0f, 11500.0f, "Saturn V F-1" },
        { 24, "Motor F-1B Avanzado", PartCategory::ENGINE, 810.0f, 15.5f, 0.0f, 14500.0f, "Pyrios F-1B" },

        // PROPULSORES LATERALES (25..30)
        { 25, "Sin Propulsores", PartCategory::BOOSTER, 0.0f, 0.0f, 0.0f, 0.0f, "Configuración Base" },
        { 26, "2x Castor-30", PartCategory::BOOSTER, 130.0f, 3.0f, 25.0f, 1200.0f, "Antares Castor" },
        { 27, "2x Gem-63XL", PartCategory::BOOSTER, 220.0f, 5.5f, 55.0f, 2400.0f, "Vulcan GEM" },
        { 28, "2x SRB 4-Segmentos", PartCategory::BOOSTER, 340.0f, 9.0f, 110.0f, 4500.0f, "Shuttle SRB" },
        { 29, "2x SRB 5-Segmentos", PartCategory::BOOSTER, 460.0f, 12.5f, 160.0f, 6800.0f, "Artemis SLS SRB" },
        { 30, "2x BOLE Compuesto", PartCategory::BOOSTER, 590.0f, 14.0f, 210.0f, 8600.0f, "SLS Block 2 BOLE" }
    };
}

void LabShopPhase::OnEnter(SharedMissionData& data) {
    if (parts.empty()) Init();
    selectedFilter = 0;
    scrollY = 0.0f;
    data.remainingBudget = data.awardedBudget;
    data.eqCapsuleIdx = -1;
    data.eqLanderIdx  = -1;
    data.eqTankIdx    = -1;
    data.eqEngineIdx  = -1;
    data.eqBoosterIdx = 24;
    RecalculateRocket(data);
}

void LabShopPhase::RestoreFromSave(SharedMissionData& data) {
    if (parts.empty()) Init();
    if (data.eqBoosterIdx == 0) data.eqBoosterIdx = 24;
    RecalculateRocket(data);
}

std::vector<int> LabShopPhase::GetFilteredPartIndices() const {
    std::vector<int> list;
    for (int i = 0; i < (int)parts.size(); ++i) {
        if (selectedFilter == 0) {
            list.push_back(i);
        } else {
            PartCategory target = static_cast<PartCategory>(selectedFilter - 1);
            if (parts[i].category == target) {
                list.push_back(i);
            }
        }
    }
    return list;
}

void LabShopPhase::RecalculateRocket(SharedMissionData& data) {
    RocketStats st{};
    if (data.eqCapsuleIdx >= 0 && data.eqCapsuleIdx < (int)parts.size()) st.capsule = parts[data.eqCapsuleIdx];
    if (data.eqLanderIdx  >= 0 && data.eqLanderIdx  < (int)parts.size()) st.lander  = parts[data.eqLanderIdx];
    if (data.eqTankIdx    >= 0 && data.eqTankIdx    < (int)parts.size()) st.tank    = parts[data.eqTankIdx];
    if (data.eqEngineIdx  >= 0 && data.eqEngineIdx  < (int)parts.size()) st.engine  = parts[data.eqEngineIdx];
    if (data.eqBoosterIdx >= 0 && data.eqBoosterIdx < (int)parts.size()) st.booster = parts[data.eqBoosterIdx];

    st.totalCost    = st.capsule.costMillions + st.lander.costMillions + st.tank.costMillions + st.engine.costMillions + st.booster.costMillions;
    st.dryMassTons  = st.capsule.dryMassTons  + st.lander.dryMassTons  + st.tank.dryMassTons  + st.engine.dryMassTons  + st.booster.dryMassTons;
    st.maxFuelTons  = st.capsule.fuelMassTons + st.lander.fuelMassTons + st.tank.fuelMassTons + st.engine.fuelMassTons + st.booster.fuelMassTons;
    st.thrustKN     = st.capsule.thrustKN     + st.lander.thrustKN     + st.tank.thrustKN     + st.engine.thrustKN     + st.booster.thrustKN;

    float totalMass = st.dryMassTons + st.maxFuelTons;
    st.twr = (totalMass > 0.1f) ? (st.thrustKN / (totalMass * 9.81f)) : 0.0f;

    if (st.dryMassTons > 0.1f && totalMass > st.dryMassTons) {
        st.deltaV = 3150.0f * std::log(totalMass / st.dryMassTons);
    } else {
        st.deltaV = 0.0f;
    }

    data.rocket = st;
    data.remainingBudget = data.awardedBudget - st.totalCost;
}

int LabShopPhase::GetActiveRocketSpriteIndex(const SharedMissionData& data) const {
    if (rocketPngs.empty()) return -1;
    if (rocketPngs.size() == 1) return 0;

    int score = 0;
    if (data.eqCapsuleIdx >= 0) score += (data.eqCapsuleIdx % 6) + 1;
    if (data.eqLanderIdx  >= 0) score += (data.eqLanderIdx  % 6) + 1;
    if (data.eqTankIdx    >= 0) score += (data.eqTankIdx    % 6) + 1;
    if (data.eqEngineIdx  >= 0) score += (data.eqEngineIdx  % 6) + 1;
    if (data.eqBoosterIdx > 24) score += (data.eqBoosterIdx - 24);

    float ratio = std::clamp(score / 26.0f, 0.0f, 0.999f);
    int idx = static_cast<int>(ratio * rocketPngs.size());
    return std::clamp(idx, 0, static_cast<int>(rocketPngs.size()) - 1);
}

bool LabShopPhase::CanReachLunarOrbit(const SharedMissionData& data, std::string& outReason) const {
    int coreCount = (data.eqCapsuleIdx >= 0) + (data.eqLanderIdx >= 0) + (data.eqTankIdx >= 0) + (data.eqEngineIdx >= 0);
    if (coreCount < 4) {
        outReason = TextFormat(Tr(data.lang,
            "NO VIABLE · Faltan módulos base (%d/4 equipados)",
            "NOT VIABLE · Missing core modules (%d/4 equipped)"), coreCount);
        return false;
    }
    if (data.rocket.twr < 1.20f) {
        outReason = TextFormat(Tr(data.lang,
            "NO VIABLE · Empuje insuficiente (TWR %.2f < 1,20)",
            "NOT VIABLE · Insufficient thrust (TWR %.2f < 1.20)"), data.rocket.twr);
        return false;
    }
    if (data.rocket.deltaV < 5800.0f) {
        outReason = TextFormat(Tr(data.lang,
            "NO VIABLE · Tanque pequeño para órbita lunar (%.0f / 5800 m/s)",
            "NOT VIABLE · Too little fuel for lunar orbit (%.0f / 5800 m/s)"), data.rocket.deltaV);
        return false;
    }

    outReason = TextFormat(Tr(data.lang,
        "¡APTO PARA ÓRBITA LUNAR! (TWR %.2f · Delta-V %.0f m/s)",
        "LUNAR ORBIT CAPABLE! (TWR %.2f · Delta-V %.0f m/s)"), data.rocket.twr, data.rocket.deltaV);
    return true;
}

GameState LabShopPhase::Update(SharedMissionData& data) {
    Vector2 mouse = GetMousePosition();
    float dt = GetFrameTime();
    if (saveTimer > 0.0f) saveTimer = std::max(0.0f, saveTimer - dt);

    if (IsKeyPressed(KEY_M)) return GameState::MENU;

    Rectangle shopViewport = { 20.0f, 118.0f, 610.0f, 582.0f };
    Rectangle scrollTrack  = { 614.0f, 124.0f, 12.0f, 570.0f };

    std::vector<int> visibleIndices = GetFilteredPartIndices();
    const float cardH = 98.0f;
    const float cardGap = 10.0f;
    float contentH = visibleIndices.size() * (cardH + cardGap) + 8.0f;
    maxScrollY = std::max(0.0f, contentH - shopViewport.height);

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        scrollY -= wheel * 62.0f;
    }
    if (IsKeyDown(KEY_DOWN)) scrollY += 350.0f * dt;
    if (IsKeyDown(KEY_UP))   scrollY -= 350.0f * dt;
    scrollY = std::clamp(scrollY, 0.0f, maxScrollY);

    if (maxScrollY > 0.0f) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, scrollTrack)) {
            draggingScrollbar = true;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            draggingScrollbar = false;
        }
        if (draggingScrollbar) {
            float ratio = (mouse.y - scrollTrack.y) / scrollTrack.height;
            scrollY = std::clamp(ratio * maxScrollY, 0.0f, maxScrollY);
        }
    } else {
        draggingScrollbar = false;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle btnMenu   = { 20.0f, 14.0f, 100.0f, 34.0f };
        Rectangle btnSave   = { 130.0f, 14.0f, 120.0f, 34.0f };
        Rectangle btnLaunch = { 660.0f, 636.0f, 585.0f, 52.0f };

        if (CheckCollisionPointRec(mouse, btnMenu)) return GameState::MENU;
        if (CheckCollisionPointRec(mouse, btnSave)) {
            SaveManager::SaveGame(data.activeSaveSlot, data, GameState::HANGAR);
            saveTimer = 2.0f;
            return GameState::HANGAR;
        }

        for (int f = 0; f < 6; ++f) {
            Rectangle tab = { 20.0f + f * 102.0f, 72.0f, 98.0f, 36.0f };
            if (CheckCollisionPointRec(mouse, tab)) {
                selectedFilter = f;
                scrollY = 0.0f;
                return GameState::HANGAR;
            }
        }

        if (CheckCollisionPointRec(mouse, shopViewport) && !CheckCollisionPointRec(mouse, scrollTrack)) {
            for (int row = 0; row < (int)visibleIndices.size(); ++row) {
                int partIdx = visibleIndices[row];
                float cardY = shopViewport.y + 8.0f + row * (cardH + cardGap) - scrollY;
                Rectangle card = { shopViewport.x + 8.0f, cardY, 580.0f, cardH };

                if (card.y + card.height < shopViewport.y || card.y > shopViewport.y + shopViewport.height) {
                    continue;
                }

                if (CheckCollisionPointRec(mouse, card)) {
                    PartCategory cat = parts[partIdx].category;
                    int* targetSlot = &data.eqCapsuleIdx;
                    if (cat == PartCategory::LANDER)  targetSlot = &data.eqLanderIdx;
                    if (cat == PartCategory::TANK)    targetSlot = &data.eqTankIdx;
                    if (cat == PartCategory::ENGINE)  targetSlot = &data.eqEngineIdx;
                    if (cat == PartCategory::BOOSTER) targetSlot = &data.eqBoosterIdx;

                    float oldCost = (*targetSlot >= 0) ? parts[*targetSlot].costMillions : 0.0f;
                    if (data.remainingBudget + oldCost >= parts[partIdx].costMillions) {
                        *targetSlot = partIdx;
                        RecalculateRocket(data);
                    }
                    break;
                }
            }
        }

        bool ready = (data.eqCapsuleIdx >= 0 && data.eqLanderIdx >= 0 && data.eqTankIdx >= 0 && data.eqEngineIdx >= 0);
        if (ready && CheckCollisionPointRec(mouse, btnLaunch)) {
            return GameState::FLIGHT;
        }
    }

    return GameState::HANGAR;
}

void LabShopPhase::Draw(const SharedMissionData& data) {
    if (bgLoaded && texLabBg.id > 0) {
        DrawTexturePro(texLabBg, { 0, 0, (float)texLabBg.width, (float)texLabBg.height },
                       { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, { 0, 0 }, 0.0f, Color{ 85, 92, 110, 255 });
    } else {
        ClearBackground(Color{ 12, 18, 32, 255 });
    }

    DrawRectangle(0, 0, SCREEN_WIDTH, 60, Color{ 10, 15, 28, 245 });
    DrawRectangle(20, 14, 100, 34, Color{ 25, 35, 60, 255 });
    DrawRectangleLines(20, 14, 100, 34, GOLD);
    DrawUIText("MENÚ", 44.0f, 21.0f, 18.0f, WHITE);

    DrawRectangle(130, 14, 120, 34, Color{ 38, 125, 62, 255 });
    DrawRectangleLines(130, 14, 120, 34, GOLD);
    DrawUIText(Tr(data.lang, "GUARDAR", "SAVE"), 150.0f, 21.0f, 18.0f, WHITE);

    DrawUIText(Tr(data.lang, "FASE 2 · HANGAR Y TIENDA DE COMPONENTES", "PHASE 2 · ROCKET HANGAR & PARTS SHOP"), 275.0f, 18.0f, 24.0f, GOLD);
    DrawUIText(TextFormat(Tr(data.lang, "FONDOS: $%.1fM", "FUNDS: $%.1fM"), data.remainingBudget), 1020.0f, 18.0f, 24.0f, GREEN);

    if (saveTimer > 0.0f) {
        DrawUIText(Tr(data.lang, "¡Guardado!", "Saved!"), 895.0f, 20.0f, 20.0f, SKYBLUE);
    }

    const char* filterEs[6] = { "TODAS", "CÁPSULAS", "LUNAR", "TANQUES", "MOTORES", "BOOSTERS" };
    const char* filterEn[6] = { "ALL", "CAPSULES", "LANDERS", "TANKS", "ENGINES", "BOOSTERS" };
    for (int f = 0; f < 6; ++f) {
        Rectangle tab = { 20.0f + f * 102.0f, 72.0f, 98.0f, 36.0f };
        bool active = (selectedFilter == f);
        DrawRectangleRec(tab, active ? Color{ 45, 95, 165, 255 } : Color{ 20, 27, 45, 245 });
        DrawRectangleLinesEx(tab, 2.0f, active ? GOLD : GRAY);
        DrawUIText(Tr(data.lang, filterEs[f], filterEn[f]), tab.x + 10.0f, tab.y + 9.0f, 15.0f, WHITE);
    }

    Rectangle shopViewport = { 20.0f, 118.0f, 610.0f, 582.0f };
    DrawRectangleRec(shopViewport, Color{ 12, 18, 34, 244 });
    DrawRectangleLinesEx(shopViewport, 2.0f, GOLD);

    std::vector<int> visibleIndices = GetFilteredPartIndices();
    const float cardH = 98.0f;
    const float cardGap = 10.0f;

    BeginScissorMode((int)shopViewport.x + 2, (int)shopViewport.y + 2, (int)shopViewport.width - 20, (int)shopViewport.height - 4);
    for (int row = 0; row < (int)visibleIndices.size(); ++row) {
        int i = visibleIndices[row];
        // En ALL (selectedFilter == 0) numera 1..30; en categorias individuales numera 1..6
        int displayNum = (selectedFilter == 0) ? parts[i].id : (row + 1);

        float cardY = shopViewport.y + 8.0f + row * (cardH + cardGap) - scrollY;
        Rectangle card = { shopViewport.x + 8.0f, cardY, 580.0f, cardH };

        if (card.y + card.height < shopViewport.y || card.y > shopViewport.y + shopViewport.height) {
            continue;
        }

        bool equipped = (i == data.eqCapsuleIdx || i == data.eqLanderIdx || i == data.eqTankIdx || i == data.eqEngineIdx || i == data.eqBoosterIdx);

        const char* catBadge = "CÁPSULA";
        Color badgeCol = SKYBLUE;
        if (parts[i].category == PartCategory::LANDER)  { catBadge = "MOD. LUNAR"; badgeCol = GOLD; }
        if (parts[i].category == PartCategory::TANK)    { catBadge = "TANQUE";     badgeCol = ORANGE; }
        if (parts[i].category == PartCategory::ENGINE)  { catBadge = "MOTOR";      badgeCol = Color{ 255, 110, 110, 255 }; }
        if (parts[i].category == PartCategory::BOOSTER) { catBadge = "BOOSTER";    badgeCol = Color{ 180, 140, 255, 255 }; }

        DrawRectangleRec(card, equipped ? Color{ 26, 68, 44, 252 } : Color{ 18, 25, 44, 248 });
        DrawRectangleLinesEx(card, equipped ? 2.5f : 1.5f, equipped ? GREEN : Color{ 70, 115, 175, 255 });

        DrawUIText(TextFormat("#%d · %s", displayNum, parts[i].name.c_str()), card.x + 12.0f, card.y + 8.0f, 20.0f, GOLD);
        DrawUIText(catBadge, card.x + 350.0f, card.y + 10.0f, 15.0f, badgeCol);
        DrawUIText(TextFormat("$%.0fM", parts[i].costMillions), card.x + 490.0f, card.y + 8.0f, 21.0f, GREEN);

        DrawUIText(TextFormat(Tr(data.lang,
                   "Masa seca: %.1f t  |  Combustible: %.1f t  |  Empuje: %.0f kN",
                   "Dry mass: %.1f t  |  Propellant: %.1f t  |  Thrust: %.0f kN"),
                   parts[i].dryMassTons, parts[i].fuelMassTons, parts[i].thrustKN),
                   card.x + 12.0f, card.y + 40.0f, 17.0f, WHITE);

        DrawUIText(TextFormat("Ref: %s", parts[i].nasaRef.c_str()), card.x + 12.0f, card.y + 68.0f, 16.0f, LIGHTGRAY);

        if (equipped) {
            DrawUIText(Tr(data.lang, "[EQUIPADA]", "[EQUIPPED]"), card.x + 460.0f, card.y + 66.0f, 17.0f, GREEN);
        } else {
            DrawUIText(Tr(data.lang, "Clic para equipar", "Click to equip"), card.x + 435.0f, card.y + 66.0f, 16.0f, SKYBLUE);
        }
    }
    EndScissorMode();

    Rectangle scrollTrack = { 614.0f, 124.0f, 12.0f, 570.0f };
    DrawRectangleRec(scrollTrack, Color{ 22, 30, 52, 255 });
    if (maxScrollY > 0.0f) {
        float visibleRatio = std::clamp(shopViewport.height / (shopViewport.height + maxScrollY), 0.12f, 1.0f);
        float thumbH = scrollTrack.height * visibleRatio;
        float scrollRatio = scrollY / maxScrollY;
        float thumbY = scrollTrack.y + scrollRatio * (scrollTrack.height - thumbH);
        DrawRectangle((int)scrollTrack.x + 1, (int)thumbY, (int)scrollTrack.width - 2, (int)thumbH, GOLD);
    } else {
        DrawRectangle((int)scrollTrack.x + 1, (int)scrollTrack.y, (int)scrollTrack.width - 2, (int)scrollTrack.height, GRAY);
    }

    Rectangle rocketShowcase = { 645.0f, 72.0f, 615.0f, 470.0f };
    DrawRectangleRec(rocketShowcase, Color{ 9, 15, 32, 248 });
    DrawRectangleLinesEx(rocketShowcase, 2.5f, SKYBLUE);

    for (int gx = (int)rocketShowcase.x + 20; gx < (int)(rocketShowcase.x + rocketShowcase.width); gx += 28) {
        DrawLine(gx, (int)rocketShowcase.y + 42, gx, (int)(rocketShowcase.y + rocketShowcase.height - 46), Color{ 22, 42, 74, 115 });
    }
    for (int gy = (int)rocketShowcase.y + 44; gy < (int)(rocketShowcase.y + rocketShowcase.height - 46); gy += 28) {
        DrawLine((int)rocketShowcase.x + 8, gy, (int)rocketShowcase.x + rocketShowcase.width - 8, gy, Color{ 22, 42, 74, 115 });
    }

    DrawUIText(Tr(data.lang, "VISTA PREVIA DEL COHETE", "ROCKET VISUAL PREVIEW"),
               rocketShowcase.x + 18.0f, rocketShowcase.y + 10.0f, 21.0f, SKYBLUE);

    int spriteIdx = GetActiveRocketSpriteIndex(data);
    if (spriteIdx >= 0 && spriteIdx < (int)rocketPngs.size()) {
        Texture2D tex = rocketPngs[spriteIdx];

        float maxW = 330.0f;
        float maxH = 365.0f;
        float scale = std::min(maxW / (float)tex.width, maxH / (float)tex.height);
        float drawW = tex.width * scale;
        float drawH = tex.height * scale;
        float drawX = rocketShowcase.x + 25.0f + (maxW - drawW) * 0.5f;
        float drawY = rocketShowcase.y + 48.0f + (maxH - drawH) * 0.5f;

        DrawTexturePro(tex,
                       { 0, 0, (float)tex.width, (float)tex.height },
                       { drawX, drawY, drawW, drawH },
                       { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawUITextBoxed(Tr(data.lang,
            "Añade tus PNGs de cohete en assets/sprites/ para ver el modelo aquí.",
            "Place your rocket PNGs inside assets/sprites/ to preview the model here."),
            { rocketShowcase.x + 35.0f, rocketShowcase.y + 180.0f, 300.0f, 120.0f }, 18.0f, 4.0f, ORANGE);
    }

    float infoX = rocketShowcase.x + 365.0f;
    DrawLine((int)infoX - 12, (int)rocketShowcase.y + 44, (int)infoX - 12, (int)rocketShowcase.y + 418, Color{ 45, 70, 110, 255 });

    auto drawEquippedItem = [&](const char* label, const std::string& val, bool ok, float y) {
        DrawUIText(label, infoX, y, 15.0f, SKYBLUE);
        DrawUIText(ok ? val.c_str() : Tr(data.lang, "--- Sin equipar ---", "--- Unequipped ---"),
                   infoX, y + 18.0f, 17.0f, ok ? WHITE : ORANGE);
    };

    drawEquippedItem(Tr(data.lang, "1. CÁPSULA:", "1. CAPSULE:"), data.rocket.capsule.name, data.eqCapsuleIdx >= 0, rocketShowcase.y + 52.0f);
    drawEquippedItem(Tr(data.lang, "2. MÓDULO LUNAR:", "2. LUNAR LANDER:"), data.rocket.lander.name, data.eqLanderIdx >= 0, rocketShowcase.y + 114.0f);
    drawEquippedItem(Tr(data.lang, "3. TANQUE:", "3. FUEL TANK:"), data.rocket.tank.name, data.eqTankIdx >= 0, rocketShowcase.y + 176.0f);
    drawEquippedItem(Tr(data.lang, "4. MOTOR:", "4. ENGINE:"), data.rocket.engine.name, data.eqEngineIdx >= 0, rocketShowcase.y + 238.0f);
    drawEquippedItem(Tr(data.lang, "5. PROPULSORES:", "5. BOOSTERS:"), data.rocket.booster.name, true, rocketShowcase.y + 300.0f);

    DrawUIText(TextFormat(Tr(data.lang, "Coste: $%.0fM", "Cost: $%.0fM"), data.rocket.totalCost),
               infoX, rocketShowcase.y + 368.0f, 20.0f, GOLD);

    DrawRectangle((int)rocketShowcase.x + 8, (int)rocketShowcase.y + 422, (int)rocketShowcase.width - 16, 40, Color{ 16, 26, 52, 255 });
    DrawUIText(TextFormat(Tr(data.lang,
               "Masa Seca: %.1f t   |   Fuel: %.1f t   |   Empuje: %.0f kN   |   TWR: %.2f",
               "Dry Mass: %.1f t   |   Fuel: %.1f t   |   Thrust: %.0f kN   |   TWR: %.2f"),
               data.rocket.dryMassTons, data.rocket.maxFuelTons, data.rocket.thrustKN, data.rocket.twr),
               rocketShowcase.x + 20.0f, rocketShowcase.y + 432.0f, 17.0f, WHITE);

    Rectangle bottomRight = { 645.0f, 552.0f, 615.0f, 148.0f };
    DrawRectangleRec(bottomRight, Color{ 12, 18, 34, 248 });
    DrawRectangleLinesEx(bottomRight, 2.5f, GOLD);

    std::string orbitReason;
    bool viable = CanReachLunarOrbit(data, orbitReason);

    Rectangle statusBanner = { 660.0f, 564.0f, 585.0f, 58.0f };
    DrawRectangleRec(statusBanner, viable ? Color{ 22, 74, 42, 255 } : Color{ 82, 28, 28, 255 });
    DrawRectangleLinesEx(statusBanner, 2.0f, viable ? GREEN : RED);

    DrawUIText(viable ? Tr(data.lang, "ESTADO ORBITAL: SÍ PUEDE ALCANZAR ÓRBITA LUNAR", "ORBITAL STATUS: CAN REACH LUNAR ORBIT")
                      : Tr(data.lang, "ESTADO ORBITAL: NO PUEDE ALCANZAR ÓRBITA LUNAR", "ORBITAL STATUS: CANNOT REACH LUNAR ORBIT"),
               statusBanner.x + 14.0f, statusBanner.y + 8.0f, 18.0f, viable ? GREEN : Color{ 255, 130, 130, 255 });

    DrawUIText(orbitReason.c_str(), statusBanner.x + 14.0f, statusBanner.y + 32.0f, 16.0f, WHITE);

    bool ready = (data.eqCapsuleIdx >= 0 && data.eqLanderIdx >= 0 && data.eqTankIdx >= 0 && data.eqEngineIdx >= 0);
    Rectangle btnLaunch = { 660.0f, 636.0f, 585.0f, 52.0f };
    DrawRectangleRec(btnLaunch, ready ? (viable ? Color{ 45, 145, 76, 255 } : Color{ 185, 115, 25, 255 }) : Color{ 55, 60, 72, 255 });
    DrawRectangleLinesEx(btnLaunch, 2.5f, ready ? WHITE : GRAY);

    const char* launchText = Tr(data.lang, "EQUIPA LAS 4 PIEZAS BASE PARA DESPEGAR", "EQUIP 4 CORE PARTS TO LAUNCH");
    if (ready) {
        launchText = viable ? Tr(data.lang, "IR A LA LUNA >", "GO TO THE MOON >")
                            : Tr(data.lang, "IR A LA LUNA (RIESGO DE FALLO ORBITAL) >", "GO TO THE MOON (ORBITAL FAILURE RISK) >");
    }

    float txtW = MeasureUIText(launchText, 22.0f);
    DrawUIText(launchText, btnLaunch.x + (btnLaunch.width - txtW) * 0.5f, btnLaunch.y + 14.0f, 22.0f, ready ? WHITE : LIGHTGRAY);
}

void LabShopPhase::Unload() {
    if (bgLoaded && texLabBg.id > 0) {
        UnloadTexture(texLabBg);
        bgLoaded = false;
    }
    for (auto& tex : rocketPngs) {
        if (tex.id > 0) UnloadTexture(tex);
    }
    rocketPngs.clear();
    rocketPngNames.clear();
}