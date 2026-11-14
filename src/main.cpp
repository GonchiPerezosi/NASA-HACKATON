LabShopPhase labShopPhase;
labShopPhase.Init();

// Dentro del switch(currentState):
case GameState::COMMITTEE:
    if (committeePhase.Update(missionData) == GameState::HANGAR) {
        labShopPhase.OnEnter(missionData);
        currentState = GameState::HANGAR;
    }
    break;

case GameState::HANGAR:
    if (labShopPhase.Update(missionData) == GameState::FLIGHT) {
        missionPhase.Init(missionData); // Inicializa la Fase 3 antes de entrar
        currentState = GameState::FLIGHT;
    }
    break;
