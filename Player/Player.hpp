#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void HandleInput(HWND hwnd, float deltaTime, float wWidth, float wHeight);
bool CheckWorldCollision(float nextX, float nextY, float radius);
