#include "save_manager.h"
#include <fstream>
#include <filesystem>

std::string SaveManager::GetSlotFilePath(int slot) {
    return "data/slot" + std::to_string(slot) + ".sav";
}

bool SaveManager::SaveGame(int slot, const SharedMissionData& data, GameState currentPhase) {
    if (slot < 1 || slot > 3) return false;

    std::error_code ec;
    std::filesystem::create_directories("data", ec);

    std::ofstream file(GetSlotFilePath(slot));
    if (!file.is_open()) return false;

    int phaseCode = 1;
    if (currentPhase == GameState::HANGAR) phaseCode = 2;
    else if (currentPhase == GameState::FLIGHT) phaseCode = 3;

    file << "MOONSHOT_SAVE_V1\n";
    file << (data.lang == Language::ES ? 0 : 1) << "\n";
    file << phaseCode << "\n";
    file << data.awardedBudget << "\n";
    file << data.remainingBudget << "\n";
    file << data.eqCapsuleIdx << "\n";
    file << data.eqLanderIdx << "\n";
    file << data.eqTankIdx << "\n";
    file << data.eqEngineIdx << "\n";
    file << data.eqBoosterIdx << "\n";
    file << data.rocket.totalCost << "\n";
    file << data.rocket.dryMassTons << "\n";
    file << data.rocket.maxFuelTons << "\n";
    file << data.rocket.thrustKN << "\n";
    file << data.rocket.twr << "\n";

    file.close();
    return true;
}

bool SaveManager::LoadGame(int slot, SharedMissionData& outData, GameState& outPhase) {
    if (slot < 1 || slot > 3) return false;

    std::ifstream file(GetSlotFilePath(slot));
    if (!file.is_open()) return false;

    std::string header;
    std::getline(file, header);
    if (header != "MOONSHOT_SAVE_V1") return false;

    int langCode = 0;
    int phaseCode = 1;

    file >> langCode
         >> phaseCode
         >> outData.awardedBudget
         >> outData.remainingBudget
         >> outData.eqCapsuleIdx
         >> outData.eqLanderIdx
         >> outData.eqTankIdx
         >> outData.eqEngineIdx
         >> outData.eqBoosterIdx
         >> outData.rocket.totalCost
         >> outData.rocket.dryMassTons
         >> outData.rocket.maxFuelTons
         >> outData.rocket.thrustKN
         >> outData.rocket.twr;

    if (file.fail()) return false;

    outData.activeSaveSlot = slot;
    outData.lang = (langCode == 0) ? Language::ES : Language::EN;

    if (phaseCode == 2) outPhase = GameState::HANGAR;
    else if (phaseCode == 3) outPhase = GameState::FLIGHT;
    else outPhase = GameState::COMMITTEE;

    outData.savedPhase = outPhase;
    return true;
}

SaveSlotSummary SaveManager::GetSlotSummary(int slot) {
    SaveSlotSummary summary;
    summary.slotNumber = slot;

    SharedMissionData temp;
    GameState phase = GameState::COMMITTEE;
    if (LoadGame(slot, temp, phase)) {
        summary.occupied = true;
        summary.lang = temp.lang;
        summary.savedPhase = phase;
        summary.awardedBudget = temp.awardedBudget;
        summary.remainingBudget = temp.remainingBudget;
        summary.dryMassTons = temp.rocket.dryMassTons;
        summary.thrustKN = temp.rocket.thrustKN;
    }
    return summary;
}

void SaveManager::DeleteSlot(int slot) {
    if (slot < 1 || slot > 3) return;
    std::error_code ec;
    std::filesystem::remove(GetSlotFilePath(slot), ec);
}