#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Отсекает старый Winsock 1.0 из состава Windows.h
#endif

#define _WINSOCKAPI_  // Полная блокировка старого заголовочного файла winsock.h
#include <winsock2.h> // Подключаем только современный сетевой стек
#include <windows.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include "../Types.hpp" // ПРАВИЛЬНО: Подключаем Types, чтобы менеджер увидел Vector3D без ошибок переопределения

// Убираем инклуд Types.hpp, чтобы избежать круговой зависимости!
// Вместо этого делаем форвард-декларацию типов вашего движка:
namespace bunker
{
    struct Vector3D
    {
        float x;
        float y;
        float z;
    };
    enum class UnitMode : int;

    const int MAX_NET_PLAYERS = 20;
    const unsigned short NET_DEFAULT_PORT = 17115;

    enum class NetPacketType : unsigned char
    {

        Handshake,       // Подключение к сессии бункера
        PlayerState,     // Координаты, ХП, угол взгляда Пилота
        SpawnProjectile, // Выстрелы пуль/кассетных ракет
        ContainerSync,   // Синхронизация открытия ящиков
        MapModifySync,   // Сетевой клик Toolgun-а
        Disconnect       // Выход игрока
    };

#pragma pack(push, 1)

    struct ContainerSyncPacket
    {
        NetPacketType type = NetPacketType::ContainerSync;
        float posX;
        float posY; // Позиция вскрытого сундука в мире для поиска в массиве
    };

    struct MapModifyPacket
    {
        NetPacketType type = NetPacketType::MapModifySync;
        int tileX;
        int tileY;
        int tileType; // 1 = поставить стену, 0 = стереть
    };

    // Компактная структура сетевого пакета синхронизации движения (25 байт)
    struct PlayerStatePacket
    {
        NetPacketType type = NetPacketType::PlayerState;
        unsigned int accountID;
        Vector3D position;
        float facingAngle;
        float currentHealth;
        bunker::UnitMode mode;
    };

#pragma pack(pop)

    struct RemotePlayer
    {
        unsigned int accountID = 0;
        Vector3D position = {0.0f, 0.0f, 0.0f};
        float facingAngle = 0.0f;
        float health = 100.0f;
        bunker::UnitMode mode = static_cast<bunker::UnitMode>(0);
        bool isConnected = false;
    };

    class NetManager
    {
    private:
        SOCKET m_UdpSocket = INVALID_SOCKET;
        bool m_IsServer = false;
        bool m_IsActive = false;
        unsigned int m_LocalAccountID = 0;

        // Массив удаленных реплицируемых игроков в сессии (до 20 человек)
        RemotePlayer m_RemotePlayers[MAX_NET_PLAYERS];

    public:
        NetManager();
        ~NetManager();

        // Запуск локального Хоста/Сервера сессии Убежища 17
        bool StartServer(unsigned short port = NET_DEFAULT_PORT);

        // Подключение Клиента к удаленному Хосту по IP-адресу
        bool ConnectToServer(const std::string &ipAddress, unsigned short port = NET_DEFAULT_PORT);

        // Мгновенная отправка своего состояния по UDP конвейеру
        void SendLocalState(const Vector3D &pos, float angle, float hp, bunker::UnitMode mode);

        // Ежекадровый неблокирующий сбор входящих пакетов от остальных 19 игроков
        void PollIncomingPackets();

        void ShutdownNetwork();

        // Геттеры состояния сети
        const RemotePlayer *GetRemotePlayers() const { return m_RemotePlayers; }
        bool IsActive() const { return m_IsActive; }
    };

    extern NetManager g_NetManager;

} // namespace bunker
