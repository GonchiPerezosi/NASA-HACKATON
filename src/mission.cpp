#include "mission.h"
#include "save_manager.h"
#include <algorithm>
#include <cmath>

void MissionPhase::LoadSprites() {
    if (spritesLoaded) return;

    for (int i = 0; i < 6; ++i) {
        const char* pCap = TextFormat("assets/sprites/parts/cap_%d.png", i + 1);
        if (FileExists(pCap)) texCapsules[i] = LoadTexture(pCap);

        const char* pLan = TextFormat("assets/sprites/parts/lander_%d.png", i + 1);
        if (FileExists(pLan)) texLanders[i] = LoadTexture(pLan);

        const char* pTnk = TextFormat("assets/sprites/parts/tank_%d.png", i + 1);
        if (FileExists(pTnk)) texTanks[i] = LoadTexture(pTnk);

        const char* pEng = TextFormat("assets/sprites/parts/eng_%d.png", i + 1);
        if (FileExists(pEng)) texEngines[i] = LoadTexture(pEng);
    }

    for (int i = 0; i < 5; ++i) {
        const char* pBst = TextFormat("assets/sprites/parts/boost_%d.png", i + 1);
        if (FileExists(pBst)) texBoosters[i] = LoadTexture(pBst);
    }

    if (FileExists("assets/sprites/flight/launch_bg.png"))    texLaunchBg    = LoadTexture("assets/sprites/flight/launch_bg.png");
    if (FileExists("assets/sprites/flight/launch_tower.png")) texLaunchTower = LoadTexture("assets/sprites/flight/launch_tower.png");
    if (FileExists("assets/sprites/flight/flame_main_1.png")) texFlameMain1  = LoadTexture("assets/sprites/flight/flame_main_1.png");
    if (FileExists("assets/sprites/flight/flame_main_2.png")) texFlameMain2  = LoadTexture("assets/sprites/flight/flame_main_2.png");
    if (FileExists("assets/sprites/flight/flame_srb_1.png"))  texFlameSrb1   = LoadTexture("assets/sprites/flight/flame_srb_1.png");
    if (FileExists("assets/sprites/flight/flame_srb_2.png"))  texFlameSrb2   = LoadTexture("assets/sprites/flight/flame_srb_2.png");
    if (FileExists("assets/sprites/flight/smoke_cloud.png"))  texSmoke       = LoadTexture("assets/sprites/flight/smoke_cloud.png");

    spritesLoaded = true;
}

void MissionPhase::Init() {
    LoadSprites();

    stars.clear();
    stars.reserve(150);
    for (int i = 0; i < 150; ++i) {
        stars.push_back({
            static_cast<float>((i * 211 + 37) % SCREEN_WIDTH),
            static_cast<float>((i * 331 + 73) % SCREEN_HEIGHT),
            (i % 4 == 0) ? 2.4f : 1.4f,
            static_cast<unsigned char>(130 + (i * 31) % 125)
        });
    }
}

void MissionPhase::OnEnter(const SharedMissionData& data) {
    if (!spritesLoaded || stars.empty()) Init();

    stage = FlightStage::COUNTDOWN_ASCENT;
    countdownTimer = 2.6f; // Cuenta atras rapida y dinamica
    missionClock = 0.0f;
    timeWarp = 1;
    autoPilot = false;

    altitudeKm = 0.0f;
    velocityKms = 0.0f;
    fuelRemainingTons = data.rocket.maxFuelTons;
    throttle = 1.0f;
    pitchDeg = 90.0f;
    padFailTimer = 0.0f;

    boostersAttached = (data.eqBoosterIdx >= 25);
    leftBooster = SeparatedBooster{};
    rightBooster = SeparatedBooster{};

    tliCursor = 0.15f;
    tliProgress = 0.0f;
    tliBurning = false;

    landerAltMeters = 1200.0f;
    landerVelY = -26.0f;
    landerPosX = static_cast<float>((data.activeSaveSlot % 2 == 0) ? -140 : 140);
    landerVelX = 0.0f;
    landerFuelPct = 100.0f;
    touchdownSpeed = 0.0f;

    success = false;
    grade = "B";
    resultTitleEs.clear();
    resultTitleEn.clear();
    resultDetailEs.clear();
    resultDetailEn.clear();
}

void MissionPhase::FinishMission(SharedMissionData& data, bool ok, const std::string& missionGrade,
                                 const std::string& titleEs, const std::string& titleEn,
                                 const std::string& detailEs, const std::string& detailEn) {
    stage = FlightStage::DEBRIEF;
    success = ok;
    grade = missionGrade;
    resultTitleEs = titleEs;
    resultTitleEn = titleEn;
    resultDetailEs = detailEs;
    resultDetailEn = detailEn;

    data.missionSuccess = ok;
    data.endReason = Tr(data.lang, titleEs.c_str(), titleEn.c_str());
    SaveManager::SaveGame(data.activeSaveSlot, data, GameState::FLIGHT);
}

GameState MissionPhase::Update(SharedMissionData& data) {
    Vector2 mouse = GetMousePosition();
    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_M)) return GameState::MENU;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle btnMenu   = { 20.0f, 14.0f, 100.0f, 34.0f };
        Rectangle btnHangar = { 130.0f, 14.0f, 175.0f, 34.0f };
        if (CheckCollisionPointRec(mouse, btnMenu))   return GameState::MENU;
        if (CheckCollisionPointRec(mouse, btnHangar)) return GameState::HANGAR;
    }

    // ==================== 1. CUENTA ATRÁS Y ASCENSO ====================
    if (stage == FlightStage::COUNTDOWN_ASCENT) {
        if (IsKeyPressed(KEY_P)) autoPilot = !autoPilot;
        if (IsKeyPressed(KEY_SPACE)) timeWarp = (timeWarp == 1) ? 2 : 1;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Rectangle btnWarp  = { 315.0f, 14.0f, 125.0f, 34.0f };
            Rectangle btnPilot = { 450.0f, 14.0f, 175.0f, 34.0f };
            if (CheckCollisionPointRec(mouse, btnWarp))  timeWarp = (timeWarp == 1) ? 2 : 1;
            if (CheckCollisionPointRec(mouse, btnPilot)) autoPilot = !autoPilot;
        }

        float simDt = dt * static_cast<float>(timeWarp);

        if (countdownTimer > 0.0f) {
            countdownTimer = std::max(0.0f, countdownTimer - dt);
            return GameState::FLIGHT;
        }

        missionClock += simDt;

        // Controles de acelerador e inclinacion (Gravity Turn)
        if (autoPilot) {
            throttle = 1.0f;
            float targetPitch = std::clamp(90.0f - (altitudeKm / 160.0f) * 78.0f, 12.0f, 90.0f);
            pitchDeg += (targetPitch - pitchDeg) * std::min(1.0f, simDt * 2.0f);
        } else {
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))   throttle = std::min(1.0f, throttle + 0.7f * dt);
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) throttle = std::max(0.15f, throttle - 0.7f * dt);
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) pitchDeg = std::min(90.0f, pitchDeg + 28.0f * dt);
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) pitchDeg = std::max(10.0f, pitchDeg - 28.0f * dt);

            // Giro gravitatorio suave automatico si el jugador no toca A/D
            if (!IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_A) && !IsKeyDown(KEY_D)) {
                float naturalPitch = std::clamp(90.0f - (altitudeKm / 170.0f) * 75.0f, 15.0f, 90.0f);
                pitchDeg += (naturalPitch - pitchDeg) * std::min(1.0f, simDt * 0.55f);
            }
        }

        // Comprobacion de Empuje-Peso en rampa (TWR)
        float effectiveTwr = data.rocket.twr * throttle;
        if (altitudeKm <= 0.01f && effectiveTwr < 1.02f) {
            padFailTimer += simDt;
            if (padFailTimer >= 2.2f) {
                FinishMission(data, false, "F",
                    "FALLO EN RAMPA · EMPUJE INSUFICIENTE (TWR < 1,02)",
                    "PAD FAILURE · INSUFFICIENT LIFTOFF THRUST (TWR < 1.02)",
                    "Los motores rugen en la plataforma pero el cohete pesa demasiado para despegar. Vuelve al Hangar y equipa motores más potentes o propulsores laterales.",
                    "The engines roar on the pad, but the rocket is too heavy to lift off. Return to the Hangar and install stronger engines or side boosters.");
            }
            return GameState::FLIGHT;
        }

        // Separacion automatica de boosters al alcanzar 58 km
        if (boostersAttached && altitudeKm >= 58.0f) {
            boostersAttached = false;
            leftBooster  = { true, -46.0f, 0.0f, -55.0f, 95.0f, -8.0f };
            rightBooster = { true,  46.0f, 0.0f,  55.0f, 95.0f,  8.0f };
        }

        if (leftBooster.active) {
            leftBooster.offsetX += leftBooster.velX * simDt;
            leftBooster.offsetY += leftBooster.velY * simDt;
            leftBooster.velY += 140.0f * simDt;
            leftBooster.rotDeg -= 32.0f * simDt;
        }
        if (rightBooster.active) {
            rightBooster.offsetX += rightBooster.velX * simDt;
            rightBooster.offsetY += rightBooster.velY * simDt;
            rightBooster.velY += 140.0f * simDt;
            rightBooster.rotDeg += 32.0f * simDt;
        }

        // Fisica de ascenso rapido hacia orbita baja (180 km)
        float netTwr = std::max(0.15f, effectiveTwr - 0.85f);
        float rad = pitchDeg * (3.14159265f / 180.0f);
        velocityKms += netTwr * 0.62f * simDt;
        altitudeKm  += (0.8f + velocityKms * 2.4f) * std::sin(rad) * simDt * 3.8f;

        float burnRate = (data.rocket.maxFuelTons * 0.42f) / 16.0f;
        fuelRemainingTons = std::max(data.rocket.maxFuelTons * 0.45f, fuelRemainingTons - burnRate * throttle * simDt);

        if (altitudeKm >= 180.0f) {
            altitudeKm = 185.0f;
            velocityKms = std::max(velocityKms, 7.8f);
            stage = FlightStage::TLI_BURN;
            tliCursor = 0.12f;
            tliProgress = 0.0f;
        }
    }
    // ==================== 2. INYECCIÓN TRANSLUNAR (TLI) ====================
    else if (stage == FlightStage::TLI_BURN) {
        tliCursor += dt * 0.55f;
        if (tliCursor > 1.0f) tliCursor -= 1.0f;

        bool firing = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Rectangle btnBurn = { 430.0f, 610.0f, 420.0f, 56.0f };
            if (CheckCollisionPointRec(mouse, btnBurn)) firing = true;
        }
        tliBurning = firing;

        if (firing) {
            // Ventana verde optima entre 0.34 y 0.68
            bool inWindow = (tliCursor >= 0.32f && tliCursor <= 0.70f);
            float rate = inWindow ? 0.42f : 0.18f;
            tliProgress = std::min(1.0f, tliProgress + rate * dt);
            velocityKms = 7.8f + tliProgress * 3.1f;

            // Limite real segun el Delta-V del cohete construido en la Fase 2
            float maxAchievableRatio = std::clamp(data.rocket.deltaV / 5800.0f, 0.25f, 1.25f);
            if (tliProgress >= maxAchievableRatio && maxAchievableRatio < 1.0f) {
                FinishMission(data, false, "C",
                    "COMBUSTIBLE AGOTADO EN INYECCIÓN TRANSLUNAR (DELTA-V INSUFICIENTE)",
                    "PROPELLANT DEPLETED DURING TLI BURN (INSUFFICIENT DELTA-V)",
                    TextFormat("Tu cohete solo disponía de %.0f m/s de Delta-V (mínimo 5.800 m/s para escapar hacia la Luna). La nave quedó en órbita terrestre alta.", data.rocket.deltaV),
                    TextFormat("Your rocket only had %.0f m/s Delta-V (5,800 m/s required for lunar escape). The spacecraft remained stranded in high Earth orbit.", data.rocket.deltaV));
                return GameState::FLIGHT;
            }

            if (tliProgress >= 1.0f) {
                stage = FlightStage::LUNAR_DESCENT;
                landerAltMeters = 1200.0f;
                landerVelY = -24.0f;
                landerVelX = 0.0f;
                landerFuelPct = 100.0f;
            }
        }
    }
    // ==================== 3. DESCENSO Y ALUNIZAJE POLO SUR ====================
    else if (stage == FlightStage::LUNAR_DESCENT) {
        bool thrustUp = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE)) && (landerFuelPct > 0.0f);
        bool thrustLeft = (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) && (landerFuelPct > 0.0f);
        bool thrustRight = (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) && (landerFuelPct > 0.0f);

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Rectangle btnRetro = { 490.0f, 628.0f, 300.0f, 52.0f };
            if (CheckCollisionPointRec(mouse, btnRetro) && landerFuelPct > 0.0f) thrustUp = true;
        }

        const float lunarGravity = -1.62f; // Gravedad real de la Luna (1.62 m/s^2)
        float accelY = lunarGravity * 4.5f;
        float accelX = 0.0f;

        if (thrustUp) {
            accelY += 16.5f;
            landerFuelPct = std::max(0.0f, landerFuelPct - 8.5f * dt);
        }
        if (thrustLeft) {
            accelX -= 38.0f;
            landerFuelPct = std::max(0.0f, landerFuelPct - 2.2f * dt);
        }
        if (thrustRight) {
            accelX += 38.0f;
            landerFuelPct = std::max(0.0f, landerFuelPct - 2.2f * dt);
        }

        landerVelY += accelY * dt;
        landerVelX += accelX * dt;
        landerVelX *= (1.0f - 0.35f * dt);

        landerAltMeters += landerVelY * dt * 4.2f;
        landerPosX += landerVelX * dt;
        landerPosX = std::clamp(landerPosX, -460.0f, 460.0f);

        if (landerAltMeters <= 0.0f) {
            landerAltMeters = 0.0f;
            touchdownSpeed = std::fabs(landerVelY);
            bool onPad = (std::fabs(landerPosX) <= 95.0f);
            bool softTouch = (touchdownSpeed <= 6.2f);

            if (onPad && softTouch) {
                std::string g = (touchdownSpeed <= 3.0f && std::fabs(landerPosX) <= 35.0f) ? "S" : "A";
                FinishMission(data, true, g,
                    "¡ALUNIZAJE EXITOSO EN EL POLO SUR LUNAR!",
                    "SUCCESSFUL TOUCHDOWN AT THE LUNAR SOUTH POLE!",
                    TextFormat("El módulo %s tocó suelo lunar a %.1f m/s dentro de la zona segura LZ-Artemis. ¡Misión histórica cumplida!", data.rocket.lander.name.c_str(), touchdownSpeed),
                    TextFormat("Lander %s touched down at %.1f m/s inside the LZ-Artemis safe zone. Historic lunar mission accomplished!", data.rocket.lander.name.c_str(), touchdownSpeed));
            } else if (!softTouch) {
                FinishMission(data, false, "D",
                    "IMPACTO BRUSCO EN SUPERFICIE LUNAR (VELOCIDAD EXCESIVA)",
                    "HARD LUNAR IMPACT (EXCESSIVE VERTICAL VELOCITY)",
                    TextFormat("Tocaste superficie a %.1f m/s (máximo seguro: 6,2 m/s). Usa los retropropulsores (W / ESPACIO) antes de llegar al suelo.", touchdownSpeed),
                    TextFormat("Touchdown occurred at %.1f m/s (safe limit: 6.2 m/s). Fire retro-thrusters (W / SPACE) before reaching the ground.", touchdownSpeed));
            } else {
                FinishMission(data, false, "D",
                    "ALUNIZAJE FUERA DE ZONA · CRÁTER ESCARPADO",
                    "OFF-TARGET LANDING · STEEP CRATER SLOPE",
                    "El módulo descendió suave pero cayó fuera de la plataforma iluminada en terreno inclinado. Usa A / D para centrarte sobre el marcador verde.",
                    "Descent speed was safe, but the lander came down outside the lit pad on steep terrain. Use A / D to center over the green marker.");
            }
        }
    }
    // ==================== 4. INFORME FINAL ====================
    else if (stage == FlightStage::DEBRIEF) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Rectangle btnRetry  = { 245.0f, 520.0f, 240.0f, 48.0f };
            Rectangle btnHangar = { 515.0f, 520.0f, 250.0f, 48.0f };
            Rectangle btnMenu   = { 795.0f, 520.0f, 240.0f, 48.0f };

            if (CheckCollisionPointRec(mouse, btnRetry))  OnEnter(data);
            if (CheckCollisionPointRec(mouse, btnHangar)) return GameState::HANGAR;
            if (CheckCollisionPointRec(mouse, btnMenu))   return GameState::MENU;
        }
    }

    return GameState::FLIGHT;
}

void MissionPhase::DrawModularRocket(const SharedMissionData& data, float centerX, float bottomY,
                                     float scale, float angleDeg, bool firingMain, bool firingBoosters) const {
    int capIdx   = std::clamp(data.eqCapsuleIdx, 0, 5);
    int lanIdx   = std::clamp(data.eqLanderIdx - 6, 0, 5);
    int tnkIdx   = std::clamp(data.eqTankIdx - 12, 0, 5);
    int engIdx   = std::clamp(data.eqEngineIdx - 18, 0, 5);
    int boostIdx = (data.eqBoosterIdx >= 25) ? std::clamp(data.eqBoosterIdx - 25, 0, 4) : -1;

    float rad = (angleDeg - 90.0f) * (3.14159265f / 180.0f);
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);

    auto rotPoint = [&](float localX, float localY) -> Vector2 {
        return {
            centerX + (localX * cosA - localY * sinA) * scale,
            bottomY + (localX * sinA + localY * cosA) * scale
        };
    };

    // Llamas inferiores si los motores estan encendidos
    if (firingMain) {
        float flicker = 75.0f + static_cast<float>(GetRandomValue(-14, 22));
        if (texFlameMain1.id > 0) {
            Texture2D fTex = (GetRandomValue(0, 1) == 0 || texFlameMain2.id == 0) ? texFlameMain1 : texFlameMain2;
            float fw = 110.0f * scale;
            float fh = 155.0f * scale;
            DrawTexturePro(fTex, { 0, 0, (float)fTex.width, (float)fTex.height },
                           { centerX, bottomY, fw, fh }, { fw * 0.5f, 0.0f }, 90.0f - angleDeg, WHITE);
        } else {
            Vector2 pL = rotPoint(-22.0f, 0.0f);
            Vector2 pR = rotPoint( 22.0f, 0.0f);
            Vector2 pTip = rotPoint(0.0f, flicker);
            DrawTriangle(pL, pTip, pR, ORANGE);

            Vector2 pL2 = rotPoint(-11.0f, 0.0f);
            Vector2 pR2 = rotPoint( 11.0f, 0.0f);
            Vector2 pTip2 = rotPoint(0.0f, flicker * 0.62f);
            DrawTriangle(pL2, pTip2, pR2, WHITE);
        }
    }

    if (firingBoosters && boostIdx >= 0) {
        float bFlicker = 62.0f + static_cast<float>(GetRandomValue(-10, 18));
        for (int side = -1; side <= 1; side += 2) {
            float bx = side * 48.0f;
            Vector2 bL = rotPoint(bx - 10.0f, -6.0f);
            Vector2 bR = rotPoint(bx + 10.0f, -6.0f);
            Vector2 bTip = rotPoint(bx, bFlicker);
            DrawTriangle(bL, bTip, bR, GOLD);
        }
    }

    // Si existen sprites PNG modulares dibujados por el equipo, se apilan centrados
    bool hasAnyModularPng = (texCapsules[capIdx].id > 0 || texTanks[tnkIdx].id > 0 || texEngines[engIdx].id > 0);
    if (hasAnyModularPng) {
        float curY = bottomY;
        auto drawPartTex = [&](Texture2D t, float h) {
            if (t.id <= 0) return;
            float w = 220.0f * scale;
            float hh = h * scale;
            DrawTexturePro(t, { 0, 0, (float)t.width, (float)t.height },
                           { centerX, curY, w, hh }, { w * 0.5f, hh }, 90.0f - angleDeg, WHITE);
            curY -= hh;
        };
        drawPartTex(texEngines[engIdx], 65.0f);
        float tankBottomY = curY;
        drawPartTex(texTanks[tnkIdx], 175.0f);
        drawPartTex(texLanders[lanIdx], 95.0f);
        drawPartTex(texCapsules[capIdx], 90.0f);

        if (boostIdx >= 0 && texBoosters[boostIdx].id > 0) {
            Texture2D tb = texBoosters[boostIdx];
            float bw = 220.0f * scale;
            float bh = 195.0f * scale;
            DrawTexturePro(tb, { 0, 0, (float)tb.width, (float)tb.height },
                           { centerX, tankBottomY + 10.0f * scale, bw, bh }, { bw * 0.5f, bh }, 90.0f - angleDeg, WHITE);
        }
        return;
    }

    // Renderizado tecnico modular NASA (adaptado a las piezas exactas compradas)
    float tankW = 46.0f + tnkIdx * 5.5f;   // 46..73 px segun el tanque elegido
    float tankH = 128.0f + tnkIdx * 9.0f;  // 128..173 px
    float engH  = 34.0f;
    float lanH  = 52.0f;
    float capH  = 52.0f;

    auto drawRotatedBox = [&](float localYTop, float localYBot, float width, Color fill, Color border) {
        float hw = width * 0.5f;
        Vector2 tl = rotPoint(-hw, localYTop);
        Vector2 tr = rotPoint( hw, localYTop);
        Vector2 br = rotPoint( hw, localYBot);
        Vector2 bl = rotPoint(-hw, localYBot);
        DrawTriangle(tl, bl, br, fill);
        DrawTriangle(tl, br, tr, fill);
        DrawLineEx(tl, tr, 1.5f, border);
        DrawLineEx(tr, br, 1.5f, border);
        DrawLineEx(br, bl, 1.5f, border);
        DrawLineEx(bl, tl, 1.5f, border);
    };

    // 1. Motor principal (toberas)
    int nozzleCount = std::min(5, 1 + engIdx);
    float spread = (tankW * 0.72f) / std::max(1, nozzleCount);
    for (int n = 0; n < nozzleCount; ++n) {
        float nx = (nozzleCount == 1) ? 0.0f : (-tankW * 0.32f + n * spread);
        Vector2 nTopL = rotPoint(nx - 7.0f, -engH);
        Vector2 nTopR = rotPoint(nx + 7.0f, -engH);
        Vector2 nBotL = rotPoint(nx - 11.0f, 0.0f);
        Vector2 nBotR = rotPoint(nx + 11.0f, 0.0f);
        DrawTriangle(nTopL, nBotL, nBotR, Color{ 55, 62, 74, 255 });
        DrawTriangle(nTopL, nBotR, nTopR, Color{ 85, 94, 110, 255 });
    }

    // 2. Tanque de combustible (naranja criogenico para SLS/Super-H2, blanco NASA para Saturno/Redstone)
    float tankBot = -engH;
    float tankTop = tankBot - tankH;
    Color tankColor = (tnkIdx == 3 || tnkIdx == 5) ? Color{ 218, 108, 38, 255 } : Color{ 238, 242, 248, 255 };
    drawRotatedBox(tankTop, tankBot, tankW, tankColor, Color{ 35, 45, 65, 255 });
    drawRotatedBox(tankBot - 16.0f, tankBot, tankW, Color{ 25, 30, 42, 255 }, BLACK);
    drawRotatedBox(tankTop, tankTop + 14.0f, tankW, Color{ 25, 30, 42, 255 }, BLACK);

    // 3. Modulo Lunar / Carena
    float lanBot = tankTop;
    float lanTop = lanBot - lanH;
    drawRotatedBox(lanTop, lanBot, tankW * 0.86f, Color{ 225, 230, 238, 255 }, Color{ 40, 50, 70, 255 });
    drawRotatedBox(lanTop + 14.0f, lanBot - 12.0f, tankW * 0.56f, Color{ 212, 168, 46, 255 }, DARKBROWN);

    // 4. Capsula tripulada + Torre de Escape (LES)
    float capBot = lanTop;
    float capTop = capBot - capH;
    Color capColor = (capIdx <= 1) ? Color{ 62, 68, 78, 255 } : Color{ 242, 245, 250, 255 };
    Vector2 cBL = rotPoint(-tankW * 0.42f, capBot);
    Vector2 cBR = rotPoint( tankW * 0.42f, capBot);
    Vector2 cTip = rotPoint(0.0f, capTop);
    DrawTriangle(cBL, cBR, cTip, capColor);
    DrawLineEx(cBL, cTip, 2.0f, DARKGRAY);
    DrawLineEx(cBR, cTip, 2.0f, DARKGRAY);

    // Torre LES
    Vector2 lesBase = rotPoint(0.0f, capTop);
    Vector2 lesTip  = rotPoint(0.0f, capTop - 28.0f);
    DrawLineEx(lesBase, lesTip, 3.0f, RED);

    // 5. Propulsores laterales (si estan acoplados)
    if (boostIdx >= 0) {
        float bW = 16.0f + boostIdx * 2.0f;
        float bH = tankH * 0.88f;
        Color bCol = (boostIdx == 1 || boostIdx == 4) ? Color{ 48, 54, 66, 255 } : Color{ 245, 247, 252, 255 };
        for (int side = -1; side <= 1; side += 2) {
            float bx = side * (tankW * 0.5f + bW * 0.55f);
            drawRotatedBox(tankBot - bH, tankBot + 4.0f, bW, bCol, BLACK);
            Vector2 noseL = rotPoint(bx - bW * 0.5f, tankBot - bH);
            Vector2 noseR = rotPoint(bx + bW * 0.5f, tankBot - bH);
            Vector2 noseT = rotPoint(bx, tankBot - bH - 22.0f);
            DrawTriangle(noseL, noseR, noseT, RED);
        }
    }
}

