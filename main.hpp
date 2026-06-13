#pragma once

#ifndef NOMINMAX
#define NOMINMAX // Защита от конфликта макросов min/max в Windows.h
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>

// ПОДКЛЮЧЕНИЕ ВСЕХ ОБЛЕГЧЕННЫХ ПОДСИСТЕМ СТРОГО ПО ПУТЯМ ВАШЕГО ПРОЕКТА
#include "Data/Types.hpp"                   // Файл Types лежит прямо в папке Data
#include "Data/Camera.hpp"                  // Камера лежит прямо в Data, без подпапок
#include "Data/Renderer.hpp"                // Рендерер лежит в Data
#include "Data/RetroShader.hpp"             // Шейдер лежит в Data
#include "Data/PlayerController.hpp"        // Контроллер лежит в Data

// ТOЧЕЧНЫЙ ФИКС: Явно объявляем типы внутри bunker, если Types.hpp забаговался
namespace bunker {
    enum class UnitMode : int;
    struct Vector3D;
    struct ScreenPoint;    
    struct Vault17GridState;
    struct Vault17Progression;
}

#include "Player/Player.hpp"                // ПРАВИЛЬНО: Папка Player лежит в корне
#include "Player/Inventory.hpp"             // Исправлено: убираем Data/
#include "Player/SaveSystem.hpp"            // Исправлено: SaveSystem лежит в Player

#include "Data/Physics/Tactics.hpp"         
#include "Data/Physics/Tank.hpp"            
#include "Data/Physics/Collisions.hpp"      
#include "Data/Physics/Mechanics.hpp"       
#include "Data/Physics/Grid.hpp"            

#include "Data/Physics/Technik/SteamCar.hpp"
#include "Data/Physics/Technik/Motorcycle.hpp"
#include "Data/Physics/Technik/VehicleManager.hpp"

#include "Data/World/Map.hpp"               // Исправлено: Убираем лишнюю папку Maps/
#include "Data/bodies/Enemies.hpp"          
#include "Data/HUD/HUD.hpp"                 
#include "Data/HUD/UIEffects.hpp" 

// Forward declaration системного обработчика оконных сообщений Win32
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ И ШИНЫ ДАННЫХ (extern связывает модули через линкер)
extern bunker::UnitMode playerMode;
extern bunker::Vector3D playerPos;
extern float playerHealth;
extern float playerMaxHealth;
extern float playerSpeed;
extern float fireCooldown;
extern float playerErosionLevel;
extern int score;

// ТOЧЕЧНЫЙ ФИКС: Добавлены префиксы bunker:: для сопоставления типов линкером
extern bunker::Vector3D cameraTarget;
extern bunker::IsometricCamera isoCamera;
extern bunker::Vector3D mouseWorldPos;
extern bunker::ScreenPoint currentMouseScreenPos;

extern std::vector<Bullet> bullets;
extern std::vector<Enemy> enemies;
extern float enemySpawnTimer;

extern bunker::Vault17ClassManager gameClasses;
extern bunker::Vault17GridState regionalGrid;
extern bunker::VanguardTitan titanSimulation; // Наш ИИ БТ-7274
extern TitanAlly titan;
extern bunker::Vector3D towerPosition;
extern bool isAiming;
extern float baseZoom;
extern bunker::Vault17Progression bunkerProgression;

// СИСТЕМНЫЕ УКАЗАТЕЛИ ГРАФИЧЕСКОГО КОНВЕЙЕРА DIRECTX 11
extern ID3D11Device*           g_pd3dDevice;
extern ID3D11DeviceContext*    g_pd3dDeviceContext;
extern IDXGISwapChain*         g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern ID3D11VertexShader*     g_pVertexShader;
extern ID3D11PixelShader*      g_pPixelShader;
extern ID3D11InputLayout*      g_pInputLayout;
extern ID3D11Buffer*           g_pVertexBuffer;

// СЕТКА СЕКТОРОВ И ЭКОСИСТЕМЫ УБЕЖИЩА 17
extern int currentSectorMap[MAP_WIDTH][MAP_HEIGHT];
extern int wallDurability[MAP_WIDTH][MAP_HEIGHT];
extern float etherErosionMap[MAP_WIDTH][MAP_HEIGHT];

// ИНВЕНТАРЬ ИГРОКА
extern bunker::PlayerInventory g_PlayerInventory;
