#pragma once                          // Защита от повторного включения этого файла компилятором во время сборки
#include <vector>                     // Подключаем стандартный динамический массив std::vector для хранения списков
#include <string>                     // Подключаем работу с текстовыми строками std::string для путей к файлам моделей
#include <windows.h>                  // Подключаем API Windows, чтобы компилятор узнал системный тип дескриптора окна (HWND)
#include "../Data/Types.hpp"          // Подключаем главный глобальный хаб типов (там лежат Vector3D, LootContainer и т.д.)
#include "../../Player/Inventory.hpp" // Подключаем инвентарь, чтобы система лута могла выдавать предметы игроку

namespace bunker
{ // Входим в главное пространство имен игры

    // Все структуры данных (CampObjectType, CampPreview, ObjModelMesh, LootContainer)
    // теперь автоматически читаются компилятором из единого центра Types.hpp.
    // Это на 100% защищает движок от критической ошибки переопределения типов C2011!
    class LootSystem
    { // Объявляем класс системы лута и строительной мастерской
    private:
        std::vector<LootContainer> m_Containers; // Динамический массив (список) всех заспавненных сундуков в текущей сессии
        float m_ContainerSpawnTimer = 0.0f;      // Внутренний таймер для отсчета времени между генерацией новых сундуков
        bool m_CampModeActive = false;           // Флаг: включен ли сейчас режим строительства лагеря C.A.M.P. (true/false)
        CampPreview m_CurrentPreview;            // Объект текущего строительного силуэта (хранит координаты сетки и тип постройки)
        bool m_vReleased = true;                 // Триггер для фиксации отжатия клавиши 'V' (чтобы режим строительства не мигал)
        bool m_mReleased = true;                 // Триггер для фиксации отжатия клавиши 'M' (для безопасного переключения интерфейса)
        // --- БЛОК 3D КЭША: Хранилище полигонов для .obj моделей из папки LootAndThings ---
        ObjModelMesh m_ModelConcreteWall;  // Полигональная сетка (меш) для железобетонной защитной стены
        ObjModelMesh m_ModelSupplyCrate;   // Полигональная сетка для кастомного ящика снабжения базы
        ObjModelMesh m_ModelDefenseTurret; // Полигональная сетка для автоматической защитной турели реле связи

    public:
        LootSystem() = default;  // Конструктор по умолчанию (вызывается при создании экземпляра системы)
        ~LootSystem() = default; // Деструктор по умолчанию (безопасно очищает систему при выходе из игры)
        // Высокопроизводительный построчный парсер Wavefront .obj файлов (загружает 3D-модели с диска в память)
        bool LoadObjGeometry(const std::string &filePath, ObjModelMesh &meshOut);
        // Первичный импорт всей папки LootAndThings при старте игровой сессии
        void Initialize3DModelsRegistry();
        // Динамический игровой цикл спавна сундуков на свободных тайлах карты
        void UpdateLootSpawning(float deltaTime);
        // Процедурный рандомайзер содержимого сундука на основе таблицы весов редкости предметов
        void GenerateRandomLoot(LootContainer &containerOut);
        // Механика взаимодействия Пилота с сундуком (по нажатию клавиши 'E') с сетевой синхронизацией
        void ProcessContainerInteraction(Vector3D &pPos);
        // Сетевой хэндлер: принудительное вскрытие сундука на клиентах по сигналу из сети (для мультиплеера)
        void RemoteOpenCrate(float worldX, float worldY);
        // Модернизированный процессор Мастерской C.A.M.P. в стиле Fallout 4/76 и Garry's Mod
        void ProcessDeveloperTools(HWND hwnd, const Vector3D &pilotPos);
        // Отрисовка полупрозрачного 3D-силуэта будущей постройки на основе загруженного .obj меша
        void DrawCampPreviewGhost(std::vector<Vertex> &vBuffer);
        // Универсальный метод проецирования и отрисовки 3D моделей в изометрический игровой мир
        void Render3DModelAt(std::vector<Vertex> &vBuffer, const ObjModelMesh &mesh, const Vector3D &worldPos, float r, float g, float b, float alpha);
        // Функция генерации чит-предметов и каноничного оружия разработчика в рюкзак игрока
        void GiveDeveloperKit(PlayerInventory &playerInvOut);
        // Сброс и полная очистка динамических списков сундуков при смене или перезагрузке ячейки карты
        void ClearContainers() { m_Containers.clear(); }
        // Возвращает ссылку на текущий массив сундуков для внешних систем (например, для рендерера)
        std::vector<LootContainer> &GetContainers() { return m_Containers; }
    };
    extern LootSystem g_LootSystem; // Объявляем внешнюю глобальную шину данных системы лута, доступную для main.cpp
} // namespace bunker // Закрываем пространство имен bunker