void MissionPhase::DrawAscentScene(const SharedMissionData& data) const {
    float spaceRatio = std::clamp(altitudeKm / 130.0f, 0.0f, 1.0f);

    if (texLaunchBg.id > 0 && spaceRatio < 0.35f) {
        DrawTexturePro(texLaunchBg, { 0, 0, (float)texLaunchBg.width, (float)texLaunchBg.height },
                       { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, { 0, 0 }, 0.0f, WHITE);
    } else {
        Color skyTop = {
            static_cast<unsigned char>(24 * (1.0f - spaceRatio) + 6 * spaceRatio),
            static_cast<unsigned char>(78 * (1.0f - spaceRatio) + 10 * spaceRatio),
            static_cast<unsigned char>(158 * (1.0f - spaceRatio) + 22 * spaceRatio),
            255
        };
        Color skyBot = {
            static_cast<unsigned char>(95 * (1.0f - spaceRatio) + 12 * spaceRatio),
            static_cast<unsigned char>(162 * (1.0f - spaceRatio) + 18 * spaceRatio),
            static_cast<unsigned char>(225 * (1.0f - spaceRatio) + 38 * spaceRatio),
            255
        };
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, skyTop, skyBot);
    }

    // Estrellas aparecen al subir hacia la estratosfera
    if (spaceRatio > 0.25f) {
        float starMul = (spaceRatio - 0.25f) / 0.75f;
        for (const auto& s : stars) {
            DrawCircleV({ s.x, s.y }, s.size, Color{ 240, 245, 255, static_cast<unsigned char>(s.alpha * starMul) });
        }
    }

    // Plataforma de lanzamiento LC-39B y Torre umbilical al inicio
    float groundOffset = altitudeKm * 38.0f;
    if (groundOffset < 650.0f) {
        float padY = 635.0f + groundOffset;
        DrawRectangle(0, static_cast<int>(padY), SCREEN_WIDTH, 200, Color{ 42, 92, 52, 255 });
        DrawRectangle(470, static_cast<int>(padY - 24), 340, 24, Color{ 70, 76, 88, 255 });

        // Torre umbilical
        if (texLaunchTower.id > 0) {
            DrawTexturePro(texLaunchTower, { 0, 0, (float)texLaunchTower.width, (float)texLaunchTower.height },
                           { 430.0f, padY - 430.0f, 220.0f, 430.0f }, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangle(495, static_cast<int>(padY - 380), 34, 356, Color{ 185, 48, 42, 255 });
            DrawRectangleLines(495, static_cast<int>(padY - 380), 34, 356, WHITE);
            // Brazo umbilical que se retrae cuando countdownTimer <= 0.6
            float armExtend = (countdownTimer > 0.6f) ? 88.0f : 24.0f;
            DrawRectangle(529, static_cast<int>(padY - 295), static_cast<int>(armExtend), 14, LIGHTGRAY);
        }

        // Nube de humo al encender motores
        if (countdownTimer <= 0.4f) {
            float smokeExpand = std::min(1.0f, (0.4f - countdownTimer + missionClock) * 0.8f);
            if (texSmoke.id > 0) {
                DrawTexturePro(texSmoke, { 0, 0, (float)texSmoke.width, (float)texSmoke.height },
                               { 640.0f - 260.0f * smokeExpand, padY - 95.0f, 520.0f * smokeExpand, 140.0f },
                               { 0, 0 }, 0.0f, Color{ 255, 255, 255, 225 });
            } else {
                for (int c = -5; c <= 5; ++c) {
                    float cx = 640.0f + c * 42.0f * smokeExpand;
                    float cy = padY - 12.0f - std::abs(c) * 3.0f;
                    DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 34.0f + (5 - std::abs(c)) * 5.0f, Color{ 238, 240, 245, 210 });
                }
            }
        }
    }

    // Vibracion de camara durante el encendido
    bool ignited = (countdownTimer <= 0.35f);
    float shakeX = (ignited && altitudeKm < 35.0f) ? static_cast<float>(GetRandomValue(-3, 3)) : 0.0f;
    float shakeY = (ignited && altitudeKm < 35.0f) ? static_cast<float>(GetRandomValue(-2, 2)) : 0.0f;

    float rocketBaseY = 610.0f - std::min(195.0f, altitudeKm * 3.2f) + shakeY;
    DrawModularRocket(data, 640.0f + shakeX, rocketBaseY, 1.05f, pitchDeg, ignited, ignited && boostersAttached);

    // Boosters separandose
    if (leftBooster.active) {
        DrawRectanglePro({ 640.0f + leftBooster.offsetX, rocketBaseY - 90.0f + leftBooster.offsetY, 16.0f, 120.0f },
                         { 8.0f, 60.0f }, leftBooster.rotDeg, WHITE);
    }
    if (rightBooster.active) {
        DrawRectanglePro({ 640.0f + rightBooster.offsetX, rocketBaseY - 90.0f + rightBooster.offsetY, 16.0f, 120.0f },
                         { 8.0f, 60.0f }, rightBooster.rotDeg, WHITE);
    }

    // Cartel grande de cuenta atras rapida
    if (countdownTimer > 0.0f) {
        int sec = static_cast<int>(std::ceil(countdownTimer));
        const char* cdText = (countdownTimer > 0.35f)
            ? TextFormat("T - %d", sec)
            : Tr(data.lang, "¡IGNICIÓN · DESPEGUE!", "IGNITION · LIFTOFF!");
        DrawRectangle(450, 120, 380, 68, Color{ 12, 18, 34, 235 });
        DrawRectangleLinesEx({ 450.0f, 120.0f, 380.0f, 68.0f }, 2.5f, GOLD);
        float tw = MeasureUIText(cdText, 34.0f);
        DrawUIText(cdText, 640.0f - tw * 0.5f, 136.0f, 34.0f, countdownTimer > 0.35f ? GOLD : GREEN);
    }

    // Panel de controles de ascenso
    DrawRectangle(20, 80, 330, 195, Color{ 12, 18, 34, 235 });
    DrawRectangleLinesEx({ 20.0f, 80.0f, 330.0f, 195.0f }, 2.0f, SKYBLUE);
    DrawUIText(Tr(data.lang, "ETAPA 1 · ASCENSO ORBITAL", "STAGE 1 · ORBITAL ASCENT"), 36.0f, 92.0f, 18.0f, GOLD);
    DrawUIText(TextFormat("TWR: %.2f  |  Delta-V: %.0f m/s", data.rocket.twr, data.rocket.deltaV), 36.0f, 122.0f, 16.0f, WHITE);
    DrawUIText(TextFormat(Tr(data.lang, "Acelerador: %.0f%% (W / S)", "Throttle: %.0f%% (W / S)"), throttle * 100.0f), 36.0f, 150.0f, 16.0f, GREEN);
    DrawUIText(TextFormat(Tr(data.lang, "Inclinación: %.0f° (A / D)", "Pitch: %.0f° (A / D)"), pitchDeg), 36.0f, 178.0f, 16.0f, SKYBLUE);
    DrawUIText(Tr(data.lang, "Objetivo: Alcanzar 180 km de altitud", "Goal: Reach 180 km orbital altitude"), 36.0f, 212.0f, 15.0f, LIGHTGRAY);
    DrawUIText(Tr(data.lang, "P: Piloto Automático  |  ESPACIO: x2", "P: Auto-Pilot  |  SPACE: x2 Speed"), 36.0f, 240.0f, 14.0f, GOLD);
}

