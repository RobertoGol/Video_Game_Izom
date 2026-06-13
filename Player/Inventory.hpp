#pragma once
#include <string>
#include <vector>
#include "../Data/Types.hpp"

namespace bunker {

enum class ItemType { Weapon, Ammo, Medicine, Resource, Quest };

struct InventoryItem {
    unsigned int itemID = 0;
    ItemType type = ItemType::Resource;
    int quantity = 1;
    float weightPerUnit = 0.1f;
    std::wstring displayName = L"UNKNOWN ITEM";
};

class PlayerInventory {
private:
    std::vector<InventoryItem> m_Slots;
    float m_MaxWeight = 45.0f; // Ограничение по весу в стиле Fallout
    float m_CurrentWeight = 0.0f;

public:
    PlayerInventory() = default;

    bool AddItem(unsigned int id, ItemType type, int count, float weight, const std::wstring& name);
    bool RemoveItem(unsigned int id, int count);
    void ClearInventory();
    
    // Пересчет текущей массы предметов в рюкзаке
    void RecalculateWeight();

    const std::vector<InventoryItem>& GetSlots() const { return m_Slots; }
    float GetCurrentWeight() const { return m_CurrentWeight; }
    float GetMaxWeight() const     { return m_MaxWeight; }
};

} // namespace bunker
