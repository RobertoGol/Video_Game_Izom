#include "Inventory.hpp"
#include <algorithm>

namespace bunker {

void PlayerInventory::RecalculateWeight() 
{
    m_CurrentWeight = 0.0f;
    for (const auto& item : m_Slots) 
    {
        m_CurrentWeight += item.weightPerUnit * static_cast<float>(item.quantity);
    }
}

bool PlayerInventory::AddItem(unsigned int id, ItemType type, int count, float weight, const std::wstring& name) 
{
    // Проверяем, есть ли уже такой предмет для стакания патронов/ресурсов
    auto it = std::find_if(m_Slots.begin(), m_Slots.end(), [id](const InventoryItem& item) {
        return item.itemID == id;
    });

    // Проверка перегруза рюкзака Pip-Pad
    float potentialWeight = m_CurrentWeight + (weight * static_cast<float>(count));
    if (potentialWeight > m_MaxWeight) return false;

    if (it != m_Slots.end()) 
    {
        it->quantity += count;
    } 
    else 
    {
        m_Slots.push_back({ id, type, count, weight, name });
    }

    RecalculateWeight();
    return true;
}

bool PlayerInventory::RemoveItem(unsigned int id, int count) 
{
    auto it = std::find_if(m_Slots.begin(), m_Slots.end(), [id](const InventoryItem& item) {
        return item.itemID == id;
    });

    if (it == m_Slots.end() || it->quantity < count) return false;

    it->quantity -= count;
    if (it->quantity <= 0) 
    {
        m_Slots.erase(it);
    }

    RecalculateWeight();
    return true;
}

void PlayerInventory::ClearInventory() 
{
    m_Slots.clear();
    m_CurrentWeight = 0.0f;
}

} // namespace bunker