void MissionPhase::DrawTLIScene(const SharedMissionData& data) const {
    for (const auto& s : stars) {
        DrawCircleV({ s.x, s.y }, s.size, Color{ 235, 242, 255, s.alpha });
    }

    // Tierra a la izquierda y Luna a la derecha
    DrawCircle(185, 390, 145, Color{ 32, 96, 188, 255 });
    DrawCircleLines(185, 390, 148, Color{ 110, 185, 255, 190 });
    DrawUIText(Tr(data.lang, "TIERRA (LEO)", "EARTH (LEO)"), 130.0f, 550.0f, 18.0f, SKYBLUE);

    DrawCircle(1085, 245, 68, Color{ 212, 214, 222, 255 });
    DrawCircle(1066, 228, 14, Color{ 175, 178, 188, 255 });
    DrawCircle(1105, 262, 18, Color{ 170, 172, 182, 255 });
    DrawUIText(Tr(data.lang, "LUNA (OBJETIVO)", "MOON (TARGET)"), 1015.0f, 326.0f, 18.0f, GOLD);

    // Trayectoria punteada translunar
    Vector2 startP = { 340.0f, 380.0f };
    Vector2 endP   = { 1015.0f, 255.0f };
    for (int step = 0; step <= 20; ++step) {
        float t = step / 20.0f;
        float px = startP.x + (endP.x - startP.x) * t;
        float py = startP.y + (endP.y - startP.y) * t - std::sin(t * 3.14159f) * 65.0f;
        DrawCircle(static_cast<int>(px), static_cast<int>(py), 3.0f, (t <= tliProgress) ? GREEN : GRAY);
    }

    // Nave moviendose por la trayectoria TLI
    float shipX = startP.x + (endP.x - startP.x) * tliProgress;
    float shipY = startP.y + (endP.y - startP.y) * tliProgress - std::sin(tliProgress * 3.14159f) * 65.0f;
    DrawModularRocket(data, shipX, shipY + 25.0f, 0.45f, 18.0f, tliBurning, false);

    // Panel inferior de maniobra TLI
    DrawRectangle(250, 495, 780, 195, Color{ 12, 18, 34, 245 });
    DrawRectangleLinesEx({ 250.0f, 495.0f, 780.0f, 195.0f }, 2.5f, GOLD);

    DrawUIText(Tr(data.lang,
        "ETAPA 2 · INYECCIÓN TRANSLUNAR (TLI) — MANTÉN ESPACIO EN LA ZONA VERDE",
        "STAGE 2 · TRANSLUNAR INJECTION (TLI) — HOLD SPACE IN THE GREEN ZONE"),
        280.0f, 512.0f, 18.0f, GOLD);

    // Barra de alineacion TLI
    Rectangle bar = { 290.0f, 552.0f, 700.0f, 34.0f };
    DrawRectangleRec(bar, Color{ 25, 34, 58, 255 });
    DrawRectangle(static_cast<int>(bar.x + bar.width * 0.32f), static_cast<int>(bar.y),
                  static_cast<int>(bar.width * 0.38f), static_cast<int>(bar.height), Color{ 34, 138, 68, 255 });
    DrawRectangleLinesEx(bar, 2.0f, WHITE);

    float cursorX = bar.x + bar.width * tliCursor;
    DrawRectangle(static_cast<int>(cursorX - 4), static_cast<int>(bar.y - 5), 8, static_cast<int>(bar.height + 10), GOLD);

    Rectangle btnBurn = { 430.0f, 610.0f, 420.0f, 56.0f };
    DrawRectangleRec(btnBurn, tliBurning ? Color{ 215, 115, 25, 255 } : Color{ 36, 92, 162, 255 });
    DrawRectangleLinesEx(btnBurn, 2.0f, WHITE);
    const char* burnLabel = TextFormat(Tr(data.lang,
        "ENCENDER MOTOR TLI (ESPACIO) · %.0f%%",
        "IGNITE TLI ENGINE (SPACE) · %.0f%%"), tliProgress * 100.0f);
    float bw = MeasureUIText(burnLabel, 20.0f);
    DrawUIText(burnLabel, btnBurn.x + (btnBurn.width - bw) * 0.5f, btnBurn.y + 17.0f, 20.0f, WHITE);
}

