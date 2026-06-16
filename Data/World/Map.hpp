#pragma once
#include <string>
#include <vector>
#include "../Types.hpp" // Точный путь к Types.hpp из вашей иерархии папок

namespace bunker
{

    struct SessionMapMetaData
    {
        std::string currentMapName = "base";
        bool isBaseCleared = false;         // Зачищена ли локация в этой сессии (State of Decay style)
        float baseSuppliesLevel = 100.0f;   // Ресурсы базы/убежища
        unsigned int activeVerminNests = 0; // Количество активных гнезд Роя
    };

    class SessionWorldManager
    {
    private:
        SessionMapMetaData m_SessionMeta;
        const std::string m_DefaultWldPath = "Data/World/Maps/base.wld";

    public:
        SessionWorldManager() = default;

        // 1. Инициализация и чтение живой карты сессии из бинарного .wld файла
        bool LoadWorldSessionFromWld(const std::string &filePath = "");

        // 2. Экспорт текущих разрушений и состояния экосистемы обратно в .wld (автосохранение сессии)
        bool SaveWorldSessionToWld(const std::string &filePath = "");

        // 3. Ежекадровый тик живого мира (разрастание Эрозии, симуляция осады базы Роем)
        void UpdateLiveWorldGrid(float deltaTime);

        // 4. Процедурный спавн стартового окружения и манекенов .aut при первой генерации сессии
        void PopulateInitialTrainingZone();

        // Геттеры состояния сессии
        const SessionMapMetaData &GetSessionMeta() const { return m_SessionMeta; }
        void SetBaseClearedStatus(bool cleared) { m_SessionMeta.isBaseCleared = cleared; }
    };

} // namespace bunker
