#include "SaveSystem.hpp"
#include <fstream>
#include <iostream>
#include <windows.h>

namespace bunker
{

    bool SaveSystem::WriteSaveGame(unsigned int slotSlot, const PlayerSaveData &playerState, const PlayerInventory &inventory)
    {
        // Аппаратное создание папки ./saves/ средствами Win32, если её нет на диске
        CreateDirectoryA("./saves", nullptr);

        std::string savePath = "./saves/slot_" + std::to_string(slotSlot) + ".sav";
        std::ofstream file(savePath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return false;

        // 1. Запись заголовка валидации
        SaveFileHeader header;
        header.slotIndex = slotSlot;
        file.write(reinterpret_cast<const char *>(&header), sizeof(SaveFileHeader));

        // 2. Запись физического состояния персонажа
        file.write(reinterpret_cast<const char *>(&playerState), sizeof(PlayerSaveData));

        // 3. Сериализация динамического инвентаря рюкзака Pip-Pad
        const auto &slots = inventory.GetSlots();
        size_t inventorySize = slots.size();
        file.write(reinterpret_cast<const char *>(&inventorySize), sizeof(size_t));

        for (const auto &item : slots)
        {
            file.write(reinterpret_cast<const char *>(&item.itemID), sizeof(unsigned int));
            file.write(reinterpret_cast<const char *>(&item.type), sizeof(ItemType));
            file.write(reinterpret_cast<const char *>(&item.quantity), sizeof(int));
            file.write(reinterpret_cast<const char *>(&item.weightPerUnit), sizeof(float));

            // Сериализация широкой строки названия (размер + сырые wchar_t)
            size_t nameLen = item.displayName.size();
            file.write(reinterpret_cast<const char *>(&nameLen), sizeof(size_t));
            file.write(reinterpret_cast<const char *>(item.displayName.data()), nameLen * sizeof(wchar_t));
        }

        file.close();
        return true;
    }

    bool SaveSystem::ReadSaveGame(unsigned int slotSlot, PlayerSaveData &playerStateOut, PlayerInventory &inventoryOut)
    {
        std::string savePath = "./saves/slot_" + std::to_string(slotSlot) + ".sav";
        std::ifstream file(savePath, std::ios::binary);
        if (!file.is_open())
            return false;

        // 1. Чтение и сверка сигнатуры magic во избежание краша движка
        SaveFileHeader header;
        file.read(reinterpret_cast<char *>(&header), sizeof(SaveFileHeader));
        if (header.magic[0] != 'B' || header.magic[1] != 'S' || header.magic[2] != 'A' || header.magic[3] != 'V')
        {
            file.close();
            return false;
        }

        // 2. Восстановление базовых параметров
        file.read(reinterpret_cast<char *>(&playerStateOut), sizeof(PlayerSaveData));

        // 3. Десериализация инвентаря
        inventoryOut.ClearInventory();
        size_t inventorySize = 0;
        file.read(reinterpret_cast<char *>(&inventorySize), sizeof(size_t));

        for (size_t i = 0; i < inventorySize; ++i)
        {
            unsigned int id;
            ItemType type;
            int qty;
            float weight;
            file.read(reinterpret_cast<char *>(&id), sizeof(unsigned int));
            file.read(reinterpret_cast<char *>(&type), sizeof(ItemType));
            file.read(reinterpret_cast<char *>(&qty), sizeof(int));
            file.read(reinterpret_cast<char *>(&weight), sizeof(float));

            size_t nameLen = 0;
            file.read(reinterpret_cast<char *>(&nameLen), sizeof(size_t));
            std::wstring name(nameLen, L'\0');
            file.read(reinterpret_cast<char *>(&name[0]), nameLen * sizeof(wchar_t));

            inventoryOut.AddItem(id, type, qty, weight, name);
        }

        file.close();
        return true;
    }

} // namespace bunker
