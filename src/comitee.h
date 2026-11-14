#pragma once

#include "games_types.h"
#include "raylib.h"
#include <string>
#include <vector>

struct BoardJudge {
    std::string id;
    std::string name;
    std::string role;
    float posX = 0.0f;
    int maxContributionMillions = 2000;
    int mood = 50;
    int pledgedMillions = -1;

    int lastDelta = 0;
    float popTimer = 0.0f;

    std::string question;
    std::vector<std::string> options;
    std::vector<std::string> keywords;

    // Frases propias de cada juez para que nunca repitan la misma línea
    std::string reactPositive;
    std::string reactNeutral;
    std::string reactNegative;
    std::string reactPickGood;
    std::string reactPickBad;

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
    bool writeMode = false;
    bool showNasaFacts = false;

    std::vector<BoardJudge> judges;
    std::vector<std::string> pitchOptions;
    std::vector<ShuffledChoice> currentChoices;

    int activeSpeakerIdx = -1;
    int currentJudgeIdx = 0;
    int reactStepIdx = 0;

    std::string currentText;
    float visibleChars = 0.0f;
    std::string userTypedText;

    int pendingDeltas[3] = { 0, 0, 0 };
    std::string pendingReactions[3];

    int totalApprovedMillions = 0;
    int yesVotesCount = 0;

    Texture2D texBg   = { 0 };
    Texture2D texDesk = { 0 };
    Texture2D texHero = { 0 };

    void StartDialogue(int speaker, const std::string& msg);
    void PrepareShuffledChoices(const std::vector<std::string>& rawOptions);
    void ApplyMoodDelta(int judgeIdx, int delta);
    int ScoreFreeText(const BoardJudge& j, const std::string& text) const;
    std::string GetJudgeReaction(const BoardJudge& j, int delta) const;
    void EvaluatePitch(const std::string& text, bool pickedBest, bool usedPickMode);
    void EvaluateJudgeAnswer(int judgeIdx, const std::string& text, bool pickedBest, bool usedPickMode);
    void ComputeVerdict(SharedMissionData& missionData);
    void AdvanceStage(SharedMissionData& missionData);
    void DrawNasaFactsModal() const;
};  