#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cmath>
#include <algorithm>

// Подключаем наш тактический заголовок
#include "Tactics.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ============================================================================
// СИСТЕМНЫЕ СИГНАТУРЫ И ФОРВАРД-ДЕКЛАРАЦИИ
// ============================================================================
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CheckWorldCollision(float nextX, float nextY, float radius);
void UpdateHelldiversCamera(float deltaTime);
void HandleInput(HWND hwnd, float deltaTime, float wWidth, float wHeight);
void CreateRenderTarget();
void CleanupRenderTarget();
ScreenPoint PixelsToNDC(float x, float y, float width, float height);
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments);

// ============================================================================
// ГЛОБАЛЬНЫЙ СТЭЙТ ИГРЫ (ОБЪЯВЛЕНИЕ ПЕРЕМЕННЫХ)
// ============================================================================
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

// Экземпляры внешних лорных модулей
extern Vault17ClassManager gameClasses;
extern Vault17GridState regionalGrid;
extern Vector3D towerPosition;
extern float playerErosionLevel;
extern bool isAiming;

// Контекст аппаратного пайплайна DirectX 11
extern ID3D11Device*           g_pd3dDevice;
extern ID3D11DeviceContext*    g_pd3dDeviceContext;
extern IDXGISwapChain*         g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern ID3D11VertexShader*     g_pVertexShader;
extern ID3D11PixelShader*      g_pPixelShader;
extern ID3D11InputLayout*      g_pInputLayout;
extern ID3D11Buffer*           g_pVertexBuffer;

// Карты текущего сектора
extern int currentSectorMap[MAP_WIDTH][MAP_HEIGHT];
extern int wallDurability[MAP_WIDTH][MAP_HEIGHT];
extern float etherErosionMap[MAP_WIDTH][MAP_HEIGHT];

// Строковые исходники шейдеров HLSL
extern const char* vertexShaderSrc;
extern const char* pixelShaderSrc;
