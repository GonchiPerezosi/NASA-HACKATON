#include "committee.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <cctype>

// Helper para ajustar texto largo dentro de cajas de diálogo en Raylib
static void DrawWrappedText(const std::string& text, float x, float y, float maxWidth, int fontSize, Color color) {
    std::string line;
    std::string word;
    float curY = y;

    for (size_t i = 0; i <= text.size(); ++i) {
        char c = (i < text.size()) ? text[i] : ' ';
        if (c == '\n') {
            line += word;
            DrawText(line.c_str(), static_cast<int>(x), static_cast<int>(curY), fontSize, color);
            curY += fontSize + 5.0f;
            line.clear();
            word.clear();
        } else if (c == ' ') {
            std::string test = line + word + " ";
            if (MeasureText(test.c_str(), fontSize) > static_cast<int>(maxWidth) && !line.empty()) {
                DrawText(line.c_str(), static_cast<int>(x), static_cast<int>(curY), fontSize, color);
                curY += fontSize + 5.0f;
                line = word + " ";
            } else {
                line = test;
            }
            word.clear();
        } else {
            word += c;
        }
    }
    if (!line.empty()) {
        DrawText(line.c_str(), static_cast<int>(x), static_cast<int>(curY), fontSize, color);
    }
}

void CommitteePhase::Init() {
    stage = Stage::INTRO;
    writeMode = false;
    showNasaFacts = false;
    activeSpeakerIdx = -1;
    currentJudgeIdx = 0;
    reactStepIdx = 0;
    totalApprovedMillions = 0;
    yesVotesCount = 0;
    userTypedText.clear();

    // Carga de sprites del ZIP si no están cargados aún[cite: 1]
    if (texBg.id == 0)   texBg   = LoadTexture("sprites/bg.png");
    if (texDesk.id == 0) texDesk = LoadTexture("sprites/desk.png");
    if (texHero.id == 0) texHero = LoadTexture("sprites/hero.png");

    // Datos exactos de los 3 jueces de index.html[cite: 2]
    BoardJudge hale;
    hale.id = "hale";
    hale.name = "MR. HALE";
    hale.role = "Treasurer";
    hale.posX = 695.0f;
    hale.maxContributionMillions = 2000;
    hale.question = "Give me one number: what does this cost, and what stops it from doubling?";
    hale.options = {
        "$5.5 billion. The lander company agreed to a fixed price, so if it costs more, they pay. And $0.4B is saved for surprises that only you can unlock.",
        "$5.5 billion. We will pay the builders whatever the work ends up costing, so they can fix problems fast, and I will send you a report every month.",
        "$3 billion, rocket included. We will reuse old Apollo designs, and other countries have promised to pay anything above that amount."
    };
    hale.keywords = { "cost", "budget", "cheap", "efficien", "reus", "return", "invest", "saving", "partner", "commercial", "contract", "dollar", "$", "million", "billion", "fixed" };
    hale.sprite = LoadTexture("sprites/grey.png");
    hale.fallbackColor = LIGHTGRAY;

    BoardJudge brooks;
    brooks.id = "brooks";
    brooks.name = "CHAIRMAN BROOKS";
    brooks.role = "Chairman";
    brooks.posX = 900.0f;
    brooks.maxContributionMillions = 2500;
    brooks.question = "What do people on Earth get out of this mission?";
    brooks.options = {
        "New ways to recycle water and make energy that can help dry places on Earth, thousands of good jobs, and free data for every school.",
        "A historic first landing at the Moon's south pole, watched live by millions, and a big boost to our country's image around the world.",
        "Detailed maps of the ice on the Moon that our scientists will publish, which will help plan the next missions to the Moon."
    };
    brooks.keywords = { "science", "discover", "inspir", "job", "student", "educat", "public", "future", "humanity", "technolog", "water", "ice", "mars", "research", "leader", "generation" };
    brooks.sprite = LoadTexture("sprites/blue.png");
    brooks.fallbackColor = SKYBLUE;

    BoardJudge vance;
    vance.id = "vance";
    vance.name = "DR. VANCE";
    vance.role = "Chief Engineer";
    vance.posX = 1090.0f;
    vance.maxContributionMillions = 2000;
    vance.question = "Something will fail on the way to the Moon. What is your backup plan?";
    vance.options = {
        "Every important system has a spare, we carry 20% extra food, water and air, and if the lander fails the crew flies home in the capsule.",
        "We use the newest technology there is and test every part twice before launch, so a big failure on the way is very unlikely.",
        "Mission control watches the ship 24 hours a day, and the crew trains for months so they can fix anything with spare parts."
    };
    vance.keywords = { "safe", "risk", "test", "backup", "redundan", "margin", "crew", "abort", "schedule", "reliab", "radiation", "supplies", "oxygen", "fuel", "contingen", "spare" };
    vance.sprite = LoadTexture("sprites/slick.png");
    vance.fallbackColor = ORANGE;

    judges = { hale, brooks, vance };

    // Pitches iniciales de index.html (el índice 0 es el óptimo)[cite: 2]
    pitchOptions = {
        "Four astronauts spend a week at the Moon's south pole looking for ice. We need $5.5 billion: $4.1B for the rocket and capsule, $1B for the lander, $0.4B saved for surprises.",
        "Four astronauts spend a week at the Moon's south pole looking for ice. We only need $900 million, because we will reuse old parts and share the costs with other countries.",
        "Four astronauts spend a week at the Moon's south pole, build a small base and test a Mars rover. We need $5.5 billion and will pay builders whatever the work ends up costing."
    };

    StartDialogue(-1, "PHASE 1 - THE BUDGET HEARING\nYou are the mission economist. Convince the board to pay for a crewed mission to the Moon.\nTip: Click NASA DATA (top-left) for real Artemis cost figures you can use.");
}

void CommitteePhase::Unload() {
    if (texBg.id > 0)   UnloadTexture(texBg);
    if (texDesk.id > 0) UnloadTexture(texDesk);
    if (texHero.id > 0) UnloadTexture(texHero);
    for (auto& j : judges) {
        if (j.sprite.id > 0) UnloadTexture(j.sprite);
    }
}

void CommitteePhase::StartDialogue(int speaker, const std::string& msg) {
    activeSpeakerIdx = speaker;
    currentText = msg;
    visibleChars = 0.0f;
}

void CommitteePhase::PrepareShuffledChoices(const std::vector<std::string>& rawOptions) {
    currentChoices.clear();
    for (size_t i = 0; i < rawOptions.size(); ++i) {
        currentChoices.push_back({ rawOptions[i], i == 0 });
    }
    static std::mt19937 rng(std::random_device{}());
    std::shuffle(currentChoices.begin(), currentChoices.end(), rng);
}

void CommitteePhase::ApplyMoodDelta(int judgeIdx, int delta) {
    judges[judgeIdx].mood = std::clamp(judges[judgeIdx].mood + delta, 0, 100);
    judges[judgeIdx].lastDelta = delta;
    judges[judgeIdx].popTimer = 1.5f;
}

int CommitteePhase::ScoreFreeText(const BoardJudge& j, const std::string& text) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

    int hits = 0;
    for (const auto& k : j.keywords) {
        if (lower.find(k) != std::string::npos) hits++;
    }
    bool hasDigit = std::any_of(lower.begin(), lower.end(), ::isdigit);
    int delta = -10 + std::min(hits, 5) * 6 + (text.size() > 160 ? 4 : (text.size() < 50 ? -6 : 0)) + (hasDigit ? 4 : 0);
    return std::clamp(delta, -18, 20);
}

std::string CommitteePhase::GetReactionLine(int delta) const {
    if (delta > 6)  return "Hm. That is more concrete than I expected. I can work with that.";
    if (delta < -3) return "That is not an argument, that is a slogan. I expected real numbers.";
    return "I have heard that before. I need more specifics than that.";
}

void CommitteePhase::EvaluatePitch(const std::string& text, bool pickedBest, bool usedPickMode) {
    for (int i = 0; i < 3; ++i) {
        int d = ScoreFreeText(judges[i], text);
        if (usedPickMode) {
            d = pickedBest ? std::max(d, 12) : std::min(d, 4);
        }
        pendingDeltas[i] = d;
        pendingReactions[i] = GetReactionLine(d);
    }
}

void CommitteePhase::EvaluateJudgeAnswer(int judgeIdx, const std::string& text, bool pickedBest, bool usedPickMode) {
    int d = ScoreFreeText(judges[judgeIdx], text);
    if (usedPickMode) {
        d = pickedBest ? std::max(d, 20) : std::min(d, 6);
    }
    pendingDeltas[judgeIdx] = d;
    if (usedPickMode && pickedBest) {
        pendingReactions[judgeIdx] = "Solid numbers and clear mechanism. That directly answers my concern.";
    } else if (usedPickMode) {
        pendingReactions[judgeIdx] = "You are glossing over the risk there. That leaves a serious gap in your plan.";
    } else {
        pendingReactions[judgeIdx] = GetReactionLine(d);
    }
}

void CommitteePhase::ComputeVerdict(SharedMissionData& missionData) {
    totalApprovedMillions = 0;
    yesVotesCount = 0;

    for (auto& j : judges) {
        if (j.mood >= 40) {
            // Fórmula exacta de index.html: Math.round(j.max * m / 100 / 50) * 50[cite: 2]
            float raw = (static_cast<float>(j.maxContributionMillions) * j.mood) / 100.0f;
            int pledge = static_cast<int>(std::round(raw / 50.0f)) * 50;
            j.pledgedMillions = pledge;
            totalApprovedMillions += pledge;
            yesVotesCount++;
        } else {
            j.pledgedMillions = 0;
        }
    }

    if (yesVotesCount >= 2) {
        missionData.awardedBudget = static_cast<float>(totalApprovedMillions);
        missionData.remainingBudget = missionData.awardedBudget;
    }
}

void CommitteePhase::AdvanceStage(SharedMissionData& missionData) {
    switch (stage) {
        case Stage::INTRO:
            stage = Stage::MODE_SELECT;
            StartDialogue(-1, "How do you want to answer the board?\nPICK FROM 3: Choose one of three prepared arguments.\nWRITE MY OWN: Type your own budget pitch and answers.");
            break;

        case Stage::PITCH_INTRO:
            stage = Stage::PITCH_INPUT;
            userTypedText.clear();
            if (!writeMode) {
                PrepareShuffledChoices(pitchOptions);
                StartDialogue(-2, "Choose your opening pitch to the NASA Board:");
            } else {
                StartDialogue(-2, "Type your opening pitch (include goal, cost in billions, and safety plan):");
            }
            break;

        case Stage::PITCH_REACT:
            if (reactStepIdx < 3) {
                ApplyMoodDelta(reactStepIdx, pendingDeltas[reactStepIdx]);
                StartDialogue(reactStepIdx, pendingReactions[reactStepIdx]);
                reactStepIdx++;
            } else {
                stage = Stage::ROUND_INTRO;
                StartDialogue(-1, "Question round. Each board member asks you one question. Your answer moves their approval bar up or down.");
            }
            break;

        case Stage::ROUND_INTRO:
            currentJudgeIdx = 0;
            stage = Stage::JUDGE_ASK;
            StartDialogue(currentJudgeIdx, judges[currentJudgeIdx].question);
            break;

        case Stage::JUDGE_ASK:
            stage = Stage::JUDGE_ANSWER;
            userTypedText.clear();
            if (!writeMode) {
                PrepareShuffledChoices(judges[currentJudgeIdx].options);
                StartDialogue(-2, "Choose your answer to " + judges[currentJudgeIdx].name + ":");
            } else {
                StartDialogue(-2, "Type your answer to " + judges[currentJudgeIdx].name + ":");
            }
            break;

        case Stage::JUDGE_REACT:
            currentJudgeIdx++;
            if (currentJudgeIdx < 3) {
                stage = Stage::JUDGE_ASK;
                StartDialogue(currentJudgeIdx, judges[currentJudgeIdx].question);
            } else {
                stage = Stage::VERDICT_INTRO;
                StartDialogue(-1, "The board votes now. A member with approval below 40 gives nothing. You need at least two YES votes.");
            }
            break;

        case Stage::VERDICT_INTRO:
            ComputeVerdict(missionData);
            reactStepIdx = 0;
            stage = Stage::VERDICT_VOTES;
            AdvanceStage(missionData);
            break;

        case Stage::VERDICT_VOTES:
            if (reactStepIdx < 3) {
                const auto& j = judges[reactStepIdx];
                if (j.pledgedMillions > 0) {
                    StartDialogue(reactStepIdx, TextFormat("Approval %d/100. I vote YES and pledge $%.2f billion.", j.mood, j.pledgedMillions / 1000.0f));
                } else {
                    StartDialogue(reactStepIdx, TextFormat("Approval %d/100. You have not convinced me. I vote NO.", j.mood));
                }
                reactStepIdx++;
            } else {
                stage = Stage::VERDICT_FINAL;
                if (yesVotesCount >= 2) {
                    StartDialogue(1, TextFormat("MISSION FUNDED: $%.2f BILLION! Proceed to the Rocket Lab to buy parts and build your Moon vehicle.", totalApprovedMillions / 1000.0f));
                } else {
                    StartDialogue(1, "GAME OVER: The board rejected the mission (fewer than 2 YES votes). Strengthen your numbers and try again.");
                }
            }
            break;

        default:
            break;
    }
}

GameState CommitteePhase::Update(SharedMissionData& missionData) {
    float dt = GetFrameTime();

    // Avanzar efecto máquina de escribir
    if (visibleChars < static_cast<float>(currentText.size())) {
        visibleChars = std::min(static_cast<float>(currentText.size()), visibleChars + 90.0f * dt);
    }

    // Actualizar temporizadores de popups (+20 / -10)
    for (auto& j : judges) {
        if (j.popTimer > 0.0f) j.popTimer = std::max(0.0f, j.popTimer - dt);
    }

    Vector2 mousePos = GetMousePosition();

    // Botón NASA DATA (arriba a la izquierda dentro del panel de presupuesto)[cite: 2]
    Rectangle factBtn = { 340.0f, 34.0f, 115.0f, 34.0f };
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, factBtn)) {
        showNasaFacts = !showNasaFacts;
        return GameState::COMMITTEE;
    }

    if (showNasaFacts) {
        Rectangle closeFactBtn = { SCREEN_WIDTH - 220.0f, SCREEN_HEIGHT - 105.0f, 140.0f, 42.0f };
        if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, closeFactBtn)) || IsKeyPressed(KEY_ESCAPE)) {
            showNasaFacts = false;
        }
        return GameState::COMMITTEE;
    }

    // Selección de modo (PICK FROM 3 vs WRITE MY OWN)
    if (stage == Stage::MODE_SELECT) {
        Rectangle btnPick  = { 760.0f, 645.0f, 220.0f, 44.0f };
        Rectangle btnWrite = { 1005.0f, 645.0f, 220.0f, 44.0f };

        if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, btnPick)) || IsKeyPressed(KEY_ONE)) {
            writeMode = false;
            stage = Stage::PITCH_INTRO;
            StartDialogue(1, "You have the floor. Tell us what this mission is and why we should fund it. Convince all three of us.");
        } else if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, btnWrite)) || IsKeyPressed(KEY_TWO)) {
            writeMode = true;
            stage = Stage::PITCH_INTRO;
            StartDialogue(1, "You have the floor. Tell us what this mission is and why we should fund it. Convince all three of us.");
        }
        return GameState::COMMITTEE;
    }

    // Selección de respuestas (PICK FROM 3)
    if (!writeMode && (stage == Stage::PITCH_INPUT || stage == Stage::JUDGE_ANSWER)) {
        for (size_t i = 0; i < currentChoices.size(); ++i) {
            Rectangle optRect = { 260.0f, 295.0f + i * 74.0f, 985.0f, 64.0f };
            bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, optRect);
            bool keyHit = (i == 0 && IsKeyPressed(KEY_ONE)) || (i == 1 && IsKeyPressed(KEY_TWO)) || (i == 2 && IsKeyPressed(KEY_THREE));

            if (clicked || keyHit) {
                if (stage == Stage::PITCH_INPUT) {
                    EvaluatePitch(currentChoices[i].text, currentChoices[i].isBest, true);
                    reactStepIdx = 0;
                    stage = Stage::PITCH_REACT;
                    AdvanceStage(missionData);
                } else {
                    EvaluateJudgeAnswer(currentJudgeIdx, currentChoices[i].text, currentChoices[i].isBest, true);
                    ApplyMoodDelta(currentJudgeIdx, pendingDeltas[currentJudgeIdx]);
                    stage = Stage::JUDGE_REACT;
                    StartDialogue(currentJudgeIdx, pendingReactions[currentJudgeIdx]);
                }
                break;
            }
        }
        return GameState::COMMITTEE;
    }

    // Entrada de texto libre (WRITE MY OWN)
    if (writeMode && (stage == Stage::PITCH_INPUT || stage == Stage::JUDGE_ANSWER)) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125 && userTypedText.size() < 260) {
                userTypedText.push_back(static_cast<char>(key));
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !userTypedText.empty()) {
            userTypedText.pop_back();
        }

        Rectangle sendBtn = { 1065.0f, 645.0f, 160.0f, 44.0f };
        bool canSend = userTypedText.size() >= 20;
        bool clickedSend = canSend && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, sendBtn);

        if (canSend && (clickedSend || IsKeyPressed(KEY_ENTER))) {
            if (stage == Stage::PITCH_INPUT) {
                EvaluatePitch(userTypedText, false, false);
                reactStepIdx = 0;
                stage = Stage::PITCH_REACT;
                AdvanceStage(missionData);
            } else {
                EvaluateJudgeAnswer(currentJudgeIdx, userTypedText, false, false);
                ApplyMoodDelta(currentJudgeIdx, pendingDeltas[currentJudgeIdx]);
                stage = Stage::JUDGE_REACT;
                StartDialogue(currentJudgeIdx, pendingReactions[currentJudgeIdx]);
            }
        }
        return GameState::COMMITTEE;
    }

    // Botón NEXT / IR AL LABORATORIO / REINTENTAR
    Rectangle nextBtn = { 1005.0f, 645.0f, 220.0f, 44.0f };
    bool clickedNext = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, nextBtn);

    if (clickedNext || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (visibleChars < static_cast<float>(currentText.size())) {
            visibleChars = static_cast<float>(currentText.size()); // Completar texto instantáneamente
        } else if (stage == Stage::VERDICT_FINAL) {
            if (yesVotesCount >= 2) {
                return GameState::HANGAR; // Pasar a Fase 2 (Tienda y Laboratorio)
            } else {
                Init(); // Reiniciar audiencia si falló
            }
        } else {
            AdvanceStage(missionData);
        }
    }

    return GameState::COMMITTEE;
}

void CommitteePhase::Draw(const SharedMissionData& missionData) const {
    // 1. Fondo de la sala (sprites/bg.png o fallback)[cite: 1, 2]
    if (texBg.id > 0) {
        DrawTexturePro(texBg, { 0, 0, (float)texBg.width, (float)texBg.height },
                       { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, { 0, 0 }, 0.0f, WHITE);
    } else {
        ClearBackground(Color{ 39, 49, 92, 255 });
    }

    // 2. Los 3 Jueces + Barras de Aprobación + Caras (:), :|, :()[cite: 2]
    for (int i = 0; i < 3; ++i) {
        const auto& j = judges[i];
        float hopY = (activeSpeakerIdx == i) ? std::sin(static_cast<float>(GetTime()) * 16.0f) * 5.0f : 0.0f;

        if (j.sprite.id > 0) {
            DrawTexturePro(j.sprite, { 0, 0, (float)j.sprite.width, (float)j.sprite.height },
                           { j.posX - 70.0f, 62.0f + hopY, 140.0f, 180.0f }, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawRectangle(static_cast<int>(j.posX - 50), static_cast<int>(80 + hopY), 100, 150, j.fallbackColor);
            DrawRectangleLines(static_cast<int>(j.posX - 50), static_cast<int>(80 + hopY), 100, 150, BLACK);
        }

        // Barra vertical de aprobación[cite: 2]
        int barX = static_cast<int>(j.posX + 68);
        int barY = 78;
        int barH = 148;
        Color barCol = (j.mood >= 65) ? Color{ 88, 179, 104, 255 } : ((j.mood >= 35) ? Color{ 232, 179, 60, 255 } : Color{ 217, 83, 79, 255 });

        const char* faceStr = (j.mood >= 65) ? ":)" : ((j.mood >= 35) ? ":|" : ":(");
        DrawText(faceStr, barX - 2, barY - 28, 22, WHITE);

        DrawRectangle(barX, barY, 24, barH, Color{ 13, 17, 36, 255 });
        int fillH = static_cast<int>((j.mood / 100.0f) * (barH - 6));
        DrawRectangle(barX + 3, barY + barH - 3 - fillH, 18, fillH, barCol);
        DrawRectangleLinesEx({ (float)barX, (float)barY, 24.0f, (float)barH }, 3.0f, BLACK);

        // Popup flotante (+20 / -10)[cite: 2]
        if (j.popTimer > 0.0f && j.lastDelta != 0) {
            Color popCol = (j.lastDelta > 0) ? GREEN : RED;
            DrawText(TextFormat("%+d", j.lastDelta), barX - 10, static_cast<int>(barY - 52 - (1.5f - j.popTimer) * 15.0f), 22, popCol);
        }
    }

    // 3. Mesa del comité en primer plano (sprites/desk.png) y placas doradas[cite: 1, 2]
    if (texDesk.id > 0) {
        DrawTexturePro(texDesk, { 0, 0, (float)texDesk.width, (float)texDesk.height },
                       { 470.0f, 228.0f, 810.0f, 492.0f }, { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangle(480, 250, 780, 260, Color{ 107, 70, 48, 255 });
        DrawRectangleLinesEx({ 480, 250, 780, 260 }, 4.0f, BLACK);
    }

    for (const auto& j : judges) {
        Rectangle plate = { j.posX - 78.0f, 282.0f, 156.0f, 42.0f };
        DrawRectangleRec(plate, Color{ 242, 193, 78, 255 });
        DrawRectangleLinesEx(plate, 3.0f, BLACK);
        DrawText(j.name.c_str(), static_cast<int>(plate.x + 10), static_cast<int>(plate.y + 6), 13, BLACK);
        DrawText(j.role.c_str(), static_cast<int>(plate.x + 10), static_cast<int>(plate.y + 22), 12, Color{ 50, 40, 20, 255 });
    }

    // 4. Panel de Presupuesto (Arriba Izquierda)[cite: 2]
    DrawRectangle(20, 20, 450, 235, Color{ 13, 17, 36, 255 });
    DrawRectangleLinesEx({ 20, 20, 450, 235 }, 4.0f, Color{ 242, 193, 78, 255 });
    DrawText("MISSION BUDGET", 38, 36, 20, Color{ 242, 193, 78, 255 });

    // Botón NASA DATA[cite: 2]
    Rectangle factBtn = { 340.0f, 34.0f, 115.0f, 34.0f };
    DrawRectangleRec(factBtn, Color{ 242, 193, 78, 255 });
    DrawRectangleLinesEx(factBtn, 2.0f, BLACK);
    DrawText("NASA DATA", 350, 44, 15, BLACK);

    if (stage == Stage::VERDICT_FINAL && yesVotesCount >= 2) {
        DrawText(TextFormat("$ %.2f BILLION", totalApprovedMillions / 1000.0f), 38, 74, 32, WHITE);
    } else if (stage == Stage::VERDICT_FINAL) {
        DrawText("$ 0.0 BILLION", 38, 74, 32, RED);
    } else {
        DrawText("$ ??? BILLION", 38, 74, 32, WHITE);
    }

    for (size_t i = 0; i < judges.size(); ++i) {
        int rowY = 128 + static_cast<int>(i) * 36;
        DrawText(judges[i].name.c_str(), 38, rowY, 18, Color{ 201, 208, 240, 255 });
        const char* pledgeTxt = (judges[i].pledgedMillions < 0) ? "..." :
                                (judges[i].pledgedMillions == 0) ? "NO" :
                                TextFormat("$ %.2f B", judges[i].pledgedMillions / 1000.0f);
        DrawText(pledgeTxt, 355, rowY, 19, Color{ 242, 193, 78, 255 });
    }

    // 5. Personaje del Jugador (sprites/hero.png)[cite: 1, 2]
    if (texHero.id > 0) {
        DrawTexturePro(texHero, { 0, 0, (float)texHero.width, (float)texHero.height },
                       { 8.0f, 445.0f, 225.0f, 315.0f }, { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangle(25, 480, 180, 240, DARKBLUE);
    }

    // 6. Panel de Opciones (Modo PICK FROM 3)[cite: 2]
    if (!writeMode && (stage == Stage::PITCH_INPUT || stage == Stage::JUDGE_ANSWER)) {
        DrawRectangle(245, 245, 1015, 280, Color{ 244, 236, 216, 255 });
        DrawRectangleLinesEx({ 245, 245, 1015, 280 }, 4.0f, BLACK);
        DrawText("CHOOSE YOUR ANSWER [Teclas 1-3 o Clic]:", 265, 260, 18, Color{ 90, 83, 72, 255 });

        Vector2 mousePos = GetMousePosition();
        for (size_t i = 0; i < currentChoices.size(); ++i) {
            Rectangle optRect = { 260.0f, 295.0f + i * 74.0f, 985.0f, 64.0f };
            bool hover = CheckCollisionPointRec(mousePos, optRect);
            DrawRectangleRec(optRect, hover ? Color{ 242, 193, 78, 255 } : Color{ 255, 250, 240, 255 });
            DrawRectangleLinesEx(optRect, 3.0f, BLACK);

            const char* letter = (i == 0) ? "[A]" : ((i == 1) ? "[B]" : "[C]");
            DrawText(letter, 272, static_cast<int>(optRect.y + 18), 20, BLACK);
            DrawWrappedText(currentChoices[i].text, 318.0f, optRect.y + 10.0f, 910.0f, 18, Color{ 27, 27, 38, 255 });
        }
    }

    // 7. Caja de Diálogo Inferior[cite: 2]
    DrawRectangle(245, 535, 1015, 170, Color{ 244, 236, 216, 255 });
    DrawRectangleLinesEx({ 245, 535, 1015, 170 }, 4.0f, BLACK);

    // Etiqueta de quién habla[cite: 2]
    std::string speakerName = "NARRATOR";
    if (activeSpeakerIdx == -2) speakerName = "YOU - THE ECONOMIST";
    else if (activeSpeakerIdx >= 0 && activeSpeakerIdx < 3) speakerName = judges[activeSpeakerIdx].name;

    int tagW = MeasureText(speakerName.c_str(), 18) + 28;
    DrawRectangle(262, 514, tagW, 32, Color{ 27, 27, 38, 255 });
    DrawRectangleLinesEx({ 262.0f, 514.0f, (float)tagW, 32.0f }, 3.0f, BLACK);
    DrawText(speakerName.c_str(), 276, 521, 18, Color{ 242, 193, 78, 255 });

    // Texto de diálogo o caja de escritura
    if (writeMode && (stage == Stage::PITCH_INPUT || stage == Stage::JUDGE_ANSWER)) {
        DrawText(currentText.c_str(), 268, 554, 18, Color{ 90, 83, 72, 255 });
        DrawRectangle(268, 580, 780, 108, Color{ 255, 250, 240, 255 });
        DrawRectangleLinesEx({ 268, 580, 780, 108 }, 3.0f, BLACK);
        DrawWrappedText(userTypedText + "_", 278.0f, 590.0f, 760.0f, 19, BLACK);

        Rectangle sendBtn = { 1065.0f, 645.0f, 160.0f, 44.0f };
        bool canSend = userTypedText.size() >= 20;
        DrawRectangleRec(sendBtn, canSend ? Color{ 27, 27, 38, 255 } : GRAY);
        DrawText("SEND >", 1105, 657, 19, WHITE);
    } else {
        std::string shown = currentText.substr(0, static_cast<size_t>(visibleChars));
        DrawWrappedText(shown, 268.0f, 558.0f, 965.0f, 22, Color{ 27, 27, 38, 255 });

        if (stage == Stage::MODE_SELECT) {
            Rectangle btnPick  = { 760.0f, 645.0f, 220.0f, 44.0f };
            Rectangle btnWrite = { 1005.0f, 645.0f, 220.0f, 44.0f };
            DrawRectangleRec(btnPick, Color{ 27, 27, 38, 255 });
            DrawText("[1] PICK FROM 3", 785, 657, 18, Color{ 242, 193, 78, 255 });
            DrawRectangleRec(btnWrite, Color{ 27, 27, 38, 255 });
            DrawText("[2] WRITE MY OWN", 1025, 657, 18, WHITE);
        } else if (stage != Stage::PITCH_INPUT && stage != Stage::JUDGE_ANSWER) {
            Rectangle nextBtn = { 1005.0f, 645.0f, 220.0f, 44.0f };
            DrawRectangleRec(nextBtn, (stage == Stage::VERDICT_FINAL && yesVotesCount >= 2) ? DARKGREEN : Color{ 27, 27, 38, 255 });
            const char* btnLabel = (stage == Stage::VERDICT_FINAL)
                ? (yesVotesCount >= 2 ? "GO TO LAB >" : "TRY AGAIN")
                : "NEXT >";
            DrawText(btnLabel, 1045, 657, 19, Color{ 242, 193, 78, 255 });
        }
    }

    // 8. Modal de NASA DATA (Cifras reales de NASA OIG)[cite: 2]
    if (showNasaFacts) {
        DrawNasaFactsModal();
    }
}

void CommitteePhase::DrawNasaFactsModal() const {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.75f));
    DrawRectangle(80, 60, SCREEN_WIDTH - 160, SCREEN_HEIGHT - 120, Color{ 244, 236, 216, 255 });
    DrawRectangleLinesEx({ 80, 60, SCREEN_WIDTH - 160, SCREEN_HEIGHT - 120 }, 4.0f, Color{ 242, 193, 78, 255 });

    DrawText("YOUR NOTES: WHAT MOON MISSIONS REALLY COST (NASA OIG DATA)", 110, 85, 22, BLACK);
    const char* bullets[] = {
        "- Launching NASA's SLS rocket + Orion capsule costs ~$4.1 billion per flight (IG-22-003).",
        "- The Orion capsule alone costs ~$1.0 billion, plus ~$300 million for its service module.",
        "- NASA's entire Artemis program (2012-2025) is projected at ~$93 billion (IG-22-003).",
        "- Cost-plus contracts overran: SLS grew +33% and Artemis I flew ~$4.3B over budget (IG-24-001).",
        "- Moon landers use FIXED-PRICE deals: SpaceX ($2.89B) and Blue Origin ($3.4B) stayed near budget (IG-26-004).",
        "- Robotic lunar landers (CLPS) cost $80M-$200M each, averaging 26% cost growth (IG-24-013).",
        "- Realistic budget for ONE crewed Moon landing: $4.0B to $6.5B (board max is $6.5B)."
    };

    for (int i = 0; i < 7; ++i) {
        DrawText(bullets[i], 115, 138 + i * 54, 20, Color{ 27, 27, 38, 255 });
    }

    Rectangle closeBtn = { SCREEN_WIDTH - 220.0f, SCREEN_HEIGHT - 105.0f, 140.0f, 42.0f };
    DrawRectangleRec(closeBtn, Color{ 27, 27, 38, 255 });
    DrawText("CLOSE [ESC]", static_cast<int>(closeBtn.x + 16), static_cast<int>(closeBtn.y + 12), 16, Color{ 242, 193, 78, 255 });
}