void MissionPhase::DrawLandingScene(const SharedMissionData& data) const {
    for (const auto& s : stars) {
        DrawCircleV({ s.x, s.y }, s.size, Color{ 235, 242, 255, s.alpha });
    }

    // Tierra lejana en el cielo negro lunar
    DrawCircle(1040, 145, 42, Color{ 42, 118, 212, 255 });

    // Superficie craterizada del Polo Sur Lunar
    float groundY = 605.0f;
    DrawRectangle(0, static_cast<int>(groundY), SCREEN_WIDTH, 130, Color{ 108, 112, 122, 255 });
    DrawEllipse(220, 635, 95, 18, Color{ 75, 78, 86, 255 });
    DrawEllipse(420, 645, 70, 14, Color{ 75, 78, 86, 255 });
    DrawEllipse(880, 640, 85, 16, Color{ 75, 78, 86, 255 });
    DrawEllipse(1090, 632, 105, 20, Color{ 75, 78, 86, 255 });

    // Zona segura de alunizaje iluminada en el centro (X = 640)
    DrawRectangle(545, static_cast<int>(groundY - 4), 190, 8, GREEN);
    DrawUIText("LZ-ARTEMIS", 595.0f, groundY + 12.0f, 16.0f, GREEN);

    // Modulo Lunar descendiendo
    float landerScreenX = 640.0f + landerPosX;
    float landerScreenY = groundY - (landerAltMeters / 1200.0f) * 460.0f;

    bool retroFiring = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) && (landerFuelPct > 0.0f);

    if (retroFiring) {
        DrawTriangle({ landerScreenX - 12.0f, landerScreenY },
                     { landerScreenX + 12.0f, landerScreenY },
                     { landerScreenX, landerScreenY + 38.0f + GetRandomValue(0, 12) }, GOLD);
    }

    // Patas desplegadas del modulo lunar
    DrawLineEx({ landerScreenX - 18.0f, landerScreenY - 12.0f }, { landerScreenX - 34.0f, landerScreenY }, 3.0f, LIGHTGRAY);
    DrawLineEx({ landerScreenX + 18.0f, landerScreenY - 12.0f }, { landerScreenX + 34.0f, landerScreenY }, 3.0f, LIGHTGRAY);

    // Cuerpo dorado/blanco del modulo lunar
    DrawRectangle(static_cast<int>(landerScreenX - 22), static_cast<int>(landerScreenY - 34), 44, 22, Color{ 218, 172, 46, 255 });
    DrawRectangleLines(static_cast<int>(landerScreenX - 22), static_cast<int>(landerScreenY - 34), 44, 22, BLACK);
    DrawRectangle(static_cast<int>(landerScreenX - 16), static_cast<int>(landerScreenY - 54), 32, 20, Color{ 238, 242, 248, 255 });
    DrawRectangleLines(static_cast<int>(landerScreenX - 16), static_cast<int>(landerScreenY - 54), 32, 20, DARKGRAY);

    // Panel de telemetria de descenso
    DrawRectangle(20, 80, 345, 195, Color{ 12, 18, 34, 235 });
    DrawRectangleLinesEx({ 20.0f, 80.0f, 345.0f, 195.0f }, 2.0f, GOLD);
    DrawUIText(Tr(data.lang, "ETAPA 3 · DESCENSO LUNAR", "STAGE 3 · LUNAR DESCENT"), 36.0f, 92.0f, 18.0f, GOLD);
    DrawUIText(TextFormat(Tr(data.lang, "Altitud: %.0f m", "Altitude: %.0f m"), landerAltMeters), 36.0f, 122.0f, 17.0f, WHITE);
    Color velCol = (std::fabs(landerVelY) <= 6.2f) ? GREEN : Color{ 255, 100, 100, 255 };
    DrawUIText(TextFormat(Tr(data.lang, "Vel. Vertical: %.1f m/s (<= 6,2)", "Vertical Speed: %.1f m/s (<= 6.2)"), landerVelY), 36.0f, 150.0f, 16.0f, velCol);
    DrawUIText(TextFormat(Tr(data.lang, "Combustible Lander: %.0f%%", "Lander Fuel: %.0f%%"), landerFuelPct), 36.0f, 178.0f, 16.0f, SKYBLUE);
    DrawUIText(Tr(data.lang, "W / ESPACIO: Frenar descenso", "W / SPACE: Retro-thrusters"), 36.0f, 212.0f, 15.0f, LIGHTGRAY);
    DrawUIText(Tr(data.lang, "A / D: Alinear con zona verde", "A / D: Align with green pad"), 36.0f, 238.0f, 15.0f, LIGHTGRAY);

    Rectangle btnRetro = { 490.0f, 628.0f, 300.0f, 52.0f };
    DrawRectangleRec(btnRetro, retroFiring ? ORANGE : Color{ 34, 86, 152, 255 });
    DrawRectangleLinesEx(btnRetro, 2.0f, WHITE);
    const char* rTxt = Tr(data.lang, "FRENAR DESCENSO (ESPACIO)", "FIRE RETRO-THRUST (SPACE)");
    float rw = MeasureUIText(rTxt, 18.0f);
    DrawUIText(rTxt, btnRetro.x + (btnRetro.width - rw) * 0.5f, btnRetro.y + 16.0f, 18.0f, WHITE);
}

