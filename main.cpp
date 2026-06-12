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
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
    void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Win32 оконная процедура для обработки системных событий
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
        case WM_MOUSEWHEEL:
            if (playerMode == UnitMode::Titan && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
                short zDelta = (short)HIWORD(wParam);
                if (zDelta > 0) isoCamera.zoom += 4.0f;
                else isoCamera.zoom -= 4.0f;
                if (isoCamera.zoom < 30.0f) isoCamera.zoom = 30.0f;
                if (isoCamera.zoom > 90.0f) isoCamera.zoom = 90.0f;
            }
            return 0;
        case WM_KEYDOWN:
            if (wParam == 'E') {
                if (!titan.isPiloted) {
                    float dx = playerPos.x - titan.position.x;
                    float dy = playerPos.y - titan.position.y;
                    if (std::sqrt(dx*dx + dy*dy) <= 1.8f) {
                        titan.isPiloted = true;
                        gameClasses.EnterVehicle();
                        playerMode = UnitMode::Titan;
                        playerMaxHealth = gameClasses.currentStats.maxHealth;
                        playerHealth = titan.health;
                        playerSpeed = gameClasses.currentStats.moveSpeed;
                    }
                } else {
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
            if (wParam == VK_TAB) {
                if (titan.isPiloted) {
                    if (titan.currentWeapon == TankWeaponMode::Cannon) {
                        titan.currentWeapon = TankWeaponMode::AutoCannon;
                    } else {
                        titan.currentWeapon = TankWeaponMode::Cannon;
                    }
                }
            }
            if (wParam == 'Q') {
                gameClasses.ActivateTacticalSkill();
                if (gameClasses.GetActivePilotClass() == PilotClass::Stim && !titan.isPiloted) {
                    playerHealth = std::min(playerMaxHealth, playerHealth + 30.0f);
                }
            }
            if (!titan.isPiloted) {
                if (wParam == '1') gameClasses.ChangePilotClass(PilotClass::Grapple);
                if (wParam == '2') gameClasses.ChangePilotClass(PilotClass::Cloak);
                if (wParam == '3') gameClasses.ChangePilotClass(PilotClass::Stim);
            } else {
                if (wParam == '4') gameClasses.ChangeTitanFirmware(TitanClass::Ion);
                if (wParam == '5') gameClasses.ChangeTitanFirmware(TitanClass::Scorch);
                if (wParam == '6') gameClasses.ChangeTitanFirmware(TitanClass::Northstar);
                if (wParam == '7') gameClasses.ChangeTitanFirmware(TitanClass::Tone);
            }
            break;
            
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
        isoCamera.zoom += (55.0f - isoCamera.zoom) * 5.0f * deltaTime;
    }

    float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (len > 0.0f) {
        float currentMoveSpeed = playerSpeed;
        float currentMoveSpeed = playerSpeed;
        
        // Условие 1: При зажатом ПКМ Скаут прицеливается, скорость перемещения падает на 37%
        if (playerMode == UnitMode::Scout && isAiming) {currentMoveSpeed *= 0.63f; // 100% - 37% = 63%
            }
        
        // Условие 2: Критическая поломка ходовой части танка (гусениц) снижает скорость до 30%
        if (playerMode == UnitMode::Titan && titan.systems.tracksCondition < 40.0f) {
            currentMoveSpeed *= 0.3f;
        }

        float nextX = playerPos.x + (moveDir.x / len) * currentMoveSpeed * deltaTime;
        float nextY = playerPos.y + (moveDir.y / len) * currentMoveSpeed * deltaTime;
        float playerRadius = (playerMode == UnitMode::Titan) ? 0.55f : 0.25f;

        // Поочередная проверка осей гарантирует плавное скольжение без залипания в углах
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

        // --------------------------------------------------------------------
        // ОГНЕВЫЕ РЕЖИМЫ КЛАССА: СКАУТ-ОПЕРАТИВНИК
        // --------------------------------------------------------------------
        if (playerMode == UnitMode::Scout) {
            // ЛКМ: Карабин (Одиночный точный) или Дробовик кучным пучком (Если нажат ПКМ)
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (isAiming) {
                    float pelletSpread = 0.04f;
                    // Режим Дробовика: Вылетает 5 дробин кучным пучком строго в одном направлении
                    float pelletSpread = 0.04f; // Высокая кучность при зажатом ПКМ прицеле
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
        // --------------------------------------------------------------------
        // ОГНЕВЫЕ РЕЖИМЫ КЛАССА: УПРАВЛЯЕМЫЙ ТАНК С ИИ
        // --------------------------------------------------------------------
        else if (playerMode == UnitMode::Titan) {
            // Если Вермины повредили турель Танка — разброс пушки резко возрастает
            if (titan.systems.turretStatus < 50.0f) {
                float brokenFactor = ((rand() % 100) / 100.0f - 0.5f) * 0.25f;
                normDir.x += brokenFactor; normDir.y += brokenFactor;
            }

            // Нажатие на СКМ циклически меняет режим залпа ракетного модуля
            if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
                if (titan.hasMissileModule) {
                    if (titan.missileMode == MissileStrikeMode::Ballistic) {
                        // Режим 1: Залп из 7-8 ракет горизонтальной баллистической стеной
                        for (int i = -3; i <= 3; ++i) {
                            Bullet m; m.start = playerPos; m.current = playerPos;
                            m.type = BulletType::BallisticMissile; m.speed = 15.0f; m.splashRadius = 1.8f;

                            // Смещение по горизонтали перпендикулярно вектору прицела
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

            // Нажатие ЛКМ активирует выбранный на Tab ствол
            if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                if (titan.currentWeapon == TankWeaponMode::Cannon) {
                    // Режим 1: Одиночный залп тяжелой пушки Танка
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; b.speed = 25.0f; bullets.push_back(b);
                    fireCooldown = 0.4f;
                } else {
                    // Режим 2: Скорострельная автоматическая пушка-автомат (турель)
                    Bullet b; b.start = playerPos; b.current = playerPos; b.direction = normDir;
                    b.type = BulletType::Standard; b.speed = 34.0f; bullets.push_back(b);
                    // Поломка турели снижает скорострельность автомата
                    fireCooldown = (titan.systems.turretStatus < 50.0f) ? 0.18f : 0.07f;
                }
            }
        }
    }
}
