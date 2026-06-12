#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h> // Для компиляции шейдеров "на лету"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// 1. МАТЕМАТИЧЕСКИЕ СТРУКТУРЫ
struct Vector3D { float x = 0.0f; float y = 0.0f; float z = 0.0f; };
struct ScreenPoint { float x = 0.0f; float y = 0.0f; };
enum class UnitMode { Scout, Titan };

struct Bullet {
    Vector3D start; Vector3D current; Vector3D direction;
    float speed = 28.0f; float distanceTraveled = 0.0f; float maxDistance = 16.0f; bool isAlive = true;
};

struct Enemy {
    Vector3D position; float health = 35.0f; float speed = 2.4f; bool isAlive = true; float radius = 0.35f;
};

// Простейшая структура вершины для нашего DirectX 11 рендерера
struct Vertex {
    float x, y, z;
    float r, g, b, a;
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

// ГЛОБАЛЬНЫЙ СТЭЙТ ИГРЫ
UnitMode playerMode = UnitMode::Scout;
Vector3D playerPos = { 0.0f, 0.0f, 0.0f };
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

// ============================================================================
// ВСТАВКА 1: КАРТА И КОЛЛИЗИИ
// ============================================================================
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

// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ DIRECTX 11
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Объекты отрисовки геометрии
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pInputLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;

// Шейдеры, зашитые прямо в код в виде строк (Никаких внешних файлов!)
const char* vertexShaderSrc = R"(
struct VS_INPUT {
    float3 pos : POSITION;
    float4 col : COLOR;
};
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
};
)";

const char* pixelShaderSrc = R"(
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
float4 main(PS_INPUT input) : SV_TARGET {
    return input.col;
};
)";

// Функции нормализации окна DX11
void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Win32 обработчик событий окна
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == 'E') {
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

// Перевод экранных пикселей в нормализованные координаты DX11 (-1.0 до 1.0)
ScreenPoint PixelsToNDC(float x, float y, float width, float height) {
    return { (x / width) * 2.0f - 1.0f, (1.0f - (y / height) * 2.0f) };
}

// Генерация примитивов в буфер вершин
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments = 12) {
    for (int i = 0; i < segments; ++i) {
        float a1 = (i / (float)segments) * 6.28318f;
        float a2 = ((i + 1) / (float)segments) * 6.28318f;
        ScreenPoint p0 = PixelsToNDC(cx, cy, width, height);
        ScreenPoint p1 = PixelsToNDC(cx + std::cos(a1) * r, cy + std::sin(a1) * r, width, height);
        ScreenPoint p2 = PixelsToNDC(cx + std::cos(a2) * r, cy + std::sin(a2) * r, width, height);
        buffer.push_back({p0.x, p0.y, 0.0f, col.r, col.g, col.b, col.a});
        buffer.push_back({p1.x, p1.y, 0.0f, col.r, col.g, col.b, col.a});
        buffer.push_back({p2.x, p2.y, 0.0f, col.r, col.g, col.b, col.a});
    }
}

