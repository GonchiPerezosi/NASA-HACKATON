#include "comitee.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <cctype>

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

    // Carga de sprites desde assets/sprites/
    if (texBg.id == 0)   texBg   = LoadTexture("assets/sprites/bg.png");
    if (texDesk.id == 0) texDesk = LoadTexture("assets/sprites/desk.png");
    if (texHero.id == 0) texHero = LoadTexture("assets/sprites/hero.png");

    // JUEZ 1: SR. HALE (Tesorero - centrado en costes y contratos fijos)
    BoardJudge hale;
    hale.id = "hale";
    hale.name = "SR. HALE";
    hale.role = "Tesorero";
    hale.posX = 695.0f;
    hale.maxContributionMillions = 2000;
    hale.question = "Dame una cifra exacta: cuanto cuesta esto y que evita que el precio se duplique?";
    hale.options = {
        "5.500 millones. El aterrizador va a precio fijo (si cuesta mas, lo paga la empresa) y guardamos 400M de reserva.",
        "5.500 millones. Pagaremos a los fabricantes lo que cueste la obra para ir rapido y te enviare un informe mensual.",
        "3.000 millones con cohete incluido. Reutilizaremos piezas viejas de Apolo y otros paises pagaran el resto."
    };
    hale.keywords = { "cost", "presupuesto", "barat", "eficien", "reutil", "retorno", "invers", "ahorr", "contrato", "fijo", "dolar", "$", "millon", "billon", "reserva" };
    hale.reactPositive = "Aja. Por fin cifras realistas y control del gasto. Con eso si puedo trabajar.";
    hale.reactNeutral  = "Esos numeros son demasiado vagos para soltar miles de millones. Concreta mas.";
    hale.reactNegative = "Eso es un eslogan, no un presupuesto. Me estas perdiendo.";
    hale.reactPickGood = "Un contrato a precio fijo con reserva controlada. Eso protege nuestras cuentas.";
    hale.reactPickBad  = "Dejar costes abiertos o prometer cifras irreales siempre acaba duplicando la factura.";
    hale.sprite = LoadTexture("assets/sprites/grey.png");
    hale.fallbackColor = LIGHTGRAY;

    // JUEZ 2: PRESIDENTE BROOKS (Presidente - centrado en beneficio publico, ciencia y empleo)
    BoardJudge brooks;
    brooks.id = "brooks";
    brooks.name = "PRESIDENTE BROOKS";
    brooks.role = "Presidente";
    brooks.posX = 900.0f;
    brooks.maxContributionMillions = 2500;
    brooks.question = "Que ganan los ciudadanos en la Tierra con esta mision lunar?";
    brooks.options = {
        "Nuevos sistemas de reciclaje de agua y energia para zonas secas, miles de empleos y datos abiertos para escuelas.",
        "Un aterrizaje historico en el polo sur lunar visto por millones y un gran impulso a nuestra imagen internacional.",
        "Mapas detallados del hielo lunar que nuestros cientificos publicaran para planificar las siguientes misiones."
    };
    brooks.keywords = { "ciencia", "descubr", "inspir", "empleo", "trabajo", "estudiant", "educa", "public", "futuro", "humanidad", "tecnolog", "agua", "hielo", "investig", "tierra" };
    brooks.reactPositive = "Me gusta como suena. Eso si podemos explicarselo con orgullo a los contribuyentes.";
    brooks.reactNeutral  = "La gente necesita ver un beneficio claro aqui en la Tierra, no solo banderas.";
    brooks.reactNegative = "Con ese discurso el Congreso nos recortara los fondos manana mismo.";
    brooks.reactPickGood = "Tecnologia util para la Tierra, empleo y ciencia abierta. Gran argumento.";
    brooks.reactPickBad  = "El prestigio por si solo no basta para justificar esta inversion ante el pais.";
    brooks.sprite = LoadTexture("assets/sprites/blue.png");
    brooks.fallbackColor = SKYBLUE;

    // JUEZ 3: DRA. VANCE (Ingeniera Jefe - centrada en seguridad, margenes y respaldo)
    BoardJudge vance;
    vance.id = "vance";
    vance.name = "DRA. VANCE";
    vance.role = "Ingeniera Jefe";
    vance.posX = 1090.0f;
    vance.maxContributionMillions = 2000;
    vance.question = "Algo fallara camino a la Luna. Cual es vuestro plan de respaldo?";
    vance.options = {
        "Cada sistema vital tiene repuesto, llevamos 20% extra de agua y aire, y si falla el modulo vuelven en la capsula.",
        "Usamos la tecnologia mas moderna y probamos cada pieza dos veces antes del despegue para evitar fallos.",
        "Control de mision vigila la nave 24 horas y la tripulacion entrena meses para reparar cualquier averia."
    };
    vance.keywords = { "segur", "riesgo", "prueba", "respaldo", "redundan", "margen", "tripulacion", "abort", "fiab", "radiacion", "oxigeno", "combustible", "repuesto", "aire" };
    vance.reactPositive = "Bien pensado. Veo margenes de seguridad realistas y redundancia tecnica.";
    vance.reactNeutral  = "En el espacio todo lo que puede fallar, falla. Necesito mas margen de seguridad.";
    vance.reactNegative = "No pienso firmar el lanzamiento de una tripulacion con ese nivel de riesgo.";
    vance.reactPickGood = "Redundancia, margen del 20% y plan de aborto claro. Ingenieria solida.";
    vance.reactPickBad  = "Confiar en que nada fallara no es un plan de ingenieria aceptable.";
    vance.sprite = LoadTexture("assets/sprites/slick.png");
    vance.fallbackColor = ORANGE;

    judges = { hale, brooks, vance };

    // Propuestas iniciales (el indice 0 es la mas solida)
    pitchOptions = {
        "4 astronautas exploraran el polo sur lunar buscando hielo. Pedimos 5.500M$: 4.100M$ cohete y capsula, 1.000M$ modulo lunar y 400M$ de reserva.",
        "4 astronautas iran al polo sur lunar buscando hielo. Solo necesitamos 900M$ porque reutilizaremos piezas viejas y compartiremos gastos.",
        "4 astronautas iran al polo sur lunar, montaran una base y probaran un rover de Marte por 5.500M$, pagando lo que cueste la obra."
    };

    StartDialogue(-1, "FASE 1 - AUDIENCIA DE PRESUPUESTO\nEres el economista de la mision. Convence al comite para financiar el viaje tripulado a la Luna.\nConsejo: Pulsa DATOS NASA (arriba a la izquierda) para consultar costes reales de auditoria.");
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
    int delta = -10 + std::min(hits, 5) * 6 + (text.size() > 140 ? 4 : (text.size() < 45 ? -6 : 0)) + (hasDigit ? 4 : 0);
    return std::clamp(delta, -18, 20);
}

std::string CommitteePhase::GetJudgeReaction(const BoardJudge& j, int delta) const {
    if (delta > 6)  return j.reactPositive;
    if (delta < -3) return j.reactNegative;
    return j.reactNeutral;
}

void CommitteePhase::EvaluatePitch(const std::string& text, bool pickedBest, bool usedPickMode) {
    for (int i = 0; i < 3; ++i) {
        int d = ScoreFreeText(judges[i], text);
        if (usedPickMode) {
            d = pickedBest ? std::max(d, 12) : std::min(d, 4);
        }
        pendingDeltas[i] = d;
        pendingReactions[i] = GetJudgeReaction(judges[i], d);
    }
}

void CommitteePhase::EvaluateJudgeAnswer(int judgeIdx, const std::string& text, bool pickedBest, bool usedPickMode) {
    int d = ScoreFreeText(judges[judgeIdx], text);
    if (usedPickMode) {
        d = pickedBest ? std::max(d, 20) : std::min(d, 6);
    }
    pendingDeltas[judgeIdx] = d;
    if (usedPickMode && pickedBest) {
        pendingReactions[judgeIdx] = judges[judgeIdx].reactPickGood;
    } else if (usedPickMode) {
        pendingReactions[judgeIdx] = judges[judgeIdx].reactPickBad;
    } else {
        pendingReactions[judgeIdx] = GetJudgeReaction(judges[judgeIdx], d);
    }
}

void CommitteePhase::ComputeVerdict(SharedMissionData& missionData) {
    totalApprovedMillions = 0;
    yesVotesCount = 0;

    for (auto& j : judges) {
        if (j.mood >= 40) {
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
            StartDialogue(-1, "Como quieres responder al comite?\n[1] ELEGIR ENTRE 3: Selecciona una de las tres respuestas preparadas.\n[2] ESCRIBIR YO MISMO: Redacta tus propios argumentos y cifras.");
            break;

        case Stage::PITCH_INTRO:
            stage = Stage::PITCH_INPUT;
            userTypedText.clear();
            if (!writeMode) {
                PrepareShuffledChoices(pitchOptions);
                StartDialogue(-2, "Elige tu propuesta inicial para el Comite de la NASA:");
            } else {
                StartDialogue(-2, "Escribe tu propuesta inicial (objetivo, coste en millones y seguridad):");
            }
            break;

        case Stage::PITCH_REACT:
            if (reactStepIdx < 3) {
                ApplyMoodDelta(reactStepIdx, pendingDeltas[reactStepIdx]);
                StartDialogue(reactStepIdx, pendingReactions[reactStepIdx]);
                reactStepIdx++;
            } else {
                stage = Stage::ROUND_INTRO;
                StartDialogue(-1, "Ronda de preguntas. Cada miembro del comite te hara una pregunta clave para decidir su voto.");
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
                StartDialogue(-2, "Elige tu respuesta para " + judges[currentJudgeIdx].name + ":");
            } else {
                StartDialogue(-2, "Escribe tu respuesta para " + judges[currentJudgeIdx].name + ":");
            }
            break;

        case Stage::JUDGE_REACT:
            currentJudgeIdx++;
            if (currentJudgeIdx < 3) {
                stage = Stage::JUDGE_ASK;
                StartDialogue(currentJudgeIdx, judges[currentJudgeIdx].question);
            } else {
                stage = Stage::VERDICT_INTRO;
                StartDialogue(-1, "El comite procede a votar. Quien tenga menos de 40 de aprobacion no aporta fondos. Necesitas al menos 2 votos a favor.");
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
                    StartDialogue(reactStepIdx, TextFormat("Aprobacion %d/100. Voto SI y aporto $%d millones ($%.2f B).", j.mood, j.pledgedMillions, j.pledgedMillions / 1000.0f));
                } else {
                    StartDialogue(reactStepIdx, TextFormat("Aprobacion %d/100. No me has convencido. Voto NO.", j.mood));
                }
                reactStepIdx++;
            } else {
                stage = Stage::VERDICT_FINAL;
                if (yesVotesCount >= 2) {
                    StartDialogue(1, TextFormat("MISION APROBADA: $%.2f BILLONES! Pasa al Laboratorio para comprar las piezas y ensamblar tu cohete.", totalApprovedMillions / 1000.0f));
                } else {
                    StartDialogue(1, "MISION RECHAZADA: No lograste al menos 2 votos a favor. Mejora tus cifras y vuelve a intentarlo.");
                }
            }
            break;

        default:
            break;
    }
}

GameState CommitteePhase::Update(SharedMissionData& missionData) {
    float dt = GetFrameTime();

    if (visibleChars < static_cast<float>(currentText.size())) {
        visibleChars = std::min(static_cast<float>(currentText.size()), visibleChars + 95.0f * dt);
    }

    for (auto& j : judges) {
        if (j.popTimer > 0.0f) j.popTimer = std::max(0.0f, j.popTimer - dt);
    }

    Vector2 mousePos = GetMousePosition();

    Rectangle factBtn = { 330.0f, 34.0f, 125.0f, 34.0f };
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, factBtn)) {
        showNasaFacts = !showNasaFacts;
        return GameState::COMMITTEE;
    }

    if (showNasaFacts) {
        Rectangle closeFactBtn = { SCREEN_WIDTH - 235.0f, SCREEN_HEIGHT - 105.0f, 155.0f, 42.0f };
        if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, closeFactBtn)) || IsKeyPressed(KEY_ESCAPE)) {
            showNasaFacts = false;
        }
        return GameState::COMMITTEE;
    }

    if (stage == Stage::MODE_SELECT) {
        Rectangle btnPick  = { 735.0f, 645.0f, 245.0f, 44.0f };
        Rectangle btnWrite = { 1000.0f, 645.0f, 240.0f, 44.0f };

        if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, btnPick)) || IsKeyPressed(KEY_ONE)) {
            writeMode = false;
            stage = Stage::PITCH_INTRO;
            StartDialogue(1, "Tienes la palabra. Explicanos en que consiste esta mision y por que debemos financiarla.");
        } else if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, btnWrite)) || IsKeyPressed(KEY_TWO)) {
            writeMode = true;
            stage = Stage::PITCH_INTRO;
            StartDialogue(1, "Tienes la palabra. Explicanos en que consiste esta mision y por que debemos financiarla.");
        }
        return GameState::COMMITTEE;
    }

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
        bool canSend = userTypedText.size() >= 15;
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

    Rectangle nextBtn = { 985.0f, 645.0f, 245.0f, 44.0f };
    bool clickedNext = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, nextBtn);

    if (clickedNext || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (visibleChars < static_cast<float>(currentText.size())) {
            visibleChars = static_cast<float>(currentText.size());
        } else if (stage == Stage::VERDICT_FINAL) {
            if (yesVotesCount >= 2) {
                return GameState::HANGAR;
            } else {
                Init();
            }
        } else {
            AdvanceStage(missionData);
        }
    }

    return GameState::COMMITTEE;
}

void CommitteePhase::Draw(const SharedMissionData& missionData) const {
    if (texBg.id > 0) {
        DrawTexturePro(texBg, { 0, 0, (float)texBg.width, (float)texBg.height },
                       { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT }, { 0, 0 }, 0.0f, WHITE);
    } else {
        ClearBackground(Color{ 39, 49, 92, 255 });
    }

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

        if (j.popTimer > 0.0f && j.lastDelta != 0) {
            Color popCol = (j.lastDelta > 0) ? GREEN : RED;
            DrawText(TextFormat("%+d", j.lastDelta), barX - 10, static_cast<int>(barY - 52 - (1.5f - j.popTimer) * 15.0f), 22, popCol);
        }
    }

    if (texDesk.id > 0) {
        DrawTexturePro(texDesk, { 0, 0, (float)texDesk.width, (float)texDesk.height },
                       { 470.0f, 228.0f, 810.0f, 492.0f }, { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangle(480, 250, 780, 260, Color{ 107, 70, 48, 255 });
        DrawRectangleLinesEx({ 480, 250, 780, 260 }, 4.0f, BLACK);
    }

    for (const auto& j : judges) {
        Rectangle plate = { j.posX - 84.0f, 282.0f, 168.0f, 42.0f };
        DrawRectangleRec(plate, Color{ 242, 193, 78, 255 });
        DrawRectangleLinesEx(plate, 3.0f, BLACK);
        DrawText(j.name.c_str(), static_cast<int>(plate.x + 8), static_cast<int>(plate.y + 6), 12, BLACK);
        DrawText(j.role.c_str(), static_cast<int>(plate.x + 8), static_cast<int>(plate.y + 22), 12, Color{ 50, 40, 20, 255 });
    }

    DrawRectangle(20, 20, 450, 235, Color{ 13, 17, 36, 255 });
    DrawRectangleLinesEx({ 20, 20, 450, 235 }, 4.0f, Color{ 242, 193, 78, 255 });
    DrawText("PRESUPUESTO MISION", 38, 36, 19, Color{ 242, 193, 78, 255 });

    Rectangle factBtn = { 330.0f, 34.0f, 125.0f, 34.0f };
    DrawRectangleRec(factBtn, Color{ 242, 193, 78, 255 });
    DrawRectangleLinesEx(factBtn, 2.0f, BLACK);
    DrawText("DATOS NASA", 340, 44, 15, BLACK);

    if (stage == Stage::VERDICT_FINAL && yesVotesCount >= 2) {
        DrawText(TextFormat("$ %.2f BILLONES", totalApprovedMillions / 1000.0f), 38, 74, 30, WHITE);
    } else if (stage == Stage::VERDICT_FINAL) {
        DrawText("$ 0.0 BILLONES", 38, 74, 30, RED);
    } else {
        DrawText("$ ??? BILLONES", 38, 74, 30, WHITE);
    }

    for (size_t i = 0; i < judges.size(); ++i) {
        int rowY = 128 + static_cast<int>(i) * 36;
        DrawText(judges[i].name.c_str(), 38, rowY, 17, Color{ 201, 208, 240, 255 });
        const char* pledgeTxt = (judges[i].pledgedMillions < 0) ? "..." :
                                (judges[i].pledgedMillions == 0) ? "NO" :
                                TextFormat("$ %.2f B", judges[i].pledgedMillions / 1000.0f);
        DrawText(pledgeTxt, 355, rowY, 19, Color{ 242, 193, 78, 255 });
    }

    if (texHero.id > 0) {
        DrawTexturePro(texHero, { 0, 0, (float)texHero.width, (float)texHero.height },
                       { 8.0f, 445.0f, 225.0f, 315.0f }, { 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangle(25, 480, 180, 240, DARKBLUE);
    }

    if (!writeMode && (stage == Stage::PITCH_INPUT || stage == Stage::JUDGE_ANSWER)) {
        DrawRectangle(245, 245, 1015, 280, Color{ 244, 236, 216, 255 });
        DrawRectangleLinesEx({ 245, 245, 1015, 280 }, 4.0f, BLACK);
        DrawText("ELIGE TU RESPUESTA [Teclas 1-3 o Clic]:", 265, 260, 18, Color{ 90, 83, 72, 255 });

        Vector2 mousePos = GetMousePosition();
        for (size_t i = 0; i < currentChoices.size(); ++i) {
            Rectangle optRect = { 260.0f, 295.0f + i * 74.0f, 985.0f, 64.0f };
            bool hover = CheckCollisionPointRec(mousePos, optRect);
            DrawRectangleRec(optRect, hover ? Color{ 242, 193, 78, 255 } : Color{ 255, 250, 240, 255 });
            DrawRectangleLinesEx(optRect, 3.0f, BLACK);

            const char* letter = (i == 0) ? "[1]" : ((i == 1) ? "[2]" : "[3]");
            DrawText(letter, 272, static_cast<int>(optRect.y + 18), 20, BLACK);
            DrawWrappedText(currentChoices[i].text, 318.0f, optRect.y + 10.0f, 910.0f, 18, Color{ 27, 27, 38, 255 });
        }
    }

    DrawRectangle(245, 535, 1015, 170, Color{ 244, 236, 216, 255 });
    DrawRectangleLinesEx({ 245, 535, 1015, 170 }, 4.0f, BLACK);

    std::string speakerName = "NARRADOR";
    if (activeSpeakerIdx == -2) speakerName = "TU - ECONOMISTA DE MISION";
    else if (activeSpeakerIdx >= 0 && activeSpeakerIdx < 3) speakerName = judges[activeSpeakerIdx].name;

    int tagW = MeasureText(speakerName.c_str(), 18) + 28;
    DrawRectangle(262, 514, tagW, 32, Color{ 27, 27, 38, 255 });
    DrawRectangleLinesEx({ 262.0f, 514.0f, (float)tagW, 32.0f }, 3.0f, BLACK);
    DrawText(speakerName.c_str(), 276, 521, 18, Color{ 242, 193, 78, 255 });

    if (writeMode && (stage == Stage::PITCH_INPUT || stage == Stage::JUDGE_ANSWER)) {
        DrawText(currentText.c_str(), 268, 554, 18, Color{ 90, 83, 72, 255 });
        DrawRectangle(268, 580, 780, 108, Color{ 255, 250, 240, 255 });
        DrawRectangleLinesEx({ 268, 580, 780, 108 }, 3.0f, BLACK);
        DrawWrappedText(userTypedText + "_", 278.0f, 590.0f, 760.0f, 19, BLACK);

        Rectangle sendBtn = { 1065.0f, 645.0f, 160.0f, 44.0f };
        bool canSend = userTypedText.size() >= 15;
        DrawRectangleRec(sendBtn, canSend ? Color{ 27, 27, 38, 255 } : GRAY);
        DrawText("ENVIAR >", 1095, 657, 19, WHITE);
    } else {
        std::string shown = currentText.substr(0, static_cast<size_t>(visibleChars));
        DrawWrappedText(shown, 268.0f, 558.0f, 965.0f, 21, Color{ 27, 27, 38, 255 });

        if (stage == Stage::MODE_SELECT) {
            Rectangle btnPick  = { 735.0f, 645.0f, 245.0f, 44.0f };
            Rectangle btnWrite = { 1000.0f, 645.0f, 240.0f, 44.0f };
            DrawRectangleRec(btnPick, Color{ 27, 27, 38, 255 });
            DrawText("[1] ELEGIR ENTRE 3", 752, 657, 17, Color{ 242, 193, 78, 255 });
            DrawRectangleRec(btnWrite, Color{ 27, 27, 38, 255 });
            DrawText("[2] ESCRIBIR YO", 1025, 657, 17, WHITE);
        } else if (stage != Stage::PITCH_INPUT && stage != Stage::JUDGE_ANSWER) {
            Rectangle nextBtn = { 985.0f, 645.0f, 245.0f, 44.0f };
            DrawRectangleRec(nextBtn, (stage == Stage::VERDICT_FINAL && yesVotesCount >= 2) ? DARKGREEN : Color{ 27, 27, 38, 255 });
            const char* btnLabel = (stage == Stage::VERDICT_FINAL)
                ? (yesVotesCount >= 2 ? "IR AL LABORATORIO >" : "REINTENTAR")
                : "SIGUIENTE >";
            DrawText(btnLabel, 1005, 657, 18, Color{ 242, 193, 78, 255 });
        }
    }

    if (showNasaFacts) {
        DrawNasaFactsModal();
    }
}

void CommitteePhase::DrawNasaFactsModal() const {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.75f));
    DrawRectangle(80, 60, SCREEN_WIDTH - 160, SCREEN_HEIGHT - 120, Color{ 244, 236, 216, 255 });
    DrawRectangleLinesEx({ 80, 60, SCREEN_WIDTH - 160, SCREEN_HEIGHT - 120 }, 4.0f, Color{ 242, 193, 78, 255 });

    DrawText("TUS NOTAS: CUANTO CUESTA REALMENTE IR A LA LUNA (AUDITORIA NASA OIG)", 105, 85, 20, BLACK);
    const char* bullets[] = {
        "- Lanzar el cohete SLS + capsula Orion cuesta ~4.100 millones $ por vuelo (IG-22-003).",
        "- La capsula Orion cuesta ~1.000M$ mas ~300M$ de su modulo de servicio.",
        "- Todo el programa Artemis (2012-2025) asciende a ~93.000 millones $ (IG-22-003).",
        "- Contratos de coste reembolsable se dispararon: SLS subio +33% y Artemis I +4.300M$ (IG-24-001).",
        "- Aterrizadores a PRECIO FIJO: SpaceX (2.890M$) y Blue Origin (3.400M$) apenas variaron (IG-26-004).",
        "- Sondas roboticas (CLPS) cuestan 80M$-200M$ cada una, con 26% de sobrecoste medio (IG-24-013).",
        "- Presupuesto realista para UNA mision tripulada: entre 4.000M$ y 6.500M$ (tope del comite)."
    };

    for (int i = 0; i < 7; ++i) {
        DrawText(bullets[i], 110, 138 + i * 54, 19, Color{ 27, 27, 38, 255 });
    }

    Rectangle closeBtn = { SCREEN_WIDTH - 235.0f, SCREEN_HEIGHT - 105.0f, 155.0f, 42.0f };
    DrawRectangleRec(closeBtn, Color{ 27, 27, 38, 255 });
    DrawText("CERRAR [ESC]", static_cast<int>(closeBtn.x + 16), static_cast<int>(closeBtn.y + 12), 16, Color{ 242, 193, 78, 255 });
}