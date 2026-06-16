#pragma once
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Запрещаем ломать сборку сетевыми дубликатами
#endif

#define _WINSOCKAPI_ // Глушим старый winsock.h
#include <windows.h> // Обеспечивает тип HWND для CaptureWin32Input
#include <vector>
#include "Types.hpp" // Базовые типы Vector3D и Vertex вашего движка

namespace bunker
{

    // Структура для хранения сырого физического ввода от Win32 API в текущем кадре
    struct CustomPlayerInput
    {
        float moveForward = 0.0f;
        float moveStrafe = 0.0f;
        bool isSprinting = false;
        bool isDiving = false;
        bool isAiming = false;
    };

    class PlayerController
    {
    private:
        // Параметры выносливости Пилота (Менеджмент костюма)
        float m_CurrentStamina = 100.0f;
        float m_MaxStamina = 100.0f;
        float m_StaminaDrain = 30.0f;
        float m_StaminaRegen = 15.0f;

        // Параметры физики скольжения персонажа
        Vector3D m_Velocity = {0.0f, 0.0f, 0.0f};
        float m_WalkSpeed = 4.0f;
        float m_SprintSpeed = 7.0f;
        float m_Acceleration = 10.0f;
        float m_Deceleration = 8.0f;

        // Параметры механики нырка (Dive) в стиле Helldivers 2
        bool m_IsDiving = false;
        float m_DiveTimer = 0.0f;
        Vector3D m_DiveDirection = {0.0f, 0.0f, 0.0f};

        // Текущий круговой угол взгляда Пилота (в градусах от 0 до 360)
        float m_FacingAngle = 0.0f;

    public:
        PlayerController() = default;
        ~PlayerController() = default;

        // Метод физического опроса клавиатуры
        void CaptureWin32Input(HWND hwnd, CustomPlayerInput &inputOut);

        // Главный цикл обновления движения, который вызывается каждый кадр из main.cpp
        void UpdateMovement(HWND hwnd, float deltaTime, float wWidth, float wHeight, float &playerRadius);

        // Вспомогательная тригонометрия расчета взгляда
        float CalculateFacingAngle(const Vector3D &pPos, const Vector3D &mPos);

        // Геттер для получения угла поворота при отрисовке спрайта
        float GetFacingAngle() const { return m_FacingAngle; }
    };

} // namespace bunker
