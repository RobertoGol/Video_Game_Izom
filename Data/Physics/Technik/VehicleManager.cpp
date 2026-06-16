#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Защита от конфликта Winsock 1.0 и Winsock 2.0
#endif
#define _WINSOCKAPI_ // Блокируем автоматическое подключение winsock.h
#include <windows.h>
// Теперь идут твои стандартные инклуды файла:

#include "VehicleManager.hpp"
#include "../Mechanics.hpp"  // ОБЯЗАТЕЛЬНО: Дает машинам доступ к функции CheckWorldCollision!
#include "../../../main.hpp" // Доступ к глобальным playerPos, playerMode и т.д.

namespace bunker
{

    void VehicleManager::MountVehicle(VehicleType type, const Vector3D &spawnPos)
    {
        if (playerMode == UnitMode::Titan)
            return; // Из Титана нельзя пересесть в авто

        m_ActiveVehicleType = type;
        m_VehiclePosition = spawnPos;

        // Блокируем стандартное WASD перемещение пехотинца, переключая режим
        playerMode = UnitMode::Scout; // Для прототипа используем Scout флаг с флагом техники в менеджере классов
        gameClasses.EnterVehicle();   // Переключаем пассивные StatModifiers
    }

    void VehicleManager::UnmountVehicle(Vector3D &outPlayerPos)
    {
        if (m_ActiveVehicleType == VehicleType::None)
            return;

        // Высаживаем игрока на полклетки правее кузова машины, чтобы не застрять в текстуре
        outPlayerPos = m_VehiclePosition;
        outPlayerPos.x += 0.5f;

        m_ActiveVehicleType = VehicleType::None;
        gameClasses.ExitVehicle(); // Возвращаем характеристики пехотинца
    }

    void VehicleManager::UpdateActiveVehiclePhysics(float deltaTime)
    {
        // Клавиша 'X' — принудительный выход из любого транспорта на ходу
        if (GetAsyncKeyState('X') & 0x8000)
        {
            UnmountVehicle(playerPos);
            return;
        }

        switch (m_ActiveVehicleType)
        {
        case VehicleType::SteamCar:
            m_SteamCar.UpdateCarPhysics(m_VehiclePosition, deltaTime);
            playerPos = m_VehiclePosition; // Привязываем камеру и хитбокс игрока к машине
            break;

        case VehicleType::Motorcycle:
            m_Motorcycle.UpdateMotorcyclePhysics(m_VehiclePosition, deltaTime);
            playerPos = m_VehiclePosition;
            break;

        default:
            break;
        }
    }

} // namespace bunker
