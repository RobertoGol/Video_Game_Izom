#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h> // Полностью закрывает ошибку D3DCompile
#include <vector>
#include <string>
#include <algorithm>

// ИСПРАВЛЕНО: Подключение строго по путям твоих вложенных папок
#include "Data/Types.hpp"
#include "Data/Physics/Tactics.hpp"
#include "Data/Map.cpp" // Для доступа к константам разметки, если требуется
#include "Data/bodies/Enemies.hpp"
#include "Data/Physics/Mechanics.hpp"
#include "Data/Player/Player.hpp"
#include "Data/HUD/HUD.hpp"
#include "Data/HUD/UIEffects.hpp"

// Forward declarations системных функций для оконного цикла
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateHelldiversCamera(float deltaTime);

// ОБЪЯВЛЕНИЕ ГЛОБАЛЬНЫХ ПЕРЕМЕННЫХ (extern связывает модули через линкер)
extern UnitMode playerMode;
extern Vector3D playerPos;
extern float playerHealth;
extern float playerMaxHealth;
extern float playerSpeed;
extern float fireCooldown;

extern Vector3D cameraTarget;
extern IsometricCamera isoCamera;
extern Vector3D mouseWorldPos;
extern ScreenPoint currentMouseScreenPos;

extern std::vector<Bullet> bullets;
extern std::vector<Enemy> enemies;
extern float enemySpawnTimer;
extern int score;

extern Vault17ClassManager gameClasses;
extern Vault17GridState regionalGrid;
extern TitanAlly titan;
extern Vector3D towerPosition;
extern float playerErosionLevel;
extern bool isAiming;
extern float baseZoom;

// DIRECTX 11 СИСТЕМНЫЕ УКАЗАТЕЛИ ДВИЖКА
extern ID3D11Device*           g_pd3dDevice;
extern ID3D11DeviceContext*    g_pd3dDeviceContext;
extern IDXGISwapChain*         g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern ID3D11VertexShader*     g_pVertexShader;
extern ID3D11PixelShader*      g_pPixelShader;
extern ID3D11InputLayout*      g_pInputLayout;
extern ID3D11Buffer*           g_pVertexBuffer;

// СЕТКА КАРТЫ СЕКТОРОВ УБЕЖИЩА 17
extern int currentSectorMap[MAP_WIDTH][MAP_HEIGHT];
extern int wallDurability[MAP_WIDTH][MAP_HEIGHT];
extern float etherErosionMap[MAP_WIDTH][MAP_HEIGHT];

// Глобальные аппаратные хелперы рендера геометрии
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments = 12);
ScreenPoint PixelsToNDC(float x, float y, float width, float height);
void CreateRenderTarget();
void CleanupRenderTarget();
