#pragma once
#include <string>
#include "../Player/Inventory.hpp"
#include "../Data/Types.hpp"

namespace bunker
{

    struct SaveFileHeader
    {
        char magic[4] = {'B', 'S', 'A', 'V'}; // Магическое число бинарника v15
        unsigned int version = 15;
        unsigned int slotIndex = 0;
    };

    struct PlayerSaveData
    {
        Vector3D position;
        UnitMode currentMode;
        float health;
        float maxHealth;
        float erosionLevel;
        int currentScore;
    };

    class SaveSystem
    {
    public:
        // Запись состояния сессии игрока и инвентаря в файл ./saves/slot_X.sav
        static bool WriteSaveGame(unsigned int slotSlot, const PlayerSaveData &playerState, const PlayerInventory &inventory);

        // Чтение и левелинговое восстановление сущности из файла сохранения
        static bool ReadSaveGame(unsigned int slotSlot, PlayerSaveData &playerStateOut, PlayerInventory &inventoryOut);
    };

} // namespace bunker
