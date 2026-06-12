#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ============================================================================
// 1. ОПЕРЕЖАЮЩИЕ ОБЪЯВЛЕНИЯ (ФИКС ОШИБКИ ИДЕНТИФИКАТОРОВ)
// ============================================================================
// --- ОБЯЗАТЕЛЬНЫЕ ПРЕДВАРИТЕЛЬНЫЕ ОБЪЯВЛЕНИЯ ФУНКЦИЙ ---
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CheckWorldCollision(float nextX, float nextY, float radius);
void UpdateHelldiversCamera(float deltaTime);
// ============================================================================
// 2. МАТЕМАТИЧЕСКИЕ И ИГРОВЫЕ СТРУКТУРЫ
// ============================================================================
struct Vector3D { 
    float x = 0.0f; 
    float y = 0.0f; 
    float z = 0.0f; 
};

struct ScreenPoint { 
    float x = 0.0f; 
    float y = 0.0f; 
};

enum class UnitMode { Scout, Titan };
enum class AIState { Follow, Guard, Combat };

struct Bullet {
    Vector3D start; 
    Vector3D current; 
    Vector3D direction;
    float speed = 28.0f; 
    float distanceTraveled = 0.0f; 
    float maxDistance = 16.0f; 
    bool isAlive = true;
};

struct Enemy {
    Vector3D position; 
    float health = 35.0f; 
    float speed = 2.4f; 
    bool isAlive = true; 
    float radius = 0.35f; // Радиус хитбокса Вермина (в метрах) - Вермины
};

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

// Компоненты Меха BT-72 из оригинального лора реставрации
struct TitanComponents {
    float coreEnergy = 100.0f;       
    float chassisIntegrity = 100.0f; 
    float turretStatus = 100.0f;     
    float sensorLink = 100.0f;       
};

struct TitanAlly {
    Vector3D position = { 2.0f, 2.0f, 0.0f };
    float health = 600.0f;
    float maxHealth = 600.0f;
    float speed = 3.2f;
    float fireCooldown = 0.0f;
    bool isPiloted = false;               
    AIState aiState = AIState::Follow;    
    TitanComponents systems;              
};

// Состояние региональной сети из файлов нормализации (NormalizeWorldFieldState)
struct Vault17GridState {
    bool towerSyncRecovered = true;   
    bool localRelayAvailable = true;  
    bool feyRingGateUnlocked = true;  
    float towerHealth = 200.0f;       
};

// Счетчики пробуждения пассивных перков из оригинального файла SkillSystem.cpp
struct SkillAwakening {
    int footKills = 0;                
    int archiveSyncs = 0;             
    int tankActions = 0;              
    bool fieldReflexAwakened = false;
    bool pilotSyncAwakened = false;
};

// 2. ИЗОМЕТРИЧЕСКАЯ КАМЕРА
class IsometricCamera {
public:
    const float ISO_COS = 0.8660254f;
    const float ISO_SIN = 0.5000000f;
    float zoom = 55.0f; 
    ScreenPoint centerOffset = { 640.0f, 360.0f };

    ScreenPoint WorldToScreen(const Vector3D& worldPos, const Vector3D& cameraTarget) const {
        float relX = worldPos.x - cameraTarget.x;
        float relY = worldPos.y - cameraTarget.y;
        float relZ = worldPos.z - cameraTarget.z;
        ScreenPoint screen;
        screen.x = (relX - relY) * ISO_COS;
        screen.y = (relX + relY) * ISO_SIN - relZ;
        screen.x = (screen.x * zoom) + centerOffset.x;
        screen.y = (screen.y * zoom) + centerOffset.y;
        return screen;
    }

    Vector3D ScreenToWorldGround(const ScreenPoint& screenPos, const Vector3D& cameraTarget) const {
        float sx = (screenPos.x - centerOffset.x) / zoom;
        float sy = (screenPos.y - centerOffset.y) / zoom;
        Vector3D world;
        world.x = (sx / (2.0f * ISO_COS)) + (sy / (2.0f * ISO_SIN)) + cameraTarget.x;
        world.y = (sy / (2.0f * ISO_SIN)) - (sx / (2.0f * ISO_COS)) + cameraTarget.y;
        world.z = 0.0f;
        return world;
    }
};

// ГЛОБАЛЬНЫЙ СТЭЙТ ИГРЫ И ПОДПИСАННЫЕ МАССИВЫ КАРТ
int currentSectorMap[MAP_WIDTH][MAP_HEIGHT] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,1,1,0,1,0,1,1,1,1,1,0,0,1,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
    {1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,1,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,1},
    {1,0,0,1,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0,1},
    {1,0,0,1,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,1,1,1,1,1,1,0,0,1,1,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int wallDurability[MAP_WIDTH][MAP_HEIGHT];
float etherErosionMap[MAP_WIDTH][MAP_HEIGHT];

UnitMode playerMode = UnitMode::Scout;
Vector3D playerPos = { 5.0f, 5.0f, 0.0f };
float playerHealth = 100.0f;
float playerMaxHealth = 100.0f;
float playerSpeed = 5.5f;
float fireCooldown = 0.0f;
float playerErosionLevel = 0.0f;         
const float EROSION_DAMAGE_THRESHOLD = 50.0f; 

Vector3D cameraTarget = { 0.0f, 0.0f, 0.0f };
IsometricCamera isoCamera;
Vector3D mouseWorldPos;
ScreenPoint currentMouseScreenPos;

std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
float enemySpawnTimer = 0.0f;
int score = 0;

TitanAlly titan;
Vault17GridState regionalGrid;
SkillAwakening skillCounters;
Vector3D towerPosition = { 10.0f, 10.0f, 0.0f }; 

// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ DIRECTX 11
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pInputLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;

// HLSL ШЕЙДЕРЫ КОНВЕЙЕРА РЕНДЕРИНГА
const char* vertexShaderSrc = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };
PS_INPUT main(VS_INPUT input) {
    PS_INPUT output; output.pos = float4(input.pos, 1.0f); output.col = input.col; return output;
};
)";

const char* pixelShaderSrc = R"(
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };
float4 main(PS_INPUT input) : SV_TARGET { return input.col; };
)";

// 3. ФУНКЦИИ ЛОГИКИ И ПРОВЕРКИ СТОЛКНОВЕНИЙ
bool CheckWorldCollision(float nextX, float nextY, float radius) {
    int checkPoints[4][2] = {
        { (int)(nextX - radius), (int)(nextY - radius) },
        { (int)(nextX + radius), (int)(nextY - radius) },
        { (int)(nextX - radius), (int)(nextY + radius) },
        { (int)(nextX + radius), (int)(nextY + radius) }
    float checkPoints[4][2] = {
        { nextX - radius, nextY - radius },
        { nextX + radius, nextY - radius },
        { nextX - radius, nextY + radius },
        { nextX + radius, nextY + radius }
    };
    for (int i = 0; i < 4; ++i) {
        int tx = (int)checkPoints[i][0];
        int ty = (int)checkPoints[i][1];
        if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return true;
        if (currentSectorMap[tx][ty] == 1) return true;
    }
    return false;
}

void UpdateHelldiversCamera(float deltaTime) {
    float targetX = playerPos.x + (mouseWorldPos.x - playerPos.x) * 0.3f;
    float targetY = playerPos.y + (mouseWorldPos.y - playerPos.y) * 0.3f;
    float maxDist = 4.5f;
    float dx = targetX - playerPos.x; float dy = targetY - playerPos.y;
    float currentDist = std::sqrt(dx * dx + dy * dy);
    if (currentDist > maxDist) {
        targetX = playerPos.x + (dx / currentDist) * maxDist;
        targetY = playerPos.y + (dy / currentDist) * maxDist;
    }
    cameraTarget.x += (targetX - cameraTarget.x) * 4.5f * deltaTime;
    cameraTarget.y += (targetY - cameraTarget.y) * 4.5f * deltaTime;
}

void HandleInput(HWND hwnd, float deltaTime, float wWidth, float wHeight) {
    Vector3D moveDir = { 0.0f, 0.0f, 0.0f };
    if (GetAsyncKeyState('W') & 0x8000) { moveDir.x += 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { moveDir.x -= 1.0f; moveDir.y += 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { moveDir.x -= 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { moveDir.x += 1.0f; moveDir.y += 1.0f; }

    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.0f) {
        float nextX = playerPos.x + (moveDir.x / len) * playerSpeed * deltaTime;
        float nextY = playerPos.y + (moveDir.y / len) * playerSpeed * deltaTime;
        float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;
        if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
        if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;
    }

    POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
    currentMouseScreenPos = { (float)mp.x, (float)mp.y };
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    if (fireCooldown > 0.0f) fireCooldown -= deltaTime;

    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && fireCooldown <= 0.0f) {
        Bullet b; b.start = playerPos; b.current = playerPos;
        float dx = mouseWorldPos.x - playerPos.x; float dy = mouseWorldPos.y - playerPos.y;
        float dLen = std::sqrt(dx * dx + dy * dy);
        if (dLen > 0.05f) {
            b.direction = { dx / dLen, dy / dLen, 0.0f };
            bullets.push_back(b);
            fireCooldown = (playerMode == UnitMode::Titan) ? 0.08f : 0.25f;
        }
    }
}

// ============================================================================
// 7. СИСТЕМНЫЕ ФУНКЦИИ DIRECTX 11
// ============================================================================
// 4. ОКОННЫЕ СИСТЕМНЫЕ ФУНКЦИИ WIN32 / DIRECTX 11
void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_KEYMENU) return 0; // Блокируем вызов системного меню по ALT
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
                case WM_KEYDOWN:
            if (wParam == 'E') {
                if (!titan.isPiloted) {
                    // Рассчитываем дистанцию между Скаутом и Мехом для инициализации Sync Link
                    float dx = playerPos.x - titan.position.x;
                    float dy = playerPos.y - titan.position.y;
                    float distToTitan = std::sqrt(dx * dx + dy * dy);

                    // Сесть внутрь кабины BT-72 можно только находясь вплотную (радиус 1.8 метра)
                    // Инициализация Sync Link с BT-72 на дистанции до 1.8 метров
                    if (distToTitan <= 1.8f) {
                        titan.isPiloted = true;
                        playerMode = UnitMode::Titan;
                        playerMaxHealth = titan.maxHealth;
                        playerHealth = titan.health;
                        playerSpeed = titan.speed; // Скорость падает под весом шасси
                        
                        // Инкрементируем счетчик синхронизации из SkillSystem.cpp
                        // Сесть внутрь кабины BT-72 можно только находясь вплотную (радиус 1.8 метра)
                        skillCounters.tankActions++;
                    }
                } else {
                    // Высадка пилота обратно на карту секторов
                    // Обратная высадка Скаута на карту секторов
                    titan.isPiloted = false;
                    // Высадка пилота обратно на карту секторов
                    titan.health = playerHealth; // Сохраняем ХП меха при выходе
                    playerMode = UnitMode::Scout;
                    playerMaxHealth = 100.0f;
                    playerHealth = 100.0f; // Восстанавливаем статы Скаута
                    playerSpeed = 6.5f;
                    playerPos.x = titan.position.x + 1.0f;
                    playerPos.y = titan.position.y;
                }
            }
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Вспомогательная функция перевода экранных пикселей в нормализованные координаты DX11 NDC (-1.0 до 1.0)
ScreenPoint PixelsToNDC(float x, float y, float width, float height) {
    return { (x / width) * 2.0f - 1.0f, (1.0f - (y / height) * 2.0f) };
}

// Генерация радиальных примитивов/полигонов в буфер вершин
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments = 12) {
    for (int i = 0; i < segments; ++i) {
        float a1 = (i / (float)segments) * 6.283185f;
        float a2 = ((i + 1) / (float)segments) * 6.283185f;
        ScreenPoint p0 = PixelsToNDC(cx, cy, width, height);
        ScreenPoint p1 = PixelsToNDC(cx + std::cos(a1) * r, cy + std::sin(a1) * r, width, height);
        ScreenPoint p2 = PixelsToNDC(cx + std::cos(a2) * r, cy + std::sin(a2) * r, width, height);
        buffer.push_back({p0.x, p0.y, 0.0f, col.r, col.g, col.b, col.a});
        buffer.push_back({p1.x, p1.y, 0.0f, col.r, col.g, col.b, col.a});
        buffer.push_back({p2.x, p2.y, 0.0f, col.r, col.g, col.b, col.a});
    }
}

// Тактический Win32-ввод в реальном времени с расчетом скольжения вдоль стен секторов
void HandleInput(HWND hwnd, float deltaTime, float wWidth, float wHeight) {
    Vector3D moveDir = { 0.0f, 0.0f, 0.0f };
    if (GetAsyncKeyState('W') & 0x8000) { moveDir.x += 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { moveDir.x -= 1.0f; moveDir.y += 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { moveDir.x -= 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { moveDir.x += 1.0f; moveDir.y += 1.0f; }

    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.0f) {
        float nextX = playerPos.x + (moveDir.x / len) * playerSpeed * deltaTime;
        float nextY = playerPos.y + (moveDir.y / len) * playerSpeed * deltaTime;
        
        // Радиус физической коллизии: Скаут — 0.25м, Мех BT-72 — 0.55м
        float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

        // Поочередная проверка осей гарантирует плавное скольжение без залипания в углах
        if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
        if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;
    }

    // Расчет прицела Pip-Pad через обратную изометрическую проекцию
    POINT mp; 
    GetCursorPos(&mp); 
    ScreenToClient(hwnd, &mp);
    POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
    currentMouseScreenPos = { (float)mp.x, (float)mp.y };
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    if (fireCooldown > 0.0f) fireCooldown -= deltaTime;

    // Ведение реального огня по зажатию ЛКМ Windows
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && fireCooldown <= 0.0f) {
        Bullet b; 
        b.start = playerPos; 
        b.current = playerPos;
        float dx = mouseWorldPos.x - playerPos.x; 
        float dy = mouseWorldPos.y - playerPos.y;
        float dLen = std::sqrt(dx * dx + dy * dy);
        
        Bullet blt; blt.start = playerPos; blt.current = playerPos;
        float dx = mouseWorldPos.x - playerPos.x; float dy = mouseWorldPos.y - playerPos.y;
        float dLen = std::sqrt(dx*dx + dy*dy);
        if (dLen > 0.05f) {
            b.direction = { dx / dLen, dy / dLen, 0.0f };
            bullets.push_back(b);
            // Автоматическая пушка BT-72 стреляет очередью, винтовка Скаута — одиночными
            fireCooldown = (playerMode == UnitMode::Titan) ? 0.08f : 0.25f;
        }
    }
}

// Нативная точка входа приложения Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Регистрация класса окна
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"PureIsoDX11", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(L"PureIsoDX11", L"Vault 17 Outpost - Pure DirectX 11 Engine", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    // Конфигурация SwapChain под DirectX 11
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; 
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; 
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; 
    sd.Windowed = TRUE;
    sd.BufferCount = 1; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE;

    // Аппаратная инициализация графического конвейера
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext);
    CreateRenderTarget();

    // Компиляция встроенного шейдерного кода HLSL "на лету"
    ID3DBlob* vsBlob = nullptr; 
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* vsBlob = nullptr; ID3DBlob* psBlob = nullptr;
    D3DCompile(vertexShaderSrc, strlen(vertexShaderSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(pixelShaderSrc, strlen(pixelShaderSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, nullptr);

    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPixelShader);

    // Описание структуры вершинного формата (Позиция + Цвет)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pInputLayout);
    vsBlob->Release(); 
    psBlob->Release();
    vsBlob->Release(); psBlob->Release();

    // Выделение динамического буфера под геометрию реального времени (До 30 000 вершин на кадр)
    D3D11_BUFFER_DESC bd = {}; 
    bd.Usage = D3D11_USAGE_DYNAMIC; 
    bd.ByteWidth = sizeof(Vertex) * 30000;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; 
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_BUFFER_DESC bd = {}; bd.Usage = D3D11_USAGE_DYNAMIC; bd.ByteWidth = sizeof(Vertex) * 30000;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);

    // ============================================================================
    // ВСТАВКА 2: ЗАПОЛНЕНИЕ КАРТЫ СЕКТОРОВ И АНОМАЛИЙ
    // ============================================================================
    // Инициализация прочности разрушаемых завалов стен и карты эфирной эрозии
    // Заполнение карт секторов, прочности завалов и аномалий эрозии
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
             // Если на карте завал (1), даем ему 100 единиц прочности
                wallDurability[x][y] = 100;
                etherErosionMap[x][y] = 0.0f;
            } else {
                wallDurability[x][y] = 0;
                // Генерируем случайные пятна эфирной эрозии в проходах бункера
                // Развертывание аномальных зон в проходах коридоров бункера
                etherErosionMap[x][y] = ((x % 5 == 0 && y % 4 == 0) ? 0.75f : 0.0f);
            }
        }
    }
    // Зачищаем стартовую комнату от завалов, чтобы Скаут и Мех не заспавнились в стене
    currentSectorMap[5][5] = 0; wallDurability[5][5] = 0;
    currentSectorMap[6][5] = 0; wallDurability[6][5] = 0;
    playerPos = { 5.0f, 5.0f, 0.0f };
    titan.position = { 6.0f, 5.0f, 0.0f };

        ShowWindow(hwnd, nCmdShow); 
    UpdateWindow(hwnd);

    ShowWindow(hwnd, nCmdShow); 
    UpdateWindow(hwnd);

    LARGE_INTEGER frequency, t1, t2; 
    QueryPerformanceFrequency(&frequency); 
    QueryPerformanceCounter(&t1);
    
    bool running = true; 
    MSG msg;

    while (running) {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        QueryPerformanceCounter(&t2);
        float deltaTime = static_cast<float>(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
        t1 = t2; if (deltaTime > 0.1f) deltaTime = 0.1f;

        RECT rect; GetClientRect(hwnd, &rect);
        float width = static_cast<float>(rect.right - rect.left);
        float height = static_cast<float>(rect.bottom - rect.top);
        isoCamera.centerOffset = { width / 2.0f, height / 2.0f };

        HandleInput(hwnd, deltaTime, width, height);
        UpdateHelldiversCamera(deltaTime);

        // Генерация роя Верминов (Vermin) за картой
        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= 1.2f) {
            Enemy e; float angle = (rand() % 360) * 0.0174533f;
            e.position.x = playerPos.x + std::cos(angle) * 11.0f;
            e.position.y = playerPos.y + std::sin(angle) * 11.0f;
            enemies.push_back(e); enemySpawnTimer = 0.0f;
        }

        // Обновление баллистики лазерных снарядов и расчистка завалов
        for (auto& b : bullets) {
            float step = b.speed * deltaTime; b.current.x += b.direction.x * step; b.current.y += b.direction.y * step; b.distanceTraveled += step;
            if (b.distanceTraveled >= b.maxDistance) b.isAlive = false;

            int mapX = (int)b.current.x; int mapY = (int)b.current.y;
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                if (currentSectorMap[mapX][mapY] == 1) {
                    b.isAlive = false;
                    int damageToWall = (playerMode == UnitMode::Titan || titan.isPiloted) ? 25 : 8;
                    wallDurability[mapX][mapY] -= damageToWall;
                    if (wallDurability[mapX][mapY] <= 0) { currentSectorMap[mapX][mapY] = 0; score += 50; }
                    continue;
                }
            }
            
            for (auto& e : enemies) {
                if (!e.isAlive) continue;
                float dx = e.position.x - b.current.x; float dy = e.position.y - b.current.y;
                if (std::sqrt(dx*dx + dy*dy) <= e.radius) {
                     e.health -= (playerMode == UnitMode::Titan) ? 30.0f : 16.0f; 
                    b.isAlive = false;
                    if (e.health <= 0.0f) { 
                        e.isAlive = false; 
                        score += 150; 
                    }
                    break;
                }
            }
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& bulletObj) { return !bulletObj.isAlive; }), bullets.end());

        // Обновление логики ИИ врагов (Вермины), атака игрока или Вышки связи Убежища 17
        for (auto& e : enemies) {
            if (!e.isAlive) continue;
            
            float vx = playerPos.x - e.position.x; 
            float vy = playerPos.y - e.position.y; 
            float dist = std::sqrt(vx * vx + vy * vy);

            // Рассчитываем дистанцию Вермина до вышки связи региона
            float tdx = towerPosition.x - e.position.x;
            float tdy = towerPosition.y - e.position.y;
            float distToTower = std::sqrt(tdx * tdx + tdy * tdy);

            // Агро-логика Верминов из лора: если они близко к вышке, они грызут её, иначе преследуют Скаута
            if (distToTower < 4.0f && regionalGrid.towerSyncRecovered) {
                if (distToTower > 0.4f) {
                    e.position.x += (tdx / distToTower) * e.speed * deltaTime;
                    e.position.y += (tdy / distToTower) * e.speed * deltaTime;
                } else {
                    // Нанесение урона инфраструктуре Убежища 17
                    regionalGrid.towerHealth -= 15.0f * deltaTime;
                    
                    // Каскадная нормализация состояний (NormalizeWorldFieldState)
                    if (regionalGrid.towerHealth <= 0.0f) {
                        regionalGrid.towerHealth = 0.0f;
                        regionalGrid.towerSyncRecovered = false;  // Вышка уничтожена!
                        regionalGrid.localRelayAvailable = false; // Локальное реле Pip-Pad падает
                        regionalGrid.feyRingGateUnlocked = false; // Врата эвакуации FeyRing блокируются
                    }
                }
            } else {
                // Преследование пилота или Меха
                if (dist > 0.35f) {
                    e.position.x += (vx / dist) * e.speed * deltaTime; 
                    e.position.y += (vy / dist) * e.speed * deltaTime;
                } else {
                    // Контактный укус
                    playerHealth -= (playerMode == UnitMode::Titan) ? 8.0f * deltaTime : 25.0f * deltaTime;
                    if (playerHealth < 0.0f) playerHealth = 0.0f;
                }
            }
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& enemyObj) { return !enemyObj.isAlive; }), enemies.end());

        // --------------------------------------------------------------------
        // ПОТОК ЛОГИКИ АВТОНОМНОГО СОЮЗНИКА BT-72 И СКАНИРОВАНИЯ PIP-PAD
        // --------------------------------------------------------------------
        if (!titan.isPiloted) {
            float tdx = playerPos.x - titan.position.x;
            float tdy = playerPos.y - titan.position.y;
            float distToPlayer = std::sqrt(tdx * tdx + tdy * tdy);

            // ИИ Меха выбирает приоритетную цель — ближайшего к нему Вермина (Vermin)
            Enemy* closestTarget = nullptr;
            float closestDist = 8.0f;

            for (auto& e : enemies) {
                if (!e.isAlive) continue;
                float edx = e.position.x - titan.position.x;
                float edy = e.position.y - titan.position.y;
                float eDist = std::sqrt(edx * edx + edy * edy);
                if (eDist < closestDist) {
                    closestDist = eDist;
                    closestTarget = &e;
                }
            }

            // Ведение прикрывающего огня автопушкой ИИ по Вермину
            if (closestTarget != nullptr) {
                if (titan.fireCooldown <= 0.0f) {
                    Bullet tb;
                    tb.start = titan.position;
                    tb.current = titan.position;
                    float bdx = closestTarget->position.x - titan.position.x;
                    float bdy = closestTarget->position.y - titan.position.y;
                    float bLen = std::sqrt(bdx * bdx + bdy * bdy);
                    if (bLen > 0.1f) {
                        tb.direction = { bdx / bLen, bdy / bLen, 0.0f };
                        bullets.push_back(tb);
                        titan.fireCooldown = 0.12f;
                    }
                }
            }

            if (titan.fireCooldown > 0.0f) titan.fireCooldown -= deltaTime;

            // ИИ-следование за Скаутом по коридорам Убежища 17
            if (distToPlayer > 2.2f) {
                float nextX = titan.position.x + (tdx / distToPlayer) * titan.speed * deltaTime;
                float nextY = titan.position.y + (tdy / distToPlayer) * titan.speed * deltaTime;
                if (!CheckWorldCollision(nextX, titan.position.y, 0.55f)) titan.position.x = nextX;
                if (!CheckWorldCollision(titan.position.x, nextY, 0.55f)) titan.position.y = nextY;
            }
        } else {
            titan.position = playerPos;
        }

        // РАСЧЕТ НАКОПЛЕНИЯ ЭФИРНОЙ ЭРОЗИИ (ETHER EROSION) СКАУТА
        int currentTileX = (int)playerPos.x;
        int currentTileY = (int)playerPos.y;

        if (currentTileX >= 0 && currentTileX < MAP_WIDTH && currentTileY >= 0 && currentTileY < MAP_HEIGHT) {
            float currentZoneErosion = etherErosionMap[currentTileX][currentTileY];
            
            if (titan.isPiloted) {
                // Защита гермокабины BT-72 экранирует пилота от внешнего излучения
                playerErosionLevel -= 12.0f * deltaTime;
            } else {
                if (currentZoneErosion > 0.0f) {
                    playerErosionLevel += currentZoneErosion * 18.0f * deltaTime;
                } else {
                    playerErosionLevel -= 4.0f * deltaTime;
                }
            }
            
            if (playerErosionLevel > 100.0f) playerErosionLevel = 100.0f;
            if (playerErosionLevel < 0.0f) playerErosionLevel = 0.0f;

            if (playerErosionLevel > EROSION_DAMAGE_THRESHOLD && !titan.isPiloted) {
                playerHealth -= 10.0f * deltaTime; 
                if (playerHealth < 0.0f) playerHealth = 0.0f;
            }
        }

        // --- ГЕНЕРАЦИЯ ГЕОМЕТРИИ ВЕРШИН (НАШ СОБСТВЕННЫЙ РЕНДЕР-СПИСОК) ---
        std::vector<Vertex> vBuffer;

        // 1. Отрисовка карты секторов Убежища 17 (Стены и проходы)
        for (int x = 0; x < MAP_WIDTH; ++x) {
            for (int y = 0; y < MAP_HEIGHT; ++y) {
                Vector3D blockPos = { (float)x, (float)y, 0.0f };
                ScreenPoint sp = isoCamera.WorldToScreen(blockPos, cameraTarget);

                if (currentSectorMap[x][y] == 1) {
                    // Разрушаемые завалы: Прочность бетона влияет на цвет плиты
                    float damageFactor = wallDurability[x][y] / 100.0f;
                    PushCircle(vBuffer, sp.x, sp.y, 16.0f, width, height, {0.32f * damageFactor, 0.35f * damageFactor, 0.38f * damageFactor, 1.0f}, 4);
                } else {
                    // Отображение эфирных аномалий пола
                    if (etherErosionMap[x][y] > 0.0f) {
                        PushCircle(vBuffer, sp.x, sp.y, 2.0f, width, height, {0.8f, 0.55f, 0.1f, 0.8f}, 4);
                    } else {
                        PushCircle(vBuffer, sp.x, sp.y, 1.5f, width, height, {0.15f, 0.18f, 0.2f, 1.0f}, 4);
                    }
                }
            }
        }

        // 2. Отрисовка роя противников (Вермины — Красные маркеры)
        for (const auto& e : enemies) {
            if (!e.isAlive) continue;
            ScreenPoint sp = isoCamera.WorldToScreen(e.position, cameraTarget);
            PushCircle(vBuffer, sp.x, sp.y, 7.5f, width, height, {0.92f, 0.25f, 0.25f, 1.0f}, 8);
        }

        // 3. Отрисовка лазерных трассеров пуль
        for (const auto& b : bullets) {
            if (!b.isAlive) continue;
            ScreenPoint sp = isoCamera.WorldToScreen(b.current, cameraTarget);
            PushCircle(vBuffer, sp.x, sp.y, 2.5f, width, height, {1.0f, 0.95f, 0.5f, 1.0f}, 4);
        }

        // 4. Отрисовка тактического прицела мыши Pip-Pad
        ScreenPoint spMouse = isoCamera.WorldToScreen(mouseWorldPos, cameraTarget);
        PushCircle(vBuffer, spMouse.x, spMouse.y, 11.0f, width, height, {1.0f, 0.7f, 0.0f, 0.5f}, 8);
        PushCircle(vBuffer, spMouse.x, spMouse.y, 3.0f, width, height, {1.0f, 0.7f, 0.0f, 1.0f}, 4);

        // 5. Отрисовка ИИ-напарника BT-72 (Если игрок снаружи пешком)
        if (!titan.isPiloted) {
            ScreenPoint sTitan = isoCamera.WorldToScreen(titan.position, cameraTarget);
            PushCircle(vBuffer, sTitan.x, sTitan.y, 15.0f, width, height, {0.05f, 0.45f, 0.85f, 1.0f}, 6);
        }

        // 6. Отрисовка маркера игрока (Скаут или ведомый Мех в режиме пилотирования)
        ScreenPoint sPlayer = isoCamera.WorldToScreen(playerPos, cameraTarget);
        if (playerMode == UnitMode::Scout) {
            PushCircle(vBuffer, sPlayer.x, sPlayer.y, 9.0f, width, height, {0.18f, 0.85f, 0.4f, 1.0f}, 8);
        } else {
            PushCircle(vBuffer, sPlayer.x, sPlayer.y, 17.0f, width, height, {0.02f, 0.52f, 0.98f, 1.0f}, 6);
        }

        // 7. Отрисовка Вышки связи региона (Vault 17 Relay Tower)
        ScreenPoint spTower = isoCamera.WorldToScreen(towerPosition, cameraTarget);
        if (regionalGrid.towerSyncRecovered) {
            PushCircle(vBuffer, spTower.x, spTower.y, 14.0f, width, height, {0.0f, 0.6f, 1.0f, 1.0f}, 3);
        } else {
            PushCircle(vBuffer, spTower.x, spTower.y, 14.0f, width, height, {0.25f, 0.2f, 0.2f, 1.0f}, 3);
        }

        // 8. НАШ КАСТОМНЫЙ ВОЕННЫЙ HUD СРЕДСТВАМИ DX11 (Отрисовка шкал биометрии)
        float barWidth = 280.0f; 
        float barHeight = 12.0f;
        
        // Шкала Здоровья (В нижнем левом углу)
        float bx1 = 45.0f; 
        float by1 = height - 45.0f;
        float healthRatio = playerHealth / playerMaxHealth;

        ScreenPoint hpBg0 = PixelsToNDC(bx1, by1, width, height);
        ScreenPoint hpBg1 = PixelsToNDC(bx1 + barWidth, by1 + barHeight, width, height);
        vBuffer.push_back({hpBg0.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({hpBg1.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({hpBg0.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({hpBg1.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({hpBg1.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({hpBg0.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});

        if (healthRatio > 0.0f) {
            ScreenPoint h1 = PixelsToNDC(bx1 + (barWidth * healthRatio), by1 + barHeight, width, height);
            Vertex hCol = (playerMode == UnitMode::Titan) ? Vertex{0.0f, 0.52f, 0.95f, 1.0f} : Vertex{0.18f, 0.85f, 0.4f, 1.0f};
            vBuffer.push_back({hpBg0.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({h1.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({hpBg0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({h1.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({h1.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({hpBg0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
        }

        // Шкала Эфирной Эрозии (В нижнем правом углу)
        float bx2 = width - barWidth - 45.0f;
        float by2 = height - 45.0f;
        float erosionRatio = playerErosionLevel / 100.0f;

        ScreenPoint erBg0 = PixelsToNDC(bx2, by2, width, height);
        ScreenPoint erBg1 = PixelsToNDC(bx2 + barWidth, by2 + barHeight, width, height);
        vBuffer.push_back({erBg0.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({erBg1.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({erBg0.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({erBg1.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({erBg1.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({erBg0.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});

        if (erosionRatio > 0.0f) {
            ScreenPoint e1 = PixelsToNDC(bx2 + (barWidth * erosionRatio), by2 + barHeight, width, height);
            Vertex eCol = {0.85f, 0.5f, 0.05f, 1.0f}; // Предупреждающее оранжевое свечение эфира
            vBuffer.push_back({erBg0.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a});
            vBuffer.push_back({e1.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a});
            vBuffer.push_back({erBg0.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a});
            vBuffer.push_back({e1.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a});
            vBuffer.push_back({e1.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a});
            vBuffer.push_back({erBg0.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a});
        }

        // ЗАГРУЗКА И СИНХРОНИЗАЦИЯ СГЕНЕРИРОВАННОЙ ГЕОМЕТРИИ В ВИДЕОПАМЯТЬ
        D3D11_MAPPED_SUBRESOURCE ms;
        g_pd3dDeviceContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, vBuffer.data(), sizeof(Vertex) * vBuffer.size());
        g_pd3dDeviceContext->Unmap(g_pVertexBuffer, 0);

        // ФАЗА РЕНДЕРИНГА DIRECTX 11
        const float clear_color[] = { 0.05f, 0.05f, 0.06f, 1.0f }; 
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, width, height, 0.0f, 1.0f };
        g_pd3dDeviceContext->RSSetViewports(1, &vp);

        UINT stride = sizeof(Vertex); 
        UINT offset = 0;
        g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
        g_pd3dDeviceContext->IASetInputLayout(g_pInputLayout);
        g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pd3dDeviceContext->VSSetShader(g_pVertexShader, nullptr, 0);
        g_pd3dDeviceContext->PSSetShader(g_pPixelShader, nullptr, 0);
// Аппаратная отрисовка всего сформированного буфера за один вызов
        g_pd3dDeviceContext->Draw(static_cast<UINT>(vBuffer.size()), 0);
        g_pSwapChain->Present(1, 0); // Синхронизация кадров включена (V-Sync)
    }

    // ДЕИНИЦИАЛИЗАЦИЯ И ОЧИСТКА ВСЕХ СИСТЕМНЫХ КОМПОНЕНТОВ DIRECT3D
    if (g_pVertexBuffer) g_pVertexBuffer->Release(); 
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release(); 
    if (g_pPixelShader) g_pPixelShader->Release();
    
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

    DestroyWindow(hwnd); 
    UnregisterClassW(L"PureIsoDX11", hInstance);
    
    return 0;
}


