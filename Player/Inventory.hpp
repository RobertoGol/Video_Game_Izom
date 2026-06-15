#pragma once
#include <string>
#include <vector>
#include "../Data/Types.hpp"

namespace bunker {

class PlayerInventory {
private:
    std::vector<InventoryItem> m_Slots;
    float m_MaxWeight = 45.0f; 
    float m_CurrentWeight = 0.0f;

public:
    PlayerInventory() = default;

    bool AddItem(unsigned int id, ItemType type, int count, float weight, const std::wstring& name);
    bool RemoveItem(unsigned int id, int count);
    void ClearInventory();
    
    void RecalculateWeight();

    const std::vector<InventoryItem>& GetSlots() const { return m_Slots; }
    float GetCurrentWeight() const { return m_CurrentWeight; }
    float GetMaxWeight() const     { return m_MaxWeight; }
};

} // namespace bunker