void HandleInput(HWND hwnd, float deltaTime, float wWidth, float wHeight) {
    Vector3D moveDir = { 0.0f, 0.0f, 0.0f };
    
    // Считываем Win32 асинхронный ввод клавиш WASD
    if (GetAsyncKeyState('W') & 0x8000) { moveDir.x += 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('S') & 0x8000) { moveDir.x -= 1.0f; moveDir.y += 1.0f; }
    if (GetAsyncKeyState('A') & 0x8000) { moveDir.x -= 1.0f; moveDir.y -= 1.0f; }
    if (GetAsyncKeyState('D') & 0x8000) { moveDir.x += 1.0f; moveDir.y += 1.0f; }

    // Расчет вектора перемещения с учетом коллизий (Стены Убежища 17)
    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.0f) {
        float nextX = playerPos.x + (moveDir.x / len) * playerSpeed * deltaTime;
        float nextY = playerPos.y + (moveDir.y / len) * playerSpeed * deltaTime;
        
        // Задаем радиус хитбокса из лора: Скаут — 0.25м, Мех BT-72 — 0.55м
        float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

        // Поочередная проверка осей позволяет плавно скользить вдоль стен, а не залипать в углах
        if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
        if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;
    }

    // Захват координат курсора мыши и расчет 3D точки прицеливания на земле
    POINT mp; 
    GetCursorPos(&mp); 
    ScreenToClient(hwnd, &mp);
    currentMouseScreenPos = { (float)mp.x, (float)mp.y };
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    // Логика кулдауна ведения огня
    if (fireCooldown > 0.0f) {
        fireCooldown -= deltaTime;
    }

    // Стрельба очередью на ЛКМ (Вектор пули летит точно от игрока к 3D-прицелу на земле)
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
            // Скорострельность автоматической пушки BT-72 значительно выше, чем у винтовки Скаута
            fireCooldown = (playerMode == UnitMode::Titan) ? 0.08f : 0.25f;
        }
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"PureIsoDX11", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(L"PureIsoDX11", L"Grimdark Engine v1.0 - Pure DirectX 11", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext);
    CreateRenderTarget();

    // КОМПИЛЯЦИЯ ШЕЙДЕРОВ В СТРОКАХ КОДА
    ID3DBlob* vsBlob = nullptr; ID3DBlob* psBlob = nullptr;
    D3DCompile(vertexShaderSrc, strlen(vertexShaderSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(pixelShaderSrc, strlen(pixelShaderSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, nullptr);

    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pInputLayout);
    vsBlob->Release(); psBlob->Release();

    // Создаем динамический вершинный буфер под геометрию (Максимум 30 000 вершин на кадр)
    D3D11_BUFFER_DESC bd = {}; bd.Usage = D3D11_USAGE_DYNAMIC; bd.ByteWidth = sizeof(Vertex) * 30000;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);

    ShowWindow(hwnd, nCmdShow); UpdateWindow(hwnd);

    ShowWindow(hwnd, nCmdShow); UpdateWindow(hwnd);

    LARGE_INTEGER frequency, t1, t2; 
    QueryPerformanceFrequency(&frequency); 
    QueryPerformanceCounter(&t1);
    
    bool running = true; 
    MSG msg;

    // ГЛАВНЫЙ ИГРОВОЙ ЦИКЛ РЕАЛЬНОГО ВРЕМЕНИ
    while (running) {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); 
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        // Расчет дельты времени через QueryPerformanceCounter
        QueryPerformanceCounter(&t2);
        float deltaTime = static_cast<float>(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
        t1 = t2; 
        
        // Предотвращаем резкие скачки дельты времени при фризах окна
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // Обновляем текущие размеры клиентской области окна Win32
        RECT rect; 
        GetClientRect(hwnd, &rect);
        float width = static_cast<float>(rect.right - rect.left);
        float height = static_cast<float>(rect.bottom - rect.top);
        isoCamera.centerOffset = { width / 2.0f, height / 2.0f };

        // ПОТОК ВВОДА И ОБНОВЛЕНИЯ ПОЗИЦИЙ СИСТЕМ
        HandleInput(hwnd, deltaTime, width, height);

        // Плавная интерполяция (Lerp) камеры за игроком в изометрии
        cameraTarget.x += (playerPos.x - cameraTarget.x) * 5.5f * deltaTime;
        cameraTarget.y += (playerPos.y - cameraTarget.y) * 5.5f * deltaTime;

        // Таймер генерации бесконечного роя жуков
        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= 1.2f) {
            Enemy e; 
            float angle = (rand() % 360) * 0.0174533f;
            // Дистанция 11 метров выносит спавн строго за пределы видимости вьюпорта
            e.position.x = playerPos.x + std::cos(angle) * 11.0f;
            e.position.y = playerPos.y + std::sin(angle) * 11.0f;
            enemies.push_back(e); 
            enemySpawnTimer = 0.0f;
        }

        // Обновление физики летящих пуль и сканирование хитбоксов врагов
        for (auto& b : bullets) {
            float step = b.speed * deltaTime; 
            b.current.x += b.direction.x * step; 
            b.current.y += b.direction.y * step; 
            b.distanceTraveled += step;
            
            if (b.distanceTraveled >= b.maxDistance) b.isAlive = false;
            
            // Если пуля влетела в бетонный завал — уничтожаем её
            if (CheckWorldCollision(b.current.x, b.current.y, 0.05f)) {
                b.isAlive = false;
                continue;
            }


            for (auto& e : enemies) {
                if (!e.isAlive) continue;
                float dx = e.position.x - b.current.x; 
                float dy = e.position.y - b.current.y;
                
                if (std::sqrt(dx * dx + dy * dy) <= e.radius) {
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
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.isAlive; }), bullets.end());

        // Обновление логики ИИ врагов и нанесение контактного урона
        for (auto& e : enemies) {
            if (!e.isAlive) continue;
            float vx = playerPos.x - e.position.x; 
            float vy = playerPos.y - e.position.y; 
            float dist = std::sqrt(vx * vx + vy * vy);
            
            if (dist > 0.35f) {
                e.position.x += (vx / dist) * e.speed * deltaTime; 
                e.position.y += (vy / dist) * e.speed * deltaTime;
            } else {
                // Прямой укус — наносит урон каждую секунду нахождения жука вплотную
                playerHealth -= (playerMode == UnitMode::Titan) ? 8.0f * deltaTime : 25.0f * deltaTime;
                if (playerHealth < 0.0f) playerHealth = 0.0f;
            }
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.isAlive; }), enemies.end());

        // --- ГЕНЕРАЦИЯ ГЕОМЕТРИИ ВЕРШИН (НАШ СОБСТВЕННЫЙ РЕНДЕР-СПИСОК) ---
        std::vector<Vertex> vBuffer;

        // 1. Отрисовка тактической координатной сетки пола
        for (int x = -11; x <= 11; ++x) {
                   // 1. Отрисовка карты секторов Убежища 17 (Стены и проходы)
             for (int x = 0; x < MAP_WIDTH; ++x) {
                for (int y = 0; y < MAP_HEIGHT; ++y) {
                    Vector3D blockPos = { (float)x, (float)y, 0.0f };
                    ScreenPoint sp = isoCamera.WorldToScreen(blockPos, cameraTarget);

                    if (currentSectorMap[x][y] == 1) {
                        // Стены — массивные серые плиты (ромбы из 4 сегментов)
                        PushCircle(vBuffer, sp.x, sp.y, 16.0f, width, height, {0.32f, 0.35f, 0.38f, 1.0f}, 4);
                    } else {
                        // Пол — мелкие тусклые маркеры проходов
                        PushCircle(vBuffer, sp.x, sp.y, 1.5f, width, height, {0.15f, 0.18f, 0.2f, 1.0f}, 4);
                    }
                }
            }
        }

        // 2. Отрисовка роя противников (Красные маркеры опасности)
        for (const auto& e : enemies) {
            if (!e.isAlive) continue;
            ScreenPoint sp = isoCamera.WorldToScreen(e.position, cameraTarget);
            PushCircle(vBuffer, sp.x, sp.y, 7.5f, width, height, {0.92f, 0.25f, 0.25f, 1.0f}, 8);
        }

        // 3. Отрисовка лазерных трассеров (Желтые точки снарядов)
        for (const auto& b : bullets) {
            if (!b.isAlive) continue;
            ScreenPoint sp = isoCamera.WorldToScreen(b.current, cameraTarget);
            PushCircle(vBuffer, sp.x, sp.y, 2.5f, width, height, {1.0f, 0.95f, 0.5f, 1.0f}, 4);
        }

        // 4. Отрисовка тактического прицела мыши (Стиль Arknights: Endfield)
        ScreenPoint spMouse = isoCamera.WorldToScreen(mouseWorldPos, cameraTarget);
        PushCircle(vBuffer, spMouse.x, spMouse.y, 11.0f, width, height, {1.0f, 0.7f, 0.0f, 0.5f}, 8);
        PushCircle(vBuffer, spMouse.x, spMouse.y, 3.0f, width, height, {1.0f, 0.7f, 0.0f, 1.0f}, 4);

        // 5. Отрисовка маркера игрока (Скаут-пехотинец или тяжелый Титан)
        ScreenPoint sPlayer = isoCamera.WorldToScreen(playerPos, cameraTarget);
        if (playerMode == UnitMode::Scout) {
            PushCircle(vBuffer, sPlayer.x, sPlayer.y, 9.0f, width, height, {0.18f, 0.85f, 0.4f, 1.0f}, 8);
        } else {
            // Мех BT-72 рендерится как массивный шестиугольный контур
            PushCircle(vBuffer, sPlayer.x, sPlayer.y, 16.0f, width, height, {0.02f, 0.5f, 0.95f, 1.0f}, 6);
        }

        // 6. КАСТОМНЫЙ ИНДУСТРИАЛЬНЫЙ HUD СРЕДСТВАМИ DX11 (Полоска целостности шасси)
        float barWidth = 280.0f; 
        float barHeight = 14.0f;
        float bx = 45.0f; 
        float by = height - 45.0f;
        float healthRatio = playerHealth / playerMaxHealth;

        // Отрисовка подложки бара (Мрачный темно-серый цвет)
        ScreenPoint b0 = PixelsToNDC(bx, by, width, height);
        ScreenPoint b1 = PixelsToNDC(bx + barWidth, by + barHeight, width, height);
        vBuffer.push_back({b0.x, b0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({b1.x, b0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({b0.x, b1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({b1.x, b0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({b1.x, b1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});
        vBuffer.push_back({b0.x, b1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f});

        // Отрисовка активной полосы биометрии (Зеленая для Скаута, Синяя для ОБЧР)
        ScreenPoint h1 = PixelsToNDC(bx + (barWidth * healthRatio), by + barHeight, width, height);
        Vertex hCol = (playerMode == UnitMode::Titan) ? Vertex{0.0f, 0.52f, 0.95f, 1.0f} : Vertex{0.18f, 0.85f, 0.4f, 1.0f};
        if (healthRatio > 0.0f) {
            vBuffer.push_back({b0.x, b0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({h1.x, b0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({b0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({h1.x, b0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({h1.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
            vBuffer.push_back({b0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a});
        }

        // ЗАГРУЗКА И СИНХРОНИЗАЦИЯ СГЕНЕРИРОВАННОЙ ГЕОМЕТРИИ В ВИДЕОПАМЯТЬ
        D3D11_MAPPED_SUBRESOURCE ms;
        g_pd3dDeviceContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, vBuffer.data(), sizeof(Vertex) * vBuffer.size());
        g_pd3dDeviceContext->Unmap(g_pVertexBuffer, 0);

        // ФАЗА РЕНДЕРИНГА DIRECTX 11
        const float clear_color[] = { 0.05f, 0.05f, 0.06f, 1.0f }; // Глубокий темный индустриальный фон
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

    // ДЕИНИЦИАЛИЗАЦИЯ И ОЧИСТКА ВСЕХ СИСТЕМНЫХ КОМПОНЕНТОВ
    if (g_pVertexBuffer) g_pVertexBuffer->Release(); 
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release(); 
    if (g_pPixelShader) g_pPixelShader->Release();
    
    CleanupRenderTarget();
    if (g_pSwapChain) g_pSwapChain->Release(); 
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release(); 
    if (g_pd3dDevice) g_pd3dDevice->Release();

    DestroyWindow(hwnd); 
    UnregisterClassW(L"PureIsoDX11", hInstance);
    return 0;
}