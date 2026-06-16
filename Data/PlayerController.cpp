#include "PlayerController.hpp" // Подключаем собственный заголовочный файл с объявлением класса контроллера
#include "../../main.hpp"       // Подключаем главный файл проекта для доступа к глобальным переменным (playerPos, isoCamera)
#include <cmath>                // Подключаем стандартную математическую библиотеку для корней (std::sqrt) и тригонометрии (std::atan2)
#include <algorithm>            // Подключаем STL алгоритмы для работы функций ограничения диапазонов (std::min, std::max)

namespace bunker
{ // Входим в пространство имен нашего игрового движка
    // МЕХАНИКА 1: Физический опрос клавиатуры и мыши через Win32 API
    void PlayerController::CaptureWin32Input(HWND hwnd, CustomPlayerInput &inputOut)
    {
        inputOut.moveForward = 0.0f; // Сбрасываем вертикальный вектор ввода перед новым опросом
        inputOut.moveStrafe = 0.0f;  // Сбрасываем горизонтальный вектор ввода перед новым опросом

        // Опрашиваем физическое состояние клавиш WASD. Маска & 0x8000 проверяет, зажата ли клавиша в текущий кадр
        if (GetAsyncKeyState('W') & 0x8000)
        {
            inputOut.moveForward += 1.0f;
        } // Игрок жмет W -> движение вперед
        if (GetAsyncKeyState('S') & 0x8000)
        {
            inputOut.moveForward -= 1.0f;
        } // Игрок жмет S -> движение назад
        if (GetAsyncKeyState('A') & 0x8000)
        {
            inputOut.moveStrafe -= 1.0f;
        } // Игрок жмет A -> стрейф влево
        if (GetAsyncKeyState('D') & 0x8000)
        {
            inputOut.moveStrafe += 1.0f;
        } // Игрок жмет D -> стрейф вправо

        // Опрашиваем спец-клавиши управления тактиками Пилота по канону Titanfall 2
        inputOut.isSprinting = (GetAsyncKeyState(VK_SHIFT) & 0x8000); // Бег (Shift)
        inputOut.isDiving = (GetAsyncKeyState(VK_SPACE) & 0x8000);    // Прыжок-нырок в стиле Helldivers 2 (Пробел)
        inputOut.isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000);  // Прицеливание (Правая кнопка мыши)
    }
    // МЕХАНИКА 2: Основной цикл обновления вектора движения и обработки коллизий
    void PlayerController::UpdateMovement(HWND hwnd, float deltaTime, float wWidth, float wHeight, float &playerRadius)
    {
        CustomPlayerInput input;        // Создаем локальную структуру для текущего кадра
        CaptureWin32Input(hwnd, input); // Заполняем её физическим состоянием клавиш
        isAiming = input.isAiming;      // Синхронизируем глобальный флаг прицеливания для рендерера оружия
        // Динамически меняем физический размер хитбокса: если игрок управляет огромным Титаном, радиус коллизии увеличивается
        playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;
        // Сброс и плавный возврат зума изометрической камеры к базовым 55 градусам, если игрок не целится
        if (playerMode == UnitMode::Scout || !isAiming)
        {
            isoCamera.zoom += (55.0f - isoCamera.zoom) * 5.0f * deltaTime; // Линейная интерполяция (Lerp) зума
        }
        // Менеджмент кулдауна (перезарядки) оружия Пилота
        if (fireCooldown > 0.0f)
        {
            fireCooldown -= deltaTime; // Уменьшаем таймер задержки выстрела на прошедшее время кадра
        }
        // МЕХАНИКА 3: Честное изометрическое движение по ромбу карты (Канон проекции v15)
        Vector3D targetDir = {0.0f, 0.0f, 0.0f}; // Инициализируем вектор целевого смещения
        if (GetAsyncKeyState('W') & 0x8000)
        {
            targetDir.x += 1.0f;
            targetDir.y -= 1.0f;
        } // W двигает по ромбу строго вверх-вправо
        if (GetAsyncKeyState('S') & 0x8000)
        {
            targetDir.x -= 1.0f;
            targetDir.y += 1.0f;
        } // S двигает строго вниз-влево
        if (GetAsyncKeyState('A') & 0x8000)
        {
            targetDir.x -= 1.0f;
            targetDir.y -= 1.0f;
        } // A двигает строго вверх-влево
        if (GetAsyncKeyState('D') & 0x8000)
        {
            targetDir.x += 1.0f;
            targetDir.y += 1.0f;
        } // D двигает строго вниз-вправо
        // Вычисляем длину вектора направления движения для последующей нормализации
        float len = std::sqrt(targetDir.x * targetDir.x + targetDir.y * targetDir.y);
        Vector3D targetVelocity = {0.0f, 0.0f, 0.0f}; // Сбрасываем целевую скорость
        // Менеджмент Выносливости / Энергетических Очков Действия экзокостюма
        bool isMoving = (len > 0.01f);                                                             // Проверяем, зажата ли вообще хоть одна клавиша движения
        bool canSprint = (m_CurrentStamina > 0.0f) && isMoving && (playerMode == UnitMode::Scout); // Условия бега: есть стамина + движемся + мы Пилот
        bool activeSprint = input.isSprinting && canSprint;                                        // Проверяем, зажал ли игрок Shift при выполнении всех условий
        if (activeSprint)
        {
            // Тратим стамину костюма при беге. Круглые скобки вокруг (std::max) защищают от поломок макросов windows.h
            m_CurrentStamina = (std::max)(0.0f, m_CurrentStamina - m_StaminaDrain * deltaTime);
        }
        else
        {
            // Восстанавливаем стамину до максимума, если игрок идет шагом или стоит на месте
            m_CurrentStamina = (std::min)(m_MaxStamina, m_CurrentStamina + m_StaminaRegen * deltaTime);
        }
        // МЕХАНИКА 4: Логика прыжка-нырка (DIVE) на землю в стиле Helldivers 2
        if (playerMode == UnitMode::Scout)
        {
            // Если прожал Пробел, мы еще не в прыжке и персонаж физически движется
            if (input.isDiving && !m_IsDiving && isMoving)
            {
                m_IsDiving = true;                     // Включаем триггер состояния нырка
                m_DiveTimer = 0.6f;                    // Задаем физическую длительность полета тела (0.6 секунды)
                m_DiveDirection.x = targetDir.x / len; // Нормализуем направление X (избавляемся от ускорения по диагонали)
                m_DiveDirection.y = targetDir.y / len; // Нормализуем направление Y
                m_DiveDirection.z = 0.0f;
            }
            if (m_IsDiving)
            {
                m_DiveTimer -= deltaTime; // Уменьшаем время полета
                if (m_DiveTimer <= 0.0f)
                {
                    m_IsDiving = false; // Приземляемся, выключаем режим нырка
                }
                // Физический импульс выталкивания тела вперед при прыжке лежа (скорость 9.5 единиц)
                float nextX = playerPos.x + m_DiveDirection.x * 9.5f * deltaTime;
                float nextY = playerPos.y + m_DiveDirection.y * 9.5f * deltaTime;
                // Проверяем коллизии со стенами Бункера (AABB-клиппинг сетки) по осям X и Y отдельно
                if (!CheckWorldCollision(nextX, playerPos.y, playerRadius))
                    playerPos.x = nextX;
                if (!CheckWorldCollision(playerPos.x, nextY, playerRadius))
                    playerPos.y = nextY;
                return; // ВАЖНО: Во время полета стандартный WASD-контроль полностью блокируется, прерываем выполнение метода
            }
        }
        // МЕХАНИКА 5: Плавный разгон, скольжение и тушение массы тела (Убираем "деревянность" движения)
        if (isMoving)
        {
            // Выбираем базовую скорость в зависимости от того, бежит персонаж (SprintSpeed) или идет шагом (WalkSpeed)
            float currentMoveSpeed = activeSprint ? m_SprintSpeed : m_WalkSpeed;
            // Если Пилот находится в режиме прицеливания через оптику карабина, его скорость штрафуется (63% от базовой)
            if (playerMode == UnitMode::Scout && isAiming)
            {
                currentMoveSpeed *= 0.63f;
            }
            // Штраф скорости Титана: если гусеничные/моторные системы повреждены коррозией эфира ниже 40%, скорость падает на 70%
            if (playerMode == UnitMode::Titan && titan.systems.tracksCondition < 40.0f)
            {
                currentMoveSpeed *= 0.3f;
            }
            // Вычисляем финальный вектор желаемой целевой скорости
            targetVelocity.x = (targetDir.x / len) * currentMoveSpeed;
            targetVelocity.y = (targetDir.y / len) * currentMoveSpeed;
        }
        // Линейно сглаживаем переход: если игрок разгоняется — берем m_Acceleration, если тормозит — m_Deceleration
        float currentLerp = isMoving ? m_Acceleration : m_Deceleration;
        m_Velocity.x += (targetVelocity.x - m_Velocity.x) * currentLerp * deltaTime;
        m_Velocity.y += (targetVelocity.y - m_Velocity.y) * currentLerp * deltaTime;
        // Рассчитываем координаты следующего шага персонажа с учетом инерции скольжения
        float nextX = playerPos.x + m_Velocity.x * deltaTime;
        float nextY = playerPos.y + m_Velocity.y * deltaTime;
        // Скользящий обсчет коллизий со стенами Убежища 17 (позволяет персонажу гладко «тереться» о стены при движении по диагонали)
        if (!CheckWorldCollision(nextX, playerPos.y, playerRadius))
            playerPos.x = nextX;
        if (!CheckWorldCollision(playerPos.x, nextY, playerRadius))
            playerPos.y = nextY;
        // МЕХАНИКА 6: Трекинг курсора мыши и расчет угла взгляда Пилота в изометрическом мире
        // Обратной проекцией переводим пиксели экрана в физические трехмерные координаты земли
        mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);
        // Вычисляем угол поворота торса игрока относительно позиции курсора мыши
        m_FacingAngle = CalculateFacingAngle(playerPos, mouseWorldPos);
    }
    // Вспомогательный метод расчета угла взгляда через тригонометрический арктангенс
    float PlayerController::CalculateFacingAngle(const Vector3D &pPos, const Vector3D &mPos)
    {
        float dx = mPos.x - pPos.x; // Дельта расстояния по оси X между игроком и мышью
        float dy = mPos.y - pPos.y; // Дельта расстояния по оси Y между игроком и мышью
        // Зона мертвой точки (мертвая зона): если мышь находится прямо на игроке, сохраняем прошлый угол обзора
        if (std::abs(dx) < 0.05f && std::abs(dy) < 0.05f)
        {
            return m_FacingAngle;
        }
        // Вычисляем угол в радианах через std::atan2 и сразу переводим его в классический градусный формат (0 - 360)
        float angle = std::atan2(dy, dx) * 180.0f / 3.14159265f;
        if (angle < 0.0f)
        {
            angle += 360.0f; // Переводим отрицательный результат арктангенса в круговой формат
        }
        return angle; // Возвращаем итоговый угол для матрицы отрисовки спрайта Пилота
    }
} // namespace bunker