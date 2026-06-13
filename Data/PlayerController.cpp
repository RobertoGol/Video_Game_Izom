#include "PlayerController.hpp"
#include "../../main.hpp" 
#include <cmath>
#include <algorithm>

namespace bunker {

void PlayerController::CaptureWin32Input(HWND hwnd, CustomPlayerInput& inputOut) {
    inputOut.moveForward = 0.0f;
    inputOut.moveStrafe = 0.0f;

    // Опрос физического состояния клавиш Win32 API
    if (GetAsyncKeyState('W') & 0x8000) { inputOut.moveForward += 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { inputOut.moveForward -= 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { inputOut.moveStrafe -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { inputOut.moveStrafe += 1.0f; }

    inputOut.isSprinting = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
    inputOut.isDiving    = (GetAsyncKeyState(VK_SPACE) & 0x8000);
    inputOut.isAiming    = (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
}

void PlayerController::UpdateMovement(HWND hwnd, float deltaTime, float wWidth, float wHeight, float& playerRadius) {
    CustomPlayerInput input;
    CaptureWin32Input(hwnd, input);
    isAiming = input.isAiming; 

    // Динамическая установка радиуса коллизии Пилот / Титан
    playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

    // Сброс зума изометрической камеры при отсутствии прицеливания
    if (playerMode == UnitMode::Scout || !isAiming) {
        isoCamera.zoom += (55.0f - isoCamera.zoom) * 5.0f * deltaTime;
    }

    if (fireCooldown > 0.0f) {
        fireCooldown -= deltaTime;
    }

    // 1. ЧЕСТНОЕ ИЗОМЕТРИЧЕСКОЕ ДВИЖЕНИЕ КАРТЫ (W двигает строго вверх-вправо по ромбу)
    Vector3D targetDir = { 0.0f, 0.0f, 0.0f };
    if (GetAsyncKeyState('W') & 0x8000) { targetDir.x += 1.0f; targetDir.y -= 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { targetDir.x -= 1.0f; targetDir.y += 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { targetDir.x -= 1.0f; targetDir.y -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { targetDir.x += 1.0f; targetDir.y += 1.0f; }

    float len = std::sqrt(targetDir.x * targetDir.x + targetDir.y * targetDir.y);
    Vector3D targetVelocity = { 0.0f, 0.0f, 0.0f };

    // Менеджмент Очков Действия / Выносливости костюма
    bool isMoving = (len > 0.01f);
    bool canSprint = (m_CurrentStamina > 0.0f) && isMoving && (playerMode == UnitMode::Scout);
    bool activeSprint = input.isSprinting && canSprint;

    if (activeSprint) {
        m_CurrentStamina = std::max(0.0f, m_CurrentStamina - m_StaminaDrain * deltaTime);
    } else {
        m_CurrentStamina = std::min(m_MaxStamina, m_CurrentStamina + m_StaminaRegen * deltaTime);
    }

    // 2. МЕХАНИКА НЫРКА (DIVE) ПО НАПРАВЛЕНИЮ ДВИЖЕНИЯ ИГРОКА
    if (playerMode == UnitMode::Scout) {
        if (input.isDiving && !m_IsDiving && isMoving) {
            m_IsDiving = true;
            m_DiveTimer = 0.6f; 
            m_DiveDirection.x = targetDir.x / len;
            m_DiveDirection.y = targetDir.y / len;
            m_DiveDirection.z = 0.0f;
        }

        if (m_IsDiving) {
            m_DiveTimer -= deltaTime;
            if (m_DiveTimer <= 0.0f) {
                m_IsDiving = false;
            }

            // Выталкивание тела вперед физическим импульсом прыжка лежа
            float nextX = playerPos.x + m_DiveDirection.x * 9.5f * deltaTime;
            float nextY = playerPos.y + m_DiveDirection.y * 9.5f * deltaTime;
            
            if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
            if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;
            
            return; // Во время прыжка стандартное WASD смещение блокируется
        }
    }

    // 3. ПЛАВНЫЙ РАЗГОН И ГАШЕНИЕ ИНЕРЦИИ МАССЫ ТЕЛА (Убираем "деревянность")
    if (isMoving) {
        float currentMoveSpeed = activeSprint ? m_SprintSpeed : m_WalkSpeed;
        
        if (playerMode == UnitMode::Scout && isAiming) {
            currentMoveSpeed *= 0.63f;
        }
        if (playerMode == UnitMode::Titan && titan.systems.tracksCondition < 40.0f) {
            currentMoveSpeed *= 0.3f;
        }

        targetVelocity.x = (targetDir.x / len) * currentMoveSpeed;
        targetVelocity.y = (targetDir.y / len) * currentMoveSpeed;
    }

    float currentLerp = isMoving ? m_Acceleration : m_Deceleration;
    m_Velocity.x += (targetVelocity.x - m_Velocity.x) * currentLerp * deltaTime;
    m_Velocity.y += (targetVelocity.y - m_Velocity.y) * currentLerp * deltaTime;

    // Проверка коллизий по сетке карты перед сдвигом
    float nextX = playerPos.x + m_Velocity.x * deltaTime;
    float nextY = playerPos.y + m_Velocity.y * deltaTime;

    if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
    if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;

    // 4. ТРЕКИНГ МЫШИ И ПОДГРУЗКА МИРОВЫХ КООРДИНАТ ИЗ ОКРУЖЕНИЯ
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    // Вычисляем угол поворота самого игрока в мире относительно курсора
    m_FacingAngle = CalculateFacingAngle(playerPos, mouseWorldPos);
}

float PlayerController::CalculateFacingAngle(const Vector3D& pPos, const Vector3D& mPos) {
    float dx = mPos.x - pPos.x;
    float dy = mPos.y - pPos.y;
    
    if (std::abs(dx) < 0.05f && std::abs(dy) < 0.05f) {
        return m_FacingAngle;
    }

    float angle = std::atan2(dy, dx) * 180.0f / 3.14159265f;
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    
    return angle; 
}

} // namespace bunker
