#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string>

// Подтягиваем типы данных, перечисления и классы
#include "Tactics.hpp"

// Forward declarations системных функций
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CheckWorldCollision(float nextX, float nextY, float radius);
void UpdateHelldiversCamera(float deltaTime);
ScreenPoint PixelsToNDC(float x, float y, float width, float height);
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments);

// ОБЪЯВЛЕНИЕ ГЛОБАЛЬНЫХ ПЕРЕМЕННЫХ (extern для линковщика)
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
extern Vector3D towerPosition;
extern float playerErosionLevel;
extern bool isAiming;

// УКАЗАТЕЛИ DIRECTX 11
extern ID3D11Device*           g_pd3dDevice;
extern ID3D11DeviceContext*    g_pd3dDeviceContext;
extern IDXGISwapChain*         g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern ID3D11VertexShader*     g_pVertexShader;
extern ID3D11PixelShader*      g_pPixelShader;
extern ID3D11InputLayout*      g_pInputLayout;
extern ID3D11Buffer*           g_pVertexBuffer;

// КАРТА СЕКТОРОВ И ИНЖЕНЕРНЫЕ ПОДСИСТЕМЫ УБЕЖИЩА 17
extern int currentSectorMap[MAP_WIDTH][MAP_HEIGHT];
extern int wallDurability[MAP_WIDTH][MAP_HEIGHT];
extern float etherErosionMap[MAP_WIDTH][MAP_HEIGHT];
