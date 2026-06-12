#include "../main.hpp"
#include "Camera.hpp"
#include <cmath>

void UpdateHelldiversCamera(float deltaTime) {
    Vector3D targetGoal;
    if (isAiming) {
        // Смещение фокуса на 35% в сторону прицела Pip-Pad
        targetGoal.x = playerPos.x * 0.65f + mouseWorldPos.x * 0.35f;
        targetGoal.y = playerPos.y * 0.65f + mouseWorldPos.y * 0.35f;
    } else {
        targetGoal.x = playerPos.x;
        targetGoal.y = playerPos.y;
    }
    // Линейная интерполяция (LERP) для плавности хода линзы
    cameraTarget.x += (targetGoal.x - cameraTarget.x) * 4.5f * deltaTime;
    cameraTarget.y += (targetGoal.y - cameraTarget.y) * 4.5f * deltaTime;
}
