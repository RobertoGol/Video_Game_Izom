#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Защита от конфликта Winsock 1 и Winsock 2
#endif
#define _WINSOCKAPI_
#include <winsock2.h>
#include <windows.h>
#include "SteamCar.hpp"
#include "Motorcycle.hpp"
#include "../../../Data/Types.hpp" // Путь к вашему Types.hpp // Гарантирует правильное определение Vector3D для пакетов

namespace bunker {

// Перечисление типов доступной ретро-техники в Убежище 17
enum class VehicleType {
    None,
    SteamCar,     // Паромобиль на котле высокого давления
    Motorcycle    // Мотоцикл с люлькой/коляской
};

class VehicleManager {
private:
    VehicleType m_ActiveVehicleType = VehicleType::None;

    // Экземпляры симуляций конкретного транспорта
    SteamCarSimulation m_SteamCar;
    MotorcycleSimulation m_Motorcycle;

    // Мировые 3D координаты физического кузова машины в сессии
    Vector3D m_VehiclePosition = { 0.0f, 0.0f, 0.0f };  

public:
    VehicleManager() = default;

    // 1. Посадка Пилота в транспортное средство
    void MountVehicle(VehicleType type, const Vector3D& spawnPos);

    // 2. Эвакуация / Высадка на землю пешком (Пилот становится Scout)
    void UnmountVehicle(Vector3D& outPlayerPos);

    // 3. Ежекадровое обновление физики колес, пара и векторов инерции
    void UpdateActiveVehiclePhysics(float deltaTime);

    // Геттеры состояния для HUD и шейдеров рендеринга
    VehicleType GetActiveVehicleType() const { return m_ActiveVehicleType; }
    Vector3D GetVehiclePosition() const      { return m_VehiclePosition; }   
    float GetCarPressurePercent() const      { return m_SteamCar.GetPressurePercent(); }
};

} // namespace bunker
