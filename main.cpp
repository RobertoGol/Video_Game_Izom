#include <iostream>
#include <cmath>
#include <algorithm>

// Подключаем наш заглавный заголовочный файл глобальных систем
#include "main.hpp"

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
TitanAlly titan;
Vector3D towerPosition = { 10.0f, 10.0f, 0.0f };
float playerErosionLevel = 0.0f;
bool isAiming = false;
float baseZoom = 55.0f;

ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;
ID3D11InputLayout*      g_pInputLayout = nullptr;
ID3D11Buffer*           g_pVertexBuffer = nullptr;

// Строковые исходники шейдеров HLSL встроенные в движок
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
}
)";

const char* pixelShaderSrc = R"(
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
float4 main(PS_INPUT input) : SV_TARGET {
    return input.col;
}
)";

void CreateRenderTarget() {
    ID3D12Texture2D* pBackBuffer = nullptr; // Фикс типов Direct3D 11
    ID3D11Texture2D* pBB11 = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBB11));
    g_pd3dDevice->CreateRenderTargetView(pBB11, nullptr, &g_mainRenderTargetView);
    pBB11->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Нативная оконная процедура Win32
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
                    if (std::sqrt(dx * dx + dy * dy) <= 1.8f) {
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
                    titan.currentWeapon = (titan.currentWeapon == TankWeaponMode::Cannon) ? TankWeaponMode::AutoCannon : TankWeaponMode::Cannon;
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
            }
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"PureIsoDX11", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(L"PureIsoDX11", L"Vault 17 Outpost Alpha - DirectX 11", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE;

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

    InitializeVault17Map(); // Модульный вызов инициализации нативной карты

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

        // МОДУЛЬНЫЕ ВЫЗОВЫ ПОТОКОВ КАДРА
        HandleInput(hwnd, deltaTime, width, height);
        UpdateHelldiversCamera(deltaTime);
        SpawnVerminGrid(deltaTime);
        UpdateVerminAI(deltaTime);
        UpdateAutonomousTankAI(deltaTime);
        ProcessGameMechanics(deltaTime);

        // --- ГЕНЕРАЦИЯ ГЕОМЕТРИИ ВЕРШИН ---
        std::vector<Vertex> vBuffer;

        for (int x = 0; x < MAP_WIDTH; ++x) {
            for (int y = 0; y < MAP_HEIGHT; ++y) {
                Vector3D blockPos = { (float)x, (float)y, 0.0f };
                ScreenPoint sp = isoCamera.WorldToScreen(blockPos, cameraTarget);
                if (currentSectorMap[x][y] == 1) {
                    float damageFactor = wallDurability[x][y] / 100.0f;
                    PushCircle(vBuffer, sp.x, sp.y, 16.0f, width, height, { 0.32f * damageFactor, 0.35f * damageFactor, 0.38f * damageFactor, 1.0f }, 4);
                } else {
                    if (etherErosionMap[x][y] > 0.0f) PushCircle(vBuffer, sp.x, sp.y, 2.0f, width, height, { 0.8f, 0.55f, 0.1f, 0.8f }, 4);
                    else PushCircle(vBuffer, sp.x, sp.y, 1.5f, width, height, { 0.15f, 0.18f, 0.2f, 1.0f }, 4);
                }
            }
        }

        bool isRadarActive = (playerMode == UnitMode::Scout) || (titan.systems.sensorLink >= 30.0f);
        for (const auto& e : enemies) {
            if (!e.isAlive) continue;
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

         // Обновляем таймеры невидимости / фазы / стима каждый кадр
        gameClasses.UpdateCooldowns(deltaTime);
        
        // ИСПРАВЛЕНО: Интеграция физического движка Крюка-кошки из Tactics.cpp
        if (gameClasses.isTacticalActive && gameClasses.GetActivePilotClass() == PilotClass::Grapple) {
            gameClasses.ProcessGrapplePhysics(playerPos, deltaTime);
        } else {
            // Если на кошке не летим — скорость ходьбы Скаута определяется классом
            if (!titan.isPiloted) {
                playerSpeed = gameClasses.currentStats.moveSpeed;
            }
        }

        HandleInput(hwnd, deltaTime, width, height);

        // ИСПРАВЛЕНО: Рендеринг шкал и биометрии HUD через новые изолированные модули
        RenderTacticalHUD(vBuffer, width, height); // Вызов из Data/HUD/HUD.cpp
        RenderUIEffects(vBuffer, width, height);   // Вызов из Data/HUD/UIEffects.cpp

        // ЗАГРУЗКА И СИНХРОНИЗАЦИЯ СГЕНЕРИРОВАННОЙ ГЕОМЕТРИИ В ВИДЕОПАМЯТЬ
        D3D11_MAPPED_SUBRESOURCE ms;
        g_pd3dDeviceContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        memcpy(ms.pData, vBuffer.data(), sizeof(Vertex) * vBuffer.size());
        g_pd3dDeviceContext->Unmap(g_pVertexBuffer, 0);

        // ФАЗА РЕНДЕРИНГА DIRECTX 11
        const float clear_color[] = { 0.05f, 0.05f, 0.06f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        // Если пилот ушел в Фазовый Сдвиг, экран обесцвечивается в серые тона изнанки измерения
        float phase_color[] = { 0.18f, 0.19f, 0.22f, 1.0f };
        float normal_color[] = { 0.05f, 0.05f, 0.06f, 1.0f };
        
        const float* current_clear_color = (gameClasses.isPhaseDimensionActive) ? phase_color : normal_color;
        
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, current_clear_color);

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
