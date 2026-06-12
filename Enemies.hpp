#pragma once
#include <vector>

// Заявляем, что структуры и глобальные переменные существуют в проекте
struct Vector3D;
struct Enemy;

// Системные функции управления роем Верминов
void SpawnVerminGrid(float deltaTime);
void UpdateVerminAI(float deltaTime);
