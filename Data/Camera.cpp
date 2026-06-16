#include "Camera.hpp"
#include "../main.hpp" // Доступ к глобальным playerPos, mouseWorldPos, cameraTarget, isAiming
#include <cmath>
#include <algorithm>

namespace bunker
{ // ВХОДИМ В ПРОСТРАНСТВО ИМЕН ДВИЖКА (Устраняет ошибки линковки C/C++)

    void UpdateHelldiversCamera(float deltaTime)
    {
        // ваш код логики камеры Helldivers
    }

    // Обновление позиции цели камеры на основе состояния Пилота
    void IsometricCamera::UpdateHelldiversCamera(float deltaTime)
    {
        Vector3D targetGoal = {0.0f, 0.0f, 0.0f};

        if (isAiming)
        {
            // Смещение фокуса на 35% в сторону прицела Pip-Pad для тактического обзора
            targetGoal.x = playerPos.x * 0.65f + mouseWorldPos.x * 0.35f;
            targetGoal.y = playerPos.y * 0.65f + mouseWorldPos.y * 0.35f;
            targetGoal.z = 0.0f;
        }
        else
        {
            // Камера центрируется ровно на координатах игрока
            targetGoal.x = playerPos.x;
            targetGoal.y = playerPos.y;
            targetGoal.z = 0.0f;
        }

        // Линейная интерполяция (LERP) со скоростью 4.5f для плавности хода линзы без статтеров
        cameraTarget.x += (targetGoal.x - cameraTarget.x) * 4.5f * deltaTime;
        cameraTarget.y += (targetGoal.y - cameraTarget.y) * 4.5f * deltaTime;
        cameraTarget.z += (targetGoal.z - cameraTarget.z) * 4.5f * deltaTime;
    }

    // Преобразование мировых 3D-координат в плоские экранные координаты
    ScreenPoint IsometricCamera::WorldToScreen(const Vector3D &worldPos, const Vector3D &cTarget) const
    {
        ScreenPoint screen;

        // Вычисляем дельту позиции объекта относительно фокуса камеры
        float dx = worldPos.x - cTarget.x;
        float dy = worldPos.y - cTarget.y;
        float dz = worldPos.z - cTarget.z;

        // Проекция ромба изометрии (2:1 по пикселям) с учетом высоты объекта (Z)
        screen.x = (dx - dy) * ISO_COS * zoom + centerOffset.x;
        screen.y = (dx + dy) * ISO_SIN * zoom - (dz * zoom) + centerOffset.y;

        return screen;
    }

    // Обратная проекция: перевод экранных пикселей мыши в 3D-координаты пола карты
    Vector3D IsometricCamera::ScreenToWorldGround(const ScreenPoint &screenPos, const Vector3D &cTarget) const
    {
        Vector3D world;

        // Убираем смещение центра экрана и масштабирование зума
        float sx = (screenPos.x - centerOffset.x) / zoom;
        float sy = (screenPos.y - centerOffset.y) / zoom;

        // Обратная матрица уравнений изометрической проекции для плоскости Z = 0
        float x_rel = (sy / (2.0f * ISO_SIN)) + (sx / (2.0f * ISO_COS));
        float y_rel = (sy / (2.0f * ISO_SIN)) - (sx / (2.0f * ISO_COS));

        // Смещаем результат относительно текущей позиции камеры в мире
        world.x = x_rel + cTarget.x;
        world.y = y_rel + cTarget.y;
        world.z = 0.0f;

        return world;
    }

} // namespace bunker // ЗАКРЫВАЕМ ПРОСТРАНСТВО ИМЕН
