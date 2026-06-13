#pragma once
#include "../Data/Types.hpp" // Путь к вашему Types.hpp (содержит Vector3D, UnitMode и т.д.)

namespace bunker {

// Инициализация стартовых параметров Пилота/Титана (вызывается при старте InitializeVault17Map)
void InitializePlayer();

// Облегченный процессор ввода и физики движения без Kernel Mode системных вызовов
void UpdatePlayerLogic(float deltaTime, float windowWidth, float windowHeight, const ScreenPoint& cachedMouseScreenPos);

// Быстрая проверка коллизий по тайлам (AABB-клиппинг сетки)
bool CheckWorldCollision(float nextX, float nextY, float radius);

} // namespace bunker