void MissionPhase::DrawDebriefModal(const SharedMissionData& data) const {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 6, 10, 20, 210 });

    DrawRectangle(215, 130, 850, 465, Color{ 13, 20, 38, 252 });
    DrawRectangleLinesEx({ 215.0f, 130.0f, 850.0f, 465.0f }, 3.0f, success ? GREEN : RED);

    DrawUIText(Tr(data.lang,
        "INFORME OFICIAL DE MISIÓN NASA · AUTO-GUARDADO",
        "OFFICIAL NASA MISSION DEBRIEF · AUTO-SAVED"),
        250.0f, 152.0f, 18.0f, SKYBLUE);

    DrawUIText(Tr(data.lang, resultTitleEs.c_str(), resultTitleEn.c_str()),
               250.0f, 188.0f, 25.0f, success ? GREEN : Color{ 255, 95, 95, 255 });

    // Insignia de calificacion
    DrawRectangle(945, 150, 85, 76, success ? Color{ 28, 118, 56, 255 } : Color{ 145, 38, 38, 255 });
    DrawRectangleLines(945, 150, 85, 76, GOLD);
    DrawUIText(grade.c_str(), 973.0f, 166.0f, 44.0f, GOLD);

    DrawUITextBoxed(Tr(data.lang, resultDetailEs.c_str(), resultDetailEn.c_str()),
                    { 250.0f, 238.0f, 780.0f, 95.0f }, 19.0f, 5.0f, WHITE);

    DrawRectangle(250, 345, 780, 145, Color{ 20, 30, 54, 255 });
    DrawRectangleLinesEx({ 250.0f, 345.0f, 780.0f, 145.0f }, 1.5f, GOLD);

    DrawUIText(TextFormat(Tr(data.lang,
        "Cohete: %s + %s + %s",
        "Vehicle: %s + %s + %s"),
        data.rocket.capsule.name.c_str(), data.rocket.tank.name.c_str(), data.rocket.engine.name.c_str()),
        270.0f, 362.0f, 17.0f, GOLD);

    DrawUIText(TextFormat(Tr(data.lang,
        "Altitud Máx: %.1f km   |   Vel. Máx: %.2f km/s   |   Vel. Alunizaje: %.1f m/s",
        "Peak Altitude: %.1f km   |   Peak Velocity: %.2f km/s   |   Touchdown: %.1f m/s"),
        altitudeKm, velocityKms, touchdownSpeed),
        270.0f, 398.0f, 17.0f, WHITE);

    DrawUIText(TextFormat(Tr(data.lang,
        "Coste Cohete: $%.0fM   |   TWR: %.2f   |   Delta-V: %.0f m/s",
        "Rocket Cost: $%.0fM   |   TWR: %.2f   |   Delta-V: %.0f m/s"),
        data.rocket.totalCost, data.rocket.twr, data.rocket.deltaV),
        270.0f, 434.0f, 17.0f, LIGHTGRAY);

    Rectangle btnRetry  = { 245.0f, 520.0f, 240.0f, 48.0f };
    Rectangle btnHangar = { 515.0f, 520.0f, 250.0f, 48.0f };
    Rectangle btnMenu   = { 795.0f, 520.0f, 240.0f, 48.0f };

    DrawRectangleRec(btnRetry, Color{ 35, 88, 155, 255 });
    DrawRectangleLinesEx(btnRetry, 2.0f, WHITE);
    DrawUIText(Tr(data.lang, "REPETIR VUELO", "RETRY FLIGHT"), 292.0f, 534.0f, 19.0f, WHITE);

    DrawRectangleRec(btnHangar, Color{ 185, 125, 25, 255 });
    DrawRectangleLinesEx(btnHangar, 2.0f, WHITE);
    DrawUIText(Tr(data.lang, "IR AL HANGAR", "GO TO HANGAR"), 568.0f, 534.0f, 19.0f, WHITE);

    DrawRectangleRec(btnMenu, Color{ 38, 125, 62, 255 });
    DrawRectangleLinesEx(btnMenu, 2.0f, WHITE);
    DrawUIText(Tr(data.lang, "MENÚ PRINCIPAL", "MAIN MENU"), 845.0f, 534.0f, 19.0f, WHITE);
}

