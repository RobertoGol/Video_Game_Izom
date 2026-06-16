#include "NetManager.hpp"
#include <iostream>

// Принудительная линковка системной сетевой библиотеки Windows сокетов через код
#pragma comment(lib, "Ws2_32.lib")

namespace bunker
{

    NetManager g_NetManager;

    NetManager::NetManager()
    {
        // Генерация случайного ID сессии для локального Пилота
        m_LocalAccountID = rand() % 90000 + 10000;

        for (int i = 0; i < MAX_NET_PLAYERS; ++i)
        {
            m_RemotePlayers[i].isConnected = false;
        }
    }

    NetManager::~NetManager() { ShutdownNetwork(); }

    bool NetManager::StartServer(unsigned short port)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return false;

        m_UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_UdpSocket == INVALID_SOCKET)
        {
            WSACleanup();
            return false;
        }

        // Переводим сокет в неблокирующий режим (Non-blocking I/O), чтобы игра не зависала при ожидании пакетов
        u_long mode = 1;
        ioctlsocket(m_UdpSocket, FIONBIO, &mode);

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(m_UdpSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            ShutdownNetwork();
            return false;
        }

        m_IsServer = true;
        m_IsActive = true;
        std::cout << "[NET] Выделенный сервер сессии успешно запущен на порту: " << port << std::endl;
        return true;
    }

    bool NetManager::ConnectToServer(const std::string &ipAddress, unsigned short port)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return false;

        m_UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_UdpSocket == INVALID_SOCKET)
        {
            WSACleanup();
            return false;
        }

        u_long mode = 1;
        ioctlsocket(m_UdpSocket, FIONBIO, &mode);

        m_IsServer = false;
        m_IsActive = true;
        std::cout << "[NET] Сетевой клиент инициализирован. Подключение к хосту..." << std::endl;
        return true;
    }

    void NetManager::SendLocalState(const Vector3D &pos, float angle, float hp, bunker::UnitMode mode)
    {
        if (!m_IsActive || m_UdpSocket == INVALID_SOCKET)
            return;

        PlayerStatePacket packet;
        packet.accountID = m_LocalAccountID;
        packet.position = pos;
        packet.facingAngle = angle;
        packet.currentHealth = hp;
        packet.mode = mode;

        // В рамках прототипа шлем широковещательный пакет (Broadcast) локальной подсети
        sockaddr_in targetAddr{};
        targetAddr.sin_family = AF_INET;
        targetAddr.sin_port = htons(NET_DEFAULT_PORT);
        targetAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        sendto(m_UdpSocket, reinterpret_cast<const char *>(&packet), sizeof(PlayerStatePacket), 0,
               (sockaddr *)&targetAddr, sizeof(targetAddr));
    }

    void NetManager::PollIncomingPackets()
    {
        if (!m_IsActive || m_UdpSocket == INVALID_SOCKET)
            return;

        PlayerStatePacket buffer;
        sockaddr_in fromAddr{};
        int fromLen = sizeof(fromAddr);

        // Считываем входящие UDP дейтаграммы до тех пор, пока буфер сокета не опустеет
        while (true)
        {
            int bytesRead = recvfrom(m_UdpSocket, reinterpret_cast<char *>(&buffer), sizeof(PlayerStatePacket), 0,
                                     (sockaddr *)&fromAddr, &fromLen);

            if (bytesRead == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK)
                    break; // Данные в сетевой карте закончились, выходим
                break;
            }

            if (bytesRead == sizeof(PlayerStatePacket) && buffer.type == NetPacketType::PlayerState)
            {
                if (buffer.accountID == m_LocalAccountID)
                    continue; // Игнорируем свои же эхо-пакеты

                // Ищем игрока в реестре репликации или регистрируем нового
                int targetIndex = -1;
                for (int i = 0; i < MAX_NET_PLAYERS; ++i)
                {
                    if (m_RemotePlayers[i].isConnected && m_RemotePlayers[i].accountID == buffer.accountID)
                    {
                        targetIndex = i;
                        break;
                    }
                }

                if (targetIndex == -1) // Если игрок подключился впервые
                {
                    for (int i = 0; i < MAX_NET_PLAYERS; ++i)
                    {
                        if (!m_RemotePlayers[i].isConnected)
                        {
                            targetIndex = i;
                            m_RemotePlayers[i].accountID = buffer.accountID;
                            m_RemotePlayers[i].isConnected = true;
                            std::cout << "[NET] Обнаружен новый союзный Пилот ID: " << buffer.accountID << " вошел в сессию." << std::endl;
                            break;
                        }
                    }
                }

                // Записываем обновленные сетевые координаты в ОЗУ репликации
                if (targetIndex != -1)
                {
                    m_RemotePlayers[targetIndex].position = buffer.position;
                    m_RemotePlayers[targetIndex].facingAngle = buffer.facingAngle;
                    m_RemotePlayers[targetIndex].health = buffer.currentHealth;
                    m_RemotePlayers[targetIndex].mode = buffer.mode;
                }
            }
        }
    }

    void NetManager::ShutdownNetwork()
    {
        if (m_UdpSocket != INVALID_SOCKET)
        {
            closesocket(m_UdpSocket);
            m_UdpSocket = INVALID_SOCKET;
        }
        if (m_IsActive)
        {
            WSACleanup();
            m_IsActive = false;
        }
    }

} // namespace bunker
