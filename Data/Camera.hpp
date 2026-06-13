#pragma once
#include "../Types.hpp"

namespace bunker {

// Перечисление доступных режимов обзора бункера
enum class CameraViewMode {
    ClassicIsometric,   // Фиксированный изометрический ромб (ваша исходная математика)
    ThirdPersonFallout  // Свободная орбитальная камера от 3-го лица с вращением осей
};

class IsometricCamera {
public:
    const float ISO_COS = 0.8660254f; 
    const float ISO_SIN = 0.5000000f; 
    
    float zoom = 55.0f; 
    ScreenPoint centerOffset = { 0.0f, 0.0f }; 

    // --- НОВЫЙ ДОПOЛНИТЕЛЬНЫЙ БЛОК: РЕЖИМ FALLOUT 4 ---
    CameraViewMode currentViewMode = CameraViewMode::ClassicIsometric; // По умолчанию классика
    float cameraYaw = 0.785f;          // Угол горизонтального вращения
    float cameraPitch = 0.6f;          // Угол вертикального наклона
    float targetDistance = 8.0f;       // Расстояние до Пилота (зум)
    Vector3D currentPosition;          // 3D координаты линзы в мире

public:
    IsometricCamera() = default;

    // Метод переключения режимов (вызывается по СКМ)
    void ToggleCameraMode() {
        currentViewMode = (currentViewMode == CameraViewMode::ClassicIsometric) ? 
                          CameraViewMode::ThirdPersonFallout : CameraViewMode::ClassicIsometric;
    }

    // Универсальный апдейтер: вызывает нужный алгоритм на основе тумблера
    void UpdateCameraView(float deltaX, float deltaY, float deltaTime);

    ScreenPoint WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const;
    Vector3D ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const;
};

} // namespace bunker