void MissionPhase::Draw(const SharedMissionData& data) {
    ClearBackground(Color{ 8, 12, 24, 255 });

    if (stage == FlightStage::COUNTDOWN_ASCENT) {
        DrawAscentScene(data);
    } else if (stage == FlightStage::TLI_BURN) {
        DrawTLIScene(data);
    } else {
        DrawLandingScene(data);
    }

    // Barra superior fija
    DrawRectangle(0, 0, SCREEN_WIDTH, 60, Color{ 10, 15, 28, 245 });
    DrawRectangle(20, 14, 100, 34, Color{ 25, 35, 60, 255 });
    DrawRectangleLines(20, 14, 100, 34, GOLD);
    DrawUIText("MENÚ", 44.0f, 21.0f, 18.0f, WHITE);

    DrawRectangle(130, 14, 175, 34, Color{ 35, 55, 95, 255 });
    DrawRectangleLines(130, 14, 175, 34, SKYBLUE);
    DrawUIText(Tr(data.lang, "< AL HANGAR", "< TO HANGAR"), 148.0f, 21.0f, 17.0f, WHITE);

    if (stage == FlightStage::COUNTDOWN_ASCENT) {
        DrawRectangle(315, 14, 125, 34, Color{ 45, 85, 55, 255 });
        DrawRectangleLines(315, 14, 125, 34, GREEN);
        DrawUIText(TextFormat("WARP: %dx", timeWarp), 334.0f, 21.0f, 17.0f, WHITE);

        DrawRectangle(450, 14, 175, 34, autoPilot ? Color{ 38, 125, 62, 255 } : Color{ 45, 52, 70, 255 });
        DrawRectangleLines(450, 14, 175, 34, autoPilot ? GREEN : GRAY);
        DrawUIText(autoPilot ? "AUTO-PILOT: ON" : "AUTO-PILOT: OFF", 464.0f, 21.0f, 16.0f, WHITE);
    }

    DrawUIText(TextFormat(Tr(data.lang,
        "ALT: %.1f km   |   VEL: %.2f km/s   |   FUEL: %.1f t",
        "ALT: %.1f km   |   VEL: %.2f km/s   |   FUEL: %.1f t"),
        altitudeKm, velocityKms, fuelRemainingTons),
        665.0f, 19.0f, 21.0f, GOLD);

    if (stage == FlightStage::DEBRIEF) {
        DrawDebriefModal(data);
    }
}

void MissionPhase::Unload() {
    if (!spritesLoaded) return;

    for (int i = 0; i < 6; ++i) {
        if (texCapsules[i].id > 0) UnloadTexture(texCapsules[i]);
        if (texLanders[i].id > 0)  UnloadTexture(texLanders[i]);
        if (texTanks[i].id > 0)    UnloadTexture(texTanks[i]);
        if (texEngines[i].id > 0)  UnloadTexture(texEngines[i]);
    }
    for (int i = 0; i < 5; ++i) {
        if (texBoosters[i].id > 0) UnloadTexture(texBoosters[i]);
    }

    if (texLaunchBg.id > 0)    UnloadTexture(texLaunchBg);
    if (texLaunchTower.id > 0) UnloadTexture(texLaunchTower);
    if (texFlameMain1.id > 0)  UnloadTexture(texFlameMain1);
    if (texFlameMain2.id > 0)  UnloadTexture(texFlameMain2);
    if (texFlameSrb1.id > 0)   UnloadTexture(texFlameSrb1);
    if (texFlameSrb2.id > 0)   UnloadTexture(texFlameSrb2);
    if (texSmoke.id > 0)       UnloadTexture(texSmoke);

    spritesLoaded = false;
}