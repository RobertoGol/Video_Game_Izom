#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Щит для транспортного менеджера // Защита от конфликта Winsock 1 и Winsock 2
#endif

#include <vector>
#define _WINSOCKAPI_
#include <winsock2.h>
#include <windows.h>
#include "../Data/Types.hpp" // Путь к вашему Types.hpp // Гарантирует правильное определение Vector3D для пакетов

// Transport-input-files
#include "SteamCar.hpp"
#include "Motorcycle.hpp"

namespace bunker
{

    // Перечисление типов доступной ретро-техники в Убежище 17
    // Перечисление типов транспорта (если оно не объявлено в Types.hpp, объявляем строго тут)
    enum class VehicleType
    {
        None,
        SteamCar,
        Motorcycle
    };

    class VehicleManager
    {
    private:
        VehicleType m_ActiveVehicleType = VehicleType::None;

        // Экземпляры симуляций конкретного транспорта
        SteamCarSimulation m_SteamCar;
        MotorcycleSimulation m_Motorcycle;

        // Мировые 3D координаты физического кузова машины в сессии
        Vector3D m_VehiclePosition = {0.0f, 0.0f, 0.0f};

    public:
        VehicleManager() = default;
        ~VehicleManager() = default; // Добавили деструктор для безопасности

        // 1. Посадка Пилота в транспортное средство
        void MountVehicle(VehicleType type, const Vector3D &spawnPos);

        // 2. Эвакуация / Высадка на землю пешком (Пилот становится Scout)
        void UnmountVehicle(Vector3D &outPlayerPos);

        // 3. Ежекадровое обновление физики колес, пара и векторов инерции
        void UpdateActiveVehiclePhysics(float deltaTime);

        // Геттеры состояния для HUD и шейдеров рендеринга
        VehicleType GetActiveVehicleType() const { return m_ActiveVehicleType; }
        Vector3D GetVehiclePosition() const { return m_VehiclePosition; }
        float GetCarPressurePercent() const { return m_SteamCar.GetPressurePercent(); }
    }; // Класс закрывается точкой с запятой идеально

} // namespace bunker
