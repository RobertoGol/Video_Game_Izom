#include <iostream>
#include <cmath>
#include <algorithm>

// Подключаем наш заголовочный файл глобальных систем
#include "main.hpp"


// ИНИЦИАЛИЗАЦИЯ ВСЕХ ГЛОБАЛЬНЫХ ПЕРЕМЕННЫХ
// ФИЗИЧЕСКОЕ ВЫДЕЛЕНИЕ ПАМЯТИ ПОД ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
UnitMode playerMode = UnitMode::Scout;
Vector3D playerPos = { 5.0f, 5.0f, 0.0f };
float playerHealth = 100.0f;
float playerMaxHealth = 100.0f;
float playerSpeed = 5.5f;
float fireCooldown = 0.0f;

Vector3D cameraTarget = { 5.0f, 5.0f, 0.0f };
IsometricCamera isoCamera;
Vector3D mouseWorldPos;
ScreenPoint currentMouseScreenPos;

std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
float enemySpawnTimer = 0.0f;
int score = 0;

Vault17ClassManager gameClasses;
Vault17GridState regionalGrid;
Vector3D towerPosition = { 10.0f, 10.0f, 0.0f };
float playerErosionLevel = 0.0f;
bool isAiming = false;

ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pInputLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;

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

// Строковые исходники шейдеров HLSL встроенные в движок
const char* vertexShaderSrc = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };
struct VS_INPUT {
    float3 pos : POSITION;
    float4 col : COLOR;
};
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
PS_INPUT main(VS_INPUT input) {
    PS_INPUT output; output.pos = float4(input.pos, 1.0f); output.col = input.col; return output;
};)";
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
};
)";

const char* pixelShaderSrc = R"(
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };
float4 main(PS_INPUT input) : SV_TARGET { return input.col; };)";

bool CheckWorldCollision(float nextX, float nextY, float radius) {
    int checkPoints[4][2] = {
        { (int)(nextX - radius), (int)(nextY - radius) },
        { (int)(nextX + radius), (int)(nextY - radius) },
        { (int)(nextX - radius), (int)(nextY + radius) },
        { (int)(nextX + radius), (int)(nextY + radius) }
    };
    for (int i = 0; i < 4; ++i) {
        int tx = checkPoints[i][0]; int ty = checkPoints[i][1];
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

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
float4 main(PS_INPUT input) : SV_TARGET {
    return input.col;
};
)";

// Логика создания_Render Target
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
        case WM_MOUSEWHEEL:
            if (playerMode == UnitMode::Titan && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
                short zDelta = (short)HIWORD(wParam);
                if (zDelta > 0) isoCamera.zoom += 4.0f; else isoCamera.zoom -= 4.0f;
                if (isoCamera.zoom < 30.0f) isoCamera.zoom = 30.0f;
                if (isoCamera.zoom > 90.0f) isoCamera.zoom = 90.0f;
            }
            return 0;
        case WM_KEYDOWN:
            if (wParam == 'E') {
                if (!titan.isPiloted) {
                    float dx = playerPos.x - titan.position.x; float dy = playerPos.y - titan.position.y;
                    float dx = playerPos.x - titan.position.x;
                    float dy = playerPos.y - titan.position.y;
                    if (std::sqrt(dx*dx + dy*dy) <= 1.8f) {
                        titan.isPiloted = true; gameClasses.EnterVehicle();
                        playerMode = UnitMode::Titan; playerMaxHealth = gameClasses.currentStats.maxHealth;
                        playerHealth = titan.health; playerSpeed = gameClasses.currentStats.moveSpeed;
                        titan.isPiloted = true;
                        gameClasses.EnterVehicle();
                        playerMode = UnitMode::Titan;
                        playerMaxHealth = gameClasses.currentStats.maxHealth;
                        playerHealth = titan.health;
                        playerSpeed = gameClasses.currentStats.moveSpeed;
                    }
                } else {
                    titan.isPiloted = false; titan.health = playerHealth; gameClasses.ExitVehicle();
                    playerMode = UnitMode::Scout; playerMaxHealth = gameClasses.currentStats.maxHealth;
                    playerHealth = 100.0f; playerSpeed = gameClasses.currentStats.moveSpeed;
                    titan.isPiloted = false;
                    titan.health = playerHealth;
                    gameClasses.ExitVehicle();
                    playerMode = UnitMode::Scout;
                    playerMaxHealth = gameClasses.currentStats.maxHealth;
                    playerHealth = 100.0f;
                    playerSpeed = gameClasses.currentStats.moveSpeed;
                    playerPos.x = titan.position.x + 1.0f;
                }
            }
        }
            if (wParam == VK_TAB) {
                if (titan.isPiloted) {
                    if (titan.currentWeapon == TankWeaponMode::Cannon) {
                        titan.currentWeapon = TankWeaponMode::AutoCannon;
                    } else {
                        titan.currentWeapon = TankWeaponMode::Cannon;
                    }
                }
            }
        }
            if (wParam == 'Q') {
                gameClasses.ActivateTacticalSkill();
                if (gameClasses.GetActivePilotClass() == PilotClass::Stim && !titan.isPiloted) {
                    playerHealth = std::min(playerMaxHealth, playerHealth + 30.0f);
                }}
            if (wParam == VK_TAB && titan.isPiloted) {
                titan.currentWeapon = (titan.currentWeapon == TankWeaponMode::Cannon) ? TankWeaponMode::AutoCannon : TankWeaponMode::Cannon;
            }
            if (wParam == 'Q') gameClasses.ActivateTacticalSkill();
            if (!titan.isPiloted) {
                if (wParam == '1') gameClasses.ChangePilotClass(PilotClass::Grapple);
                if (wParam == '2') gameClasses.ChangePilotClass(PilotClass::Cloak);
                if (wParam == '3') gameClasses.ChangePilotClass(PilotClass::Stream);
            }
            if (titan.isPiloted) {
                if (wParam == '4') gameClasses.ChangeTitanFirmware(TitanClass::Ion);
                if (wParam == '5') gameClasses.ChangeTitanFirmware(TitanClass::Scorch);
                if (wParam == '6') gameClasses.ChangeTitanFirmware(TitanClass::Northstar);
                if (wParam == '7') gameClasses.ChangeTitanFirmware(TitanClass::Tone);
            } else {
                if (wParam == '1') gameClasses.ChangePilotClass(PilotClass::Grapple);
                if (wParam == '2') gameClasses.ChangePilotClass(PilotClass::Cloak);
                if (wParam == '3') gameClasses.ChangePilotClass(PilotClass::Stim);
            }
            break;
        case WM_MOUSEWHEEL:
            if (playerMode == UnitMode::Titan && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
                short zDelta = (short)HIWORD(wParam);
                if (zDelta > 0) isoCamera.zoom += 4.0f;
                else isoCamera.zoom -= 4.0f;
                if (isoCamera.zoom < 30.0f) isoCamera.zoom = 30.0f;
                if (isoCamera.zoom > 90.0f) isoCamera.zoom = 90.0f;
            }
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

ScreenPoint PixelsToNDC(float x, float y, float width, float height) {
    return { (x / width) * 2.0f - 1.0f, (1.0f - (y / height) * 2.0f) };
}

void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments) {
void PushCircle(std::vector<Vertex>& buffer, float cx, float cy, float r, float width, float height, Vertex col, int segments = 12) {
    for (int i = 0; i < segments; ++i) {
        float a1 = (i / (float)segments) * 6.283185f; float a2 = ((i + 1) / (float)segments) * 6.283185f;
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

    isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000);

    if (playerMode == UnitMode::Scout || !isAiming) {
        isoCamera.zoom += (baseZoom - isoCamera.zoom) * 5.0f * deltaTime;
        isoCamera.zoom += (55.0f - isoCamera.zoom) * 5.0f * deltaTime;
    }

    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.0f) {
        float currentMoveSpeed = playerSpeed;
        if (playerMode == UnitMode::Scout && isAiming) currentMoveSpeed *= 0.63f;
        if (playerMode == UnitMode::Titan && titan.systems.tracksCondition < 40.0f) currentMoveSpeed *= 0.3f;
        if (playerMode == UnitMode::Scout && isAiming) {
            currentMoveSpeed *= 0.63f;
        }
        if (playerMode == UnitMode::Titan && titan.systems.tracksCondition < 40.0f) {
            currentMoveSpeed *= 0.3f;
        }

        float nextX = playerPos.x + (moveDir.x / len) * currentMoveSpeed * deltaTime;
        float nextY = playerPos.y + (moveDir.y / len) * currentMoveSpeed * deltaTime;
        float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

        if (!CheckWorldCollision(nextX, playerPos.y, playerRadius)) playerPos.x = nextX;
        if (!CheckWorldCollision(playerPos.x, nextY, playerRadius)) playerPos.y = nextY;
    }

    POINT mp; GetCursorPos(&mp); ScreenToClient(hwnd, &mp);
    currentMouseScreenPos = { (float)mp.x, (float)mp.y };
    mouseWorldPos = isoCamera.ScreenToWorldGround(currentMouseScreenPos, cameraTarget);

    if (fireCooldown > 0.0f) fireCooldown -= deltaTime;
    // Расчет вектора направления наведения на цель
    float dx = mouseWorldPos.x - playerPos.x;
    float dy = mouseWorldPos.y - playerPos.y;
    float dLen = std::sqrt(dx * dx + dy * dy);

    if (dLen > 0.05f && fireCooldown <= 0.0f) {
        Vector3D normDir = { dx / dLen, dy / dLen, 0.0f };
        // ОГНЕВЫЕ РЕЖИМЫ КЛАССА: СКАУТ-ОПЕРАТИВНИК
        // --------------------------------------------------------------------
        if (playerMode == UnitMode::Scout) {
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (isAiming) {
                    float pelletSpread = 0.04f;
                    for (int i = 0; i < 5; ++i) {
                        Bullet p; p.start = playerPos; p.current = playerPos; p.type = BulletType::Pellet;
                        float randomSpread = ((rand() % 100) / 100.0f - 0.5f) * pelletSpread;
                        p.direction = { normDir.x + randomSpread, normDir.y + randomSpread, 0.0f };
                        bullets.push_back(p);
                    }
                    fireCooldown = 0.55f;
                } else {
                    // Режим Карабина: Одиночный точный выстрел
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; bullets.push_back(b);
                    fireCooldown = 0.22f;
                }
            }
        }
        else if (playerMode == UnitMode::Titan) {
            if (titan.systems.turretStatus < 50.0f) {
                float brokenFactor = ((rand() % 100) / 100.0f - 0.5f) * 0.25f;
                normDir.x += brokenFactor; normDir.y += brokenFactor;
            }

            if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                if (titan.hasMissileModule) {
                    if (titan.missileMode == MissileStrikeMode::Ballistic) {
                        for (int i = -3; i <= 3; ++i) {
                            Bullet m; m.start = playerPos; m.current = playerPos;
                            m.type = BulletType::BallisticMissile; m.speed = 15.0f; m.splashRadius = 1.8f;
                            Vector3D sideVector = { -normDir.y, normDir.x, 0.0f };
                            m.start.x += sideVector.x * (i * 0.4f);
                            m.start.y += sideVector.y * (i * 0.4f);
                            m.direction = normDir;
                            bullets.push_back(m);
                        }
                    } else {
                        // Режим 2: Артиллерийский залп из 8 ракет по навесной траектории вверх
                        for (int i = 0; i < 8; ++i) {
                            Bullet m; m.start = playerPos; m.current = playerPos;
                            m.type = BulletType::ArtilleryMissile; m.speed = 10.0f; m.splashRadius = 2.4f;
                            m.targetPos = mouseWorldPos; 
                            Bullet m; 
                            m.start = playerPos; 
                            m.current = playerPos;
                            m.type = BulletType::ArtilleryMissile; 
                            m.speed = 10.0f; 
                            m.splashRadius = 2.4f;
                            m.targetPos = mouseWorldPos; // Точка падения — позиция курсора мыши
                            // Случайный разброс падения ракет в артиллерийском круге
                            m.targetPos.x += ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                            m.targetPos.y += ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
                            bullets.push_back(m);
                        }
                    }
                    // Переключаем режим на следующий для последующего клика СКМ
                    titan.missileMode = (titan.missileMode == MissileStrikeMode::Ballistic) ? MissileStrikeMode::Artillery : MissileStrikeMode::Ballistic;
                    fireCooldown = 1.8f; // Перезарядка ракетных шахт
                }
            }

// Нативная точка входа приложения Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"PureIsoDX11", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(L"PureIsoDX11", L"Vault 17 Tactical Core - DirectX 11", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; 
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; 
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; 
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext);
    CreateRenderTarget();

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

    D3D11_BUFFER_DESC bd = {}; bd.Usage = D3D11_USAGE_DYNAMIC; bd.ByteWidth = sizeof(Vertex) * 30000;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);

    // Инициализация прочности разрушаемых завалов стен и карты эфирной эрозии
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            if (currentSectorMap[x][y] == 1) {
                wallDurability[x][y] = 100;
                etherErosionMap[x][y] = 0.0f;
            } else {
                wallDurability[x][y] = 0;
                etherErosionMap[x][y] = ((x % 5 == 0 && y % 4 == 0) ? 0.75f : 0.0f);
            }
        }
    }
    
    currentSectorMap[5][5] = 0; wallDurability[5][5] = 0;
    currentSectorMap[6][5] = 0; wallDurability[6][5] = 0;
    playerPos = { 5.0f, 5.0f, 0.0f };
    titan.position = { 6.0f, 5.0f, 0.0f };

    ShowWindow(hwnd, nCmdShow); UpdateWindow(hwnd);

    LARGE_INTEGER frequency, t1, t2; QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&t1);
    bool running = true; MSG msg;

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

        // ТАКТИЧЕСКИЕ ОБНОВЛЕНИЯ КЛАССОВ И ТАЙМЕРОВ КУЛДАУНА СПОСОБНОСТЕЙ
        gameClasses.UpdateCooldowns(deltaTime);
        if (!titan.isPiloted) {
            playerSpeed = gameClasses.currentStats.moveSpeed;
        }

        HandleInput(hwnd, deltaTime, width, height);
        UpdateHelldiversCamera(deltaTime);

        // Таймер генерации Vermin за экраном
        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= 1.2f) {
            Enemy e; float angle = (rand() % 360) * 0.0174533f;
            e.position.x = playerPos.x + std::cos(angle) * 11.0f;
            e.position.y = playerPos.y + std::sin(angle) * 11.0f;
            enemies.push_back(e); enemySpawnTimer = 0.0f;
        }

        // Вычисления пуль и баллистических траекторий
        for (auto& b : bullets) {
            float moveStep = b.speed * deltaTime;
                        if (b.type == BulletType::ArtilleryMissile) {
                float dx = b.targetPos.x - b.current.x;
                float dy = b.targetPos.y - b.current.y;
                float distToTarget = std::sqrt(dx * dx + dy * dy);

                if (distToTarget > 0.2f) {
                    b.current.x += (dx / distToTarget) * moveStep;
                    b.current.y += (dy / distToTarget) * moveStep;
                    b.distanceTraveled += moveStep;
                    
                    // Расчет параболы высоты Z на основе прогресса пройденного пути
                    float totalExpectedDist = 12.0f;
                    float progress = b.distanceTraveled / totalExpectedDist;
                    if (progress > 1.0f) progress = 1.0f;
                    // Пик высоты артиллерийского выстрела — 4 метра вверх по оси Z
                    b.current.z = std::sin(progress * 3.1415926f) * 4.0f; 
                } else {
                    // Достижение целевой точки на земле — детонация по площади
                    b.isAlive = false;
                }
            } else {
                // Прямой настильный полет (Карабин, Дробовик, Баллистические ракеты)
                b.current.x += b.direction.x * moveStep;
                b.current.y += b.direction.y * moveStep;
                b.distanceTraveled += moveStep;
            }

            if (b.distanceTraveled >= b.maxDistance) b.isAlive = false;

            // ИНЖЕНЕРНЫЙ ПРОСЧЕТ РАЗРУШАЕМОСТИ ЗАВАЛОВ СТЕН (DEBRIS DESTRUCTION)
            int mapX = (int)b.current.x; 
            int mapY = (int)b.current.y;
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                if (currentSectorMap[mapX][mapY] == 1) {
                    b.isAlive = false;
                    // Взрыв ракет наносит завалам Убежища тяжелый урон
                    int damageToWall = (b.type == BulletType::BallisticMissile || b.type == BulletType::ArtilleryMissile) ? 50 : 8;
                    wallDurability[mapX][mapY] -= damageToWall;
                    if (wallDurability[mapX][mapY] <= 0) { 
                        currentSectorMap[mapX][mapY] = 0; // Проход расчищен
                        score += 50; 
                    }
                    continue;
                }
            }

            // Сканирование физических хитбоксов Верминов (Vermin) с учетом сплеш-урона
            for (auto& e : enemies) {
                if (!e.isAlive) continue;
                float edx = e.position.x - b.current.x; 
                float edy = e.position.y - b.current.y;
                
                if (std::sqrt(edx * edx + edy * edy) <= e.radius) {
                    b.isAlive = false;

                    if (b.type == BulletType::BallisticMissile || b.type == BulletType::ArtilleryMissile) {
                        // Осколочный взрыв по площади из ТЗ
                        for (auto& splashEnemy : enemies) {
                            if (!splashEnemy.isAlive) continue;
                            float sdx = splashEnemy.position.x - b.current.x;
                            float sdy = splashEnemy.position.y - b.current.y;
                            if (std::sqrt(sdx * sdx + sdy * sdy) <= b.splashRadius) {
                                splashEnemy.health -= 85.0f;
                                if (splashEnemy.health <= 0.0f) { splashEnemy.isAlive = false; score += 150; }
                            }
                        }
                    } else {
                        // Одиночные попадания карабина или дроби
                        e.health -= (b.type == BulletType::Pellet) ? 14.0f : 20.0f;
                        if (e.health <= 0.0f) { e.isAlive = false; score += 150; }
                    }
                    break;
                }
            }
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& bulletObj) { return !bulletObj.isAlive; }), bullets.end());

        // Обновление логики ИИ врагов (Вермины): атака Вышки реле связи или систем игрока
        for (auto& e : enemies) {
            if (!e.isAlive) continue;
            
            float vx = playerPos.x - e.position.x; 
            float vy = playerPos.y - e.position.y; 
            float dist = std::sqrt(vx * vx + vy * vy);

            // Дистанция Вермина до Вышки связи региона
            float tdx = towerPosition.x - e.position.x;
            float tdy = towerPosition.y - e.position.y;
            float distToTower = std::sqrt(tdx * tdx + tdy * tdy);

            // Проверяем тактическую маскировку или фазовый сдвиг Скаута
            bool isPlayerHidden = gameClasses.isTacticalActive && 
                (gameClasses.GetActivePilotClass() == PilotClass::Cloak || 
                 gameClasses.GetActivePilotClass() == PilotClass::PhaseShift);

            // Если пилот скрылся в невидимости/фазе — Вермины теряют его из вида и грызут Вышку связи
            if (isPlayerHidden && !titan.isPiloted) {
                dist = 999.0f; 
            }

            // Агро-логика Верминов: приоритет вышки реле, если они в радиусе 4 метров
            if (distToTower < 4.0f && regionalGrid.towerSyncRecovered) {
                if (distToTower > 0.4f) {
                    e.position.x += (tdx / distToTower) * e.speed * deltaTime;
                    e.position.y += (tdy / distToTower) * e.speed * deltaTime;
                } else {
                    // Вермины наносят урон инфраструктуре Убежища 17
                    regionalGrid.towerHealth -= 15.0f * deltaTime;
                    
                    // Каскадная нормализация состояний (NormalizeWorldFieldState) из лора
                    if (regionalGrid.towerHealth <= 0.0f) {
                        regionalGrid.towerHealth = 0.0f;
                        regionalGrid.towerSyncRecovered = false;  // Вышка связи уничтожена!
                        regionalGrid.localRelayAvailable = false; // Локальное реле Pip-Pad падает
                        regionalGrid.feyRingGateUnlocked = false; // Врата эвакуации FeyRing блокируются
                    }
                }
            } else {
                // Преследование пилота или Танка с ИИ
                if (dist > 0.35f) {
                    e.position.x += (vx / dist) * e.speed * deltaTime;
                    e.position.y += (vy / dist) * e.speed * deltaTime;
                } else {
                    // Распределение повреждений по 4 внутренним узлам Танка
                    if (playerMode == UnitMode::Titan) {
                        int targetComponent = rand() % 4;
                        float damageAmt = 15.0f * deltaTime;
                        
                        if (targetComponent == 0) titan.systems.tracksCondition -= damageAmt; // Поломка ходовой
                        else if (targetComponent == 1) titan.systems.turretStatus -= damageAmt;   // Поломка турели пушки
                        else if (targetComponent == 2) titan.systems.sensorLink -= damageAmt;     // Вывод из строя реле
                        else titan.systems.coreEnergy -= damageAmt;                               // Повреждение ядра
                        
                        // Каскадный пересчет общего здоровья Танка на основе износа узлов
                        playerHealth = (titan.systems.coreEnergy + titan.systems.tracksCondition + titan.systems.turretStatus + titan.systems.sensorLink) * 1.5f;
                        if (titan.systems.coreEnergy <= 0.0f) playerHealth = 0.0f; // Взрыв ядра Танка
                    } else {
                        // Обычный укус Скаута вне брони Танка
                        playerHealth -= 25.0f * deltaTime;
                    }
                    if (playerHealth < 0.0f) playerHealth = 0.0f;
                }
            }
        }
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& enemyObj) { return !enemyObj.isAlive; }), enemies.end());

        // --------------------------------------------------------------------
        // ПОТОК ЛОГИКИ АВТОНОМНОГО СОЮЗНИКА (ТАНК С ИИ) И СКАНИРОВАНИЯ PIP-PAD
        // --------------------------------------------------------------------
        if (!titan.isPiloted) {
            float tdx = playerPos.x - titan.position.x;
            float tdy = playerPos.y - titan.position.y;
            float distToPlayer = std::sqrt(tdx * tdx + tdy * tdy);

            // ИИ Танка ищет ближайшего к нему Вермина
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

            // Ведение автоматического прикрывающего огня Танком по Вермину
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

// ИИ-движение Танка за Скаутом по свободным проходам
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
                // Защита тяжелой брони Танка экранирует пилота от внешнего излучения секторов
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
                    PushCircle(vBuffer, sp.x, sp.y, 16.0f, width, height, { 0.32f * damageFactor, 0.35f * damageFactor, 0.38f * damageFactor, 1.0f }, 4);
                } else {
                    // Отображение эфирных аномалий пола
                    if (etherErosionMap[x][y] > 0.0f) {
                        PushCircle(vBuffer, sp.x, sp.y, 2.0f, width, height, { 0.8f, 0.55f, 0.1f, 0.8f }, 4);
                    } else {
                        PushCircle(vBuffer, sp.x, sp.y, 1.5f, width, height, { 0.15f, 0.18f, 0.2f, 1.0f }, 4);
                    }
                }
            }
        }

        // 2. Отрисовка роя противников (Вермины — Красные маркеры)
        bool isRadarActive = (playerMode == UnitMode::Scout) || (titan.systems.sensorLink >= 30.0f);
        for (const auto& e : enemies) {
            if (!e.isAlive) continue;
            // Враги отображаются на радаре Pip-Pad только если исправно реле связи
            if (isRadarActive || (playerMode == UnitMode::Scout)) {
                ScreenPoint sp = isoCamera.WorldToScreen(e.position, cameraTarget);
                PushCircle(vBuffer, sp.x, sp.y, 7.5f, width, height, { 0.92f, 0.25f, 0.25f, 1.0f }, 8);
            }
        }

        // 3. Отрисовка лазерных трассеров пуль
        for (const auto& b : bullets) {
            if (!b.isAlive) continue;
            ScreenPoint sp = isoCamera.WorldToScreen(b.current, cameraTarget);
            PushCircle(vBuffer, sp.x, sp.y, 2.5f, width, height, { 1.0f, 0.95f, 0.5f, 1.0f }, 4);
        }

        // 4. Отрисовка тактического прицела мыши Pip-Pad
        ScreenPoint spMouse = isoCamera.WorldToScreen(mouseWorldPos, cameraTarget);
        PushCircle(vBuffer, spMouse.x, spMouse.y, 11.0f, width, height, { 1.0f, 0.7f, 0.0f, 0.5f }, 8);
        PushCircle(vBuffer, spMouse.x, spMouse.y, 3.0f, width, height, { 1.0f, 0.7f, 0.0f, 1.0f }, 4);

        // 5. Отрисовка СОЮЗНОГО ТАНКА С ИИ (Если игрок снаружи пешком)
        if (!titan.isPiloted) {
            ScreenPoint sTitan = isoCamera.WorldToScreen(titan.position, cameraTarget);
            PushCircle(vBuffer, sTitan.x, sTitan.y, 15.0f, width, height, { 0.05f, 0.45f, 0.85f, 1.0f }, 6);
        }

        // 6. Отрисовка маркера игрока (Скаут или управляемый Танк)
        ScreenPoint sPlayer = isoCamera.WorldToScreen(playerPos, cameraTarget);
        if (playerMode == UnitMode::Scout) {
            PushCircle(vBuffer, sPlayer.x, sPlayer.y, 9.0f, width, height, { 0.18f, 0.85f, 0.4f, 1.0f }, 8);
        } else {
            PushCircle(vBuffer, sPlayer.x, sPlayer.y, 17.0f, width, height, { 0.02f, 0.52f, 0.98f, 1.0f }, 6);
        }

        // 7. Отрисовка Вышки связи региона (Vault 17 Relay Tower)
        ScreenPoint spTower = isoCamera.WorldToScreen(towerPosition, cameraTarget);
        if (regionalGrid.towerSyncRecovered) {
            PushCircle(vBuffer, spTower.x, spTower.y, 14.0f, width, height, { 0.0f, 0.6f, 1.0f, 1.0f }, 3);
        } else {
            PushCircle(vBuffer, spTower.x, spTower.y, 14.0f, width, height, { 0.25f, 0.2f, 0.2f, 1.0f }, 3);
        }

        // Индикаторы состояния повреждения узлов Танка с ИИ на HUD в левом углу
        if (playerMode == UnitMode::Titan) {
            float indicatorY = (height - 45.0f) - 30.0f;
            Vertex tracksCol = (titan.systems.tracksCondition > 40.0f) ? Vertex{0.2f, 0.8f, 0.2f, 1.0f} : Vertex{0.9f, 0.2f, 0.2f, 1.0f};
            PushCircle(vBuffer, 45.0f + 10.0f, indicatorY, 4.0f, width, height, tracksCol, 4);
            
            Vertex turretCol = (titan.systems.turretStatus > 50.0f) ? Vertex{0.2f, 0.8f, 0.2f, 1.0f} : Vertex{0.9f, 0.2f, 0.2f, 1.0f};
            PushCircle(vBuffer, 45.0f + 30.0f, indicatorY, 4.0f, width, height, turretCol, 4);
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
        vBuffer.push_back({ hpBg0.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ hpBg1.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ hpBg0.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ hpBg1.x, hpBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ hpBg1.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ hpBg0.x, hpBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });

        if (healthRatio > 0.0f) {
            ScreenPoint h1 = PixelsToNDC(bx1 + (barWidth * healthRatio), by1 + barHeight, width, height);
            Vertex hCol = (playerMode == UnitMode::Titan) ? Vertex{ 0.0f, 0.52f, 0.95f, 1.0f } : Vertex{ 0.18f, 0.85f, 0.4f, 1.0f };
            vBuffer.push_back({ hpBg0.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
            vBuffer.push_back({ h1.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
            vBuffer.push_back({ hpBg0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
            vBuffer.push_back({ h1.x, hpBg0.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
            vBuffer.push_back({ h1.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
            vBuffer.push_back({ hpBg0.x, h1.y, 0.0f, hCol.r, hCol.g, hCol.b, hCol.a });
        }

        // Шкала Эфирной Эрозии (В нижнем правом углу)
        float bx2 = width - barWidth - 45.0f;
        float by2 = height - 45.0f;
        float erosionRatio = playerErosionLevel / 100.0f;

        ScreenPoint erBg0 = PixelsToNDC(bx2, by2, width, height);
        ScreenPoint erBg1 = PixelsToNDC(bx2 + barWidth, by2 + barHeight, width, height);
        vBuffer.push_back({ erBg0.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ erBg1.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ erBg0.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ erBg1.x, erBg0.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ erBg1.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });
        vBuffer.push_back({ erBg0.x, erBg1.y, 0.0f, 0.15f, 0.16f, 0.18f, 1.0f });

        if (erosionRatio > 0.0f) {
            ScreenPoint e1 = PixelsToNDC(bx2 + (barWidth * erosionRatio), by2 + barHeight, width, height);
            Vertex eCol = { 0.85f, 0.5f, 0.05f, 1.0f }; // Предупреждающее оранжевое свечение эфира
            vBuffer.push_back({ erBg0.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
            vBuffer.push_back({ e1.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
            vBuffer.push_back({ erBg0.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
            vBuffer.push_back({ e1.x, erBg0.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
            vBuffer.push_back({ e1.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
            vBuffer.push_back({ erBg0.x, e1.y, 0.0f, eCol.r, eCol.g, eCol.b, eCol.a });
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

        g_pd3dDeviceContext->Draw(static_cast<UINT>(vBuffer.size()), 0);
        g_pSwapChain->Present(1, 0);
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