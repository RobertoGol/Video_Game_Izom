#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// Автоматическая линковка системных библиотек Windows для DirectX 11
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

enum class UnitMode { 
    Scout, // Оперативник-пехотинец из Убежища 17
    Titan  // Тяжелый штурмовой мех BT-72
};

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

// ============================================================================
// 3. ИЗОМЕТРИЧЕСКАЯ КАМЕРА
// ============================================================================
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

// ============================================================================
// 4. ГЛОБАЛЬНЫЙ СТЭЙТ И КАРТА СЕКТОРОВ УБЕЖИЩА 17
// ============================================================================
UnitMode playerMode = UnitMode::Scout;
Vector3D playerPos = { 2.0f, 2.0f, 0.0f }; // Стартуем внутри прохода
float playerHealth = 100.0f;
float playerMaxHealth = 100.0f;
float playerSpeed = 5.5f;
float fireCooldown = 0.0f;

Vector3D cameraTarget = { 0.0f, 0.0f, 0.0f };
IsometricCamera isoCamera;
Vector3D mouseWorldPos;
ScreenPoint currentMouseScreenPos;

std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
float enemySpawnTimer = 0.0f;
int score = 0;

const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;

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

// ============================================================================
// 5. ГЛОБАЛЬНЫЕ ОБЪЕКТЫ DIRECTX 11
// ============================================================================
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pInputLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;

// Встроенные строки HLSL шейдеров
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

// ============================================================================
// 6. СИСТЕМНЫЕ ФУНКЦИИ ЛОГИКИ И КОЛЛИЗИЙ
// ============================================================================
bool CheckWorldCollision(float nextX, float nextY, float radius) {
    int checkPoints[4][2] = {
        { (int)(nextX - radius), (int)(nextY - radius) },
        { (int)(nextX + radius), (int)(nextY - radius) },
        { (int)(nextX - radius), (int)(nextY + radius) },
        { (int)(nextX + radius), (int)(nextY + radius) }
    };
    for (int i = 0; i < 4; ++i) {
        int tx = checkPoints[i][0];
        int ty = checkPoints[i][1];
        if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) return true;
        if (currentSectorMap[tx][ty] == 1) return true;
    }
    return false;
}

void UpdateHelldiversCamera(float deltaTime) {
    float targetX = playerPos.x + (mouseWorldPos.x - playerPos.x) * 0.3f;
    float targetY = playerPos.y + (mouseWorldPos.y - playerPos.y) * 0.3f;
    float maxDist = 4.5f;
    float dx = targetX - playerPos.x;
    float dy = targetY - playerPos.y;
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
void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
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
            if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == 'E') {
                // Из лора репозитория: синхронизация со стейтом меха BT-72 или Скаута
                if (playerMode == UnitMode::Scout) {
                    playerMode = UnitMode::Titan; playerMaxHealth = 600.0f; playerHealth = 600.0f; playerSpeed = 3.2f;
                } else {
                    playerMode = UnitMode::Scout; playerMaxHealth = 100.0f; playerHealth = 100.0f; playerSpeed = 6.5f;
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

    // Аппаратная инициализация графического конвейера
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext);
    CreateRenderTarget();

    // Компиляция встроенного шейдерного кода HLSL "на лету"
    ID3DBlob* vsBlob = nullptr; 
    ID3DBlob* psBlob = nullptr;
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

    // Выделение динамического буфера под геометрию реального времени (До 30 000 вершин на кадр)
    D3D11_BUFFER_DESC bd = {}; 
    bd.Usage = D3D11_USAGE_DYNAMIC; 
    bd.ByteWidth = sizeof(Vertex) * 30000;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; 
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);

