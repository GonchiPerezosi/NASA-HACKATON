#pragma once

#include "game_types.h"
#include "raylib.h"
#include <vector>

// Estructura interna exclusiva de la Tienda (evita tocar game_types.h)
struct ShopItem {
    RocketPart part;
    bool purchased = false;
};

class LabShopPhase {
public:
    void Init();
    void Unload();
    void OnEnter(SharedMissionData& missionData);
    GameState Update(SharedMissionData& missionData);
    void Draw(const SharedMissionData& missionData) const;

private:
    // Inventario de la tienda por categorías
    std::vector<ShopItem> capsuleItems;
    std::vector<ShopItem> tankItems;
    std::vector<ShopItem> engineItems;

    // Pestaña activa en la tienda (0 = Cápsulas, 1 = Tanques, 2 = Motores)
    int activeTab = 0;

    // Índices de las piezas actualmente montadas en el cohete (-1 = ranura vacía)
    int equippedCapsuleIdx = -1;
    int equippedTankIdx    = -1;
    int equippedEngineIdx  = -1;

    // Mensaje de estado en la tienda (ej. "Pieza comprada", "Saldo insuficiente")
    std::string shopFeedback = "Selecciona una categoría y compra las 3 piezas para tu cohete.";
    Color feedbackColor = LIGHTGRAY;

    // --- HUECOS PARA TUS SPRITES ---
    Texture2D texLabBg    = { 0 };
    Texture2D texCapsule  = { 0 };
    Texture2D texTank     = { 0 };
    Texture2D texEngine   = { 0 };

    void HandleShopInteraction(std::vector<ShopItem>& currentList, int& equippedIdx, SharedMissionData& missionData);
    void DrawShopList(const std::vector<ShopItem>& currentList, int equippedIdx, float budget) const;
    void RecalculateRocketStats(SharedMissionData& missionData);
};
