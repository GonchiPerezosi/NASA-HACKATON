#pragma once

#include "game_types.h"
#include "raylib.h"
#include <string>
#include <vector>

struct BoardJudge {
    std::string id;
    std::string name;
    std::string role;
    float posX = 0.0f;
    int maxContributionMillions = 2000; // $2.0B o $2.5B según el juez[cite: 2]
    int mood = 50;                      // 0 a 100[cite: 2]
    int pledgedMillions = -1;           // -1 = pendiente, 0 = NO, >0 = aprobado[cite: 2]

    // Animación de cambio de aprobación (+12, -10, etc.)[cite: 2]
    int lastDelta = 0;
    float popTimer = 0.0f;

    std::string question;
    std::vector<std::string> options;   // options[0] siempre es la mejor antes de barajar[cite: 2]
    std::vector<std::string> keywords;  // Palabras clave para el modo de escritura libre[cite: 2]

    Texture2D sprite = { 0 };
    Color fallbackColor = SKYBLUE;
};

struct ShuffledChoice {
    std::string text;
    bool isBest = false;
};

class CommitteePhase {
public:
    void Init();
    void Unload();
    GameState Update(SharedMissionData& missionData);
    void Draw(const SharedMissionData& missionData) const;

private:
    enum class Stage {
        INTRO,
        MODE_SELECT,
        PITCH_INTRO,
        PITCH_INPUT,
        PITCH_REACT,
        ROUND_INTRO,
        JUDGE_ASK,
        JUDGE_ANSWER,
        JUDGE_REACT,
        VERDICT_INTRO,
        VERDICT_VOTES,
        VERDICT_FINAL
    };

    Stage stage = Stage::INTRO;
    bool writeMode = false;       // false = PICK FROM 3, true = WRITE MY OWN[cite: 2]
    bool showNasaFacts = false;   // Modal de datos reales de auditoría NASA OIG[cite: 2]

    std::vector<BoardJudge> judges;
    std::vector<std::string> pitchOptions;
    std::vector<ShuffledChoice> currentChoices;

    int activeSpeakerIdx = -1;    // -1 = Narrador, -2 = Tú, 0..2 = Jueces[cite: 2]
    int currentJudgeIdx = 0;
    int reactStepIdx = 0;

    std::string currentText;
    float visibleChars = 0.0f;    // Efecto máquina de escribir[cite: 2]
    std::string userTypedText;

    // Resultados de reacciones calculadas
    int pendingDeltas[3] = { 0, 0, 0 };
    std::string pendingReactions[3];

    int totalApprovedMillions = 0;
    int yesVotesCount = 0;

    // Sprites extraídos de moonshot-budget-hearing.zip[cite: 1]
    Texture2D texBg   = { 0 };
    Texture2D texDesk = { 0 };
    Texture2D texHero = { 0 };

    void StartDialogue(int speaker, const std::string& msg);
    void PrepareShuffledChoices(const std::vector<std::string>& rawOptions);
    void ApplyMoodDelta(int judgeIdx, int delta);
    int ScoreFreeText(const BoardJudge& j, const std::string& text) const;
    std::string GetReactionLine(int delta) const;
    void EvaluatePitch(const std::string& text, bool pickedBest, bool usedPickMode);
    void EvaluateJudgeAnswer(int judgeIdx, const std::string& text, bool pickedBest, bool usedPickMode);
    void ComputeVerdict(SharedMissionData& missionData);
    void AdvanceStage(SharedMissionData& missionData);
    void DrawNasaFactsModal() const;
};
