#include "main.hpp"      // Подключаем главный заголовочный файл проекта со всеми внешними связями
#include <chrono>        // Подключаем высокоточный хронометр std::chrono для расчета deltaTime кадра
#include <iostream>      // Подключаем потоки ввода-вывода для отладочных сообщений в консоли движка
#include <d3dcompiler.h> // Подключаем компилятор HLSL шейдеров DirectX 11 для ЭЛТ-фильтра

// ============================================================================//
// SOLID FORWARD DECLARATION BLOCK TO FORCIBLY OVERRIDE HEADERS ISOLATION      //
// ============================================================================//
namespace bunker
{
    struct Vertex; // Declares the token layout for your vector containers

    void InitializeRendererTables();
    void UpdateHelldiversCamera(float deltaTime);
    void ProcessGameMechanics(float deltaTime);
    void RenderTacticalHUD(std::vector<bunker::Vertex> &vBuffer, float wWidth, float wHeight);
    void RenderUIEffects(std::vector<bunker::Vertex> &vBuffer, float wWidth, float wHeight);
}

// ============================================================================//
// ВЫДЕЛЕНИЕ ФИЗИЧЕСКОЙ ПАМЯТИ ПОД ВСЕ ГЛОБАЛЬНЫЕ ОБЪЕКТЫ ИГРЫ (С префиксами)  //
// ============================================================================//
// Выделение памяти под глобальные объекты должно быть строго в одном экземпляре:

bunker::UnitMode playerMode = bunker::UnitMode::Scout; // Режим игрока (по лору v15 изначально равен Scout)
bunker::Vector3D playerPos = {5.0f, 5.0f, 0.0f};       // Стартовая позиция Пилота в Убежище 17
float playerHealth = 100.0f;                           // Текущее здоровье персонажа
float playerMaxHealth = 100.0f;                        // Максимальный запас здоровья
float playerSpeed = 5.5f;                              // Скорость перемещения экзокостюма
float fireCooldown = 0.0f;                             // Задержка (перезарядка) между выстрелами карабина
float playerErosionLevel = 0.0f;                       // Текущий уровень заражения персонажа эрозией эфира
int score = 0;                                         // Игровые очки/счет сессии

// Префиксы bunker:: для сопоставления типов линкером
bunker::Vector3D cameraTarget = {5.0f, 5.0f, 0.0f};       // Точка, куда плавно смотрит изометрическая камера
bunker::IsometricCamera isoCamera;                        // Экземпляр математической камеры движка
bunker::Vector3D mouseWorldPos = {0.0f, 0.0f, 0.0f};      // Координаты курсора мыши, спроецированные на землю
bunker::ScreenPoint currentMouseScreenPos = {0.0f, 0.0f}; // Пиксельные координаты мыши на экране окна

// Синхронизированные векторы в пространстве имен bunker::
std::vector<bunker::Bullet> bullets; // Динамический массив всех летящих снарядов в кадре
std::vector<bunker::Enemy> enemies;  // Динамический массив всех активных врагов Роя Верминов
float enemySpawnTimer = 0.0f;        // Таймер генерации волн противников

bunker::Vault17ClassManager gameClasses;             // Менеджер тактических способностей (крюк, стим, фаза)
bunker::Vault17GridState regionalGrid;               // Монитор состояния энергосетей и реле связи секторов
bunker::VanguardTitan titanSimulation;               // Процессор цепей симуляции ИИ БТ-7274
bunker::TitanAlly titan;                             // Физический объект союзного Титана на поле боя
bunker::Vector3D towerPosition = {9.5f, 9.5f, 0.0f}; // Мировые координаты центральной башни синхронизации

bool isAiming = false;                        // Флаг: зажата ли сейчас правая кнопка мыши (прицеливание Пилота)
float baseZoom = 55.0f;                       // Базовый угол обзора изометрической линзы (в градусах)
bunker::Vault17Progression bunkerProgression; // Объект отслеживания сюжетных триггеров и Pip-Pad планшета

// ВЫДЕЛЕНИЕ ПАМЯТИ ПОД ИНФРАСТРУКТУРУ DIRECT3D 11
ID3D11Device *g_pd3dDevice = nullptr;                     // Графический процессор (выделение видеопамяти и ресурсов)
ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;       // Контекст отрисовки (вызов команд DirectX 11)
IDXGISwapChain *g_pSwapChain = nullptr;                   // Буфер вывода изображения на экран (SwapChain конвейера)
ID3D11RenderTargetView *g_mainRenderTargetView = nullptr; // Представление буфера кадра для очистки и вывода
ID3D11VertexShader *g_pVertexShader = nullptr;            // Вершинный микрокод HLSL шейдера
ID3D11PixelShader *g_pPixelShader = nullptr;              // Пиксельный микрокод HLSL шейдера (ЭЛТ-интерлейсинг)
ID3D11InputLayout *g_pInputLayout = nullptr;              // Разметка структуры вершин Vertex для видеокарты
ID3D11Buffer *g_pVertexBuffer = nullptr;                  // Динамический видеобуфер под полигоны

// ФИЗИЧЕСКИЕ МАТРИЦЫ ЖИВОЙ СЕТКИ КАРТЫ (Синхронизировано с константами MAP_WIDTH и MAP_HEIGHT)
int currentSectorMap[bunker::MAP_WIDTH][bunker::MAP_HEIGHT];  // Двумерная матрица тайлов ландшафта сектора
int wallDurability[bunker::MAP_WIDTH][bunker::MAP_HEIGHT];    // Двумерная матрица прочности разрушаемых стен бункера
float etherErosionMap[bunker::MAP_WIDTH][bunker::MAP_HEIGHT]; // Двумерная тепловая карта плотности заражения эфира

// Глобальный легковесный кэш мыши для разгрузки циклов от Win32 Kernel Mode задержек
static bunker::ScreenPoint g_ThreadSafeMousePos = {0.0f, 0.0f};

// Глобальные менеджеры систем сессии (Инвентарь, Транспорт, Живая Карта)
bunker::PlayerInventory g_PlayerInventory;                // Объект инвентаря рюкзака Пилота
static bunker::VehicleManager g_VehicleManager;           // Менеджер симуляции колесной техники (SteamCar/Motorcycle)
static bunker::SessionWorldManager g_SessionWorldManager; // Менеджер сохранения и генерации мира .wld

// ============================================================================
// СИСТЕМНЫЕ ФУНКЦИИ И ОПТИМИЗАЦИИ DIRECT3D 11
// ============================================================================

// Функция создания Представления Отрисовки (Render Target View)
HRESULT CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBackBuffer);
    if (FAILED(hr))
        return hr;
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
    return hr;
}

// Освобождение ресурсов DirectX 11 при закрытии игры (предотвращает утечки видеопамяти)
void CleanupDeviceD3D()
{
    if (g_pVertexBuffer)
    {
        g_pVertexBuffer->Release();
        g_pVertexBuffer = nullptr;
    }
    if (g_pInputLayout)
    {
        g_pInputLayout->Release();
        g_pInputLayout = nullptr;
    }
    if (g_pPixelShader)
    {
        g_pPixelShader->Release();
        g_pPixelShader = nullptr;
    }
    if (g_pVertexShader)
    {
        g_pVertexShader->Release();
        g_pVertexShader = nullptr;
    }
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

// Системный обработчик оконных сообщений ОС Windows (Win32 WndProc)
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
        // АППАРАТНАЯ ОПТИМИЗАЦИЯ: Извлекаем пиксели из lParam, не опрашивая Windows API в кадрах
        g_ThreadSafeMousePos.x = static_cast<float>(LOWORD(lParam));
        g_ThreadSafeMousePos.y = static_cast<float>(HIWORD(lParam));
        return 0;

    case WM_SIZE:
        // Динамический ресайз окна: пересоздаем RenderTarget под новое разрешение экрана
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            if (g_mainRenderTargetView)
            {
                g_mainRenderTargetView->Release();
                g_mainRenderTargetView = nullptr;
            }
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0; // Блокируем Alt фризы окон движка
        break;

    case WM_DESTROY:
        // Автосохранение живой экосистемы сессии в файл base.wld при закрытии игры (в стиле State of Decay)
        g_SessionWorldManager.SaveWorldSessionToWld();

        // Автосохранение характеристика персонажа и рюкзака игрока в ./saves/save_1220.sav
        bunker::PlayerSaveData sData;
        sData.position = playerPos;
        sData.currentMode = playerMode;
        sData.health = playerHealth;
        sData.maxHealth = playerMaxHealth;
        sData.erosionLevel = playerErosionLevel;
        sData.currentScore = score;

        // Запись сериализованных данных на диск
        bunker::SaveSystem::WriteSaveGame(1220, sData, g_PlayerInventory);

        PostQuitMessage(0); // Посылаем сигнал завершения потока операционной системе
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
// ============================================================================
// ГЛАВНАЯ ТОЧКА ВХОДА ПРИЛОЖЕНИЯ WINDOWS (WinMain)
// ============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 1. Регистрация класса окна Вашего Изометрического Движка
    const wchar_t CLASS_NAME[] = L"BunkerProtocolD3D11EngineClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    float wWidth = 1280.0f;
    float wHeight = 720.0f;

    RECT wr = {0, 0, static_cast<LONG>(wWidth), static_cast<LONG>(wHeight)};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowExW(
        0, CLASS_NAME, L"Bunker Protocol - Vault 17 Core v15 (DX11)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInstance, nullptr);

    if (hWnd == nullptr)
        return 0;

    // 2. Инициализация SwapChain и контекста графического конвейера Direct3D 11
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = static_cast<UINT>(wWidth);
    sd.BufferDesc.Height = static_cast<UINT>(wHeight);
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (FAILED(hr))
    {
        CleanupDeviceD3D();
        return 0;
    }
    if (FAILED(CreateRenderTarget()))
    {
        CleanupDeviceD3D();
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 3. Компиляция встроенного HLSL шейдерного ядра на лету (Подгружаем наш ЭЛТ-фильтр из RetroShader.hpp)
    const char *shaderCode = bunker::g_BunkerRetroHLSL;
    ID3DBlob *vertexBlob = nullptr;
    hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vertexBlob, nullptr);
    if (FAILED(hr))
    {
        CleanupDeviceD3D();
        return 0;
    }

    hr = g_pd3dDevice->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        vertexBlob->Release();
        CleanupDeviceD3D();
        return 0;
    }

    D3D11_INPUT_ELEMENT_DESC localLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    hr = g_pd3dDevice->CreateInputLayout(localLayout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &g_pInputLayout);
    vertexBlob->Release();
    if (FAILED(hr))
    {
        CleanupDeviceD3D();
        return 0;
    }

    ID3DBlob *pixelBlob = nullptr;
    hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &pixelBlob, nullptr);
    if (FAILED(hr))
    {
        CleanupDeviceD3D();
        return 0;
    }

    hr = g_pd3dDevice->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pixelBlob->Release();
    if (FAILED(hr))
    {
        CleanupDeviceD3D();
        return 0;
    }

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(bunker::Vertex) * 4096; // Запас видеопамяти под 4096 вершин движка
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);
    if (FAILED(hr))
    {
        CleanupDeviceD3D();
        return 0;
    }

    // ============================================================================
    // 4. СТАРТ ИГРОВЫХ СИСТЕМ (Загрузка кэша манекенов .aut и живой сессии .wld)
    // ===========================================================================
    bunker::InitializeEnemiesRegistry();
    g_SessionWorldManager.LoadWorldSessionFromWld(); // Загружает или создает сессионный base.wld
    bunker::InitializePlayer();

    // ИСПРАВЛЕНО: Этот вызов теперь работает идеально, так как LUT-таблицы объявлены в блоке опережения вверху!
    bunker::InitializeRendererTables(); // Запекание тригонометрических таблиц кругов для быстродействия

    // Пробуем восстановить последний сейв персонажа, если он есть в папке ./saves/
    bunker::PlayerSaveData loadedState;
    if (bunker::SaveSystem::ReadSaveGame(1220, loadedState, g_PlayerInventory))
    {
        playerPos = loadedState.position;
        playerMode = loadedState.currentMode;
        playerHealth = loadedState.health;
        playerMaxHealth = loadedState.maxHealth;
        playerErosionLevel = loadedState.erosionLevel;
        score = loadedState.currentScore;
        std::cout << "[SYSTEM] Успешно восстановлен прогресс персонажа из saves/slot_1220.sav" << std::endl;
    }

    // Запекание настроек камеры под геометрию экрана
    isoCamera.zoom = baseZoom;
    isoCamera.centerOffset.x = wWidth * 0.5f;
    isoCamera.centerOffset.y = wHeight * 0.5f;

    auto lastTime = std::chrono::high_resolution_clock::now();

    // Переменная vBuffer создается строго ДО входа в игровой цикл!
    std::vector<bunker::Vertex> vBuffer;

    MSG msg = {};
    bool isGameRunning = true;
    std::cout << "[SYSTEM] Ядро Direct3D 11 запущено. Вход в игровой цикл..." << std::endl;

    std::cout << "[SYSTEM] Ядро Direct3D 11 запущено. Вход в игровой цикл..." << std::endl;

    // ============================================================================
    // 5. ГЛАВНЫЙ ИГРОВОЙ ЦИКЛ (HIGH-PERFORMANCE CORE ENGINE LOOP)
    // ============================================================================
    while (isGameRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                isGameRunning = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!isGameRunning)
        {
            break;
        }

        // Точный расчет deltaTime кадра через хронометр системы
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Защита от лагов физики (кап на случай фриза окна ОС)
        if (deltaTime > 0.1f)
        {
            deltaTime = 0.1f;
        }

        // А Обновление Кулдаунов тактических способностей Пилота (крюк, стим, сонар, фаза)
        gameClasses.UpdateCooldowns(deltaTime);

        // Б Обновление физики движения Пилота и считывание легкого кэша мыши
        bunker::UpdatePlayerLogic(deltaTime, wWidth, wHeight, g_ThreadSafeMousePos);

        // В Плавное следование камеры и тактический сдвиг прицела Helldivers
        bunker::UpdateHelldiversCamera(deltaTime);

        // Г Физика троса, параболы высоты и натяжения Крюка-кошки (Grapple)
        gameClasses.ProcessGrapplePhysics(playerPos, deltaTime);

        // Д Тик ИИ БТ-7274 Авангарда (Аналоговые мосты, Вортекс-цепи, Combat Anchor)
        titanSimulation.UpdateBT7274(deltaTime);

        // Е Симуляция физики несовременной колесной техники (SteamCar / Motorcycle)
        if (g_VehicleManager.GetActiveVehicleType() != bunker::VehicleType::None)
        {
            g_VehicleManager.UpdateActiveVehiclePhysics(deltaTime);
        }

        // Ж Тик живой карты сессии (Разрастание Эрозии, падение припасов)
        g_SessionWorldManager.UpdateLiveWorldGrid(deltaTime);

        // З Обновление ИИ Роя Верминов и баллистический конвейер
        bunker::ProcessGameMechanics(deltaTime);

        // И ОЧИСТКА И РЕЗЕРВИРОВАНИЕ ВЕРШИН (Убираем микростаттеры выделения кучи)
        vBuffer.clear();
        vBuffer.reserve(1024);

        // К Отрисовка Pip-Pad шкал
        bunker::RenderTacticalHUD(vBuffer, wWidth, wHeight);

        // Л Отрисовка эффектов интерфейса (Scanlines, ЭЛТ помехи)
        bunker::RenderUIEffects(vBuffer, wWidth, wHeight);

        // М КОНВЕЙЕРНЫЙ ВЫВОД ГЕОМЕТРИИ НА ЭКРАН ВИДЕОКАРТЫ DIRECTX 11
        float clearColor[] = {0.05f, 0.05f, 0.06f, 1.0f}; // Глубокий бункерный фон
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);

        if (!vBuffer.empty())
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            // Копируем вершины динамического буфера во временную память видеокарты
            hr = g_pd3dDeviceContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            if (SUCCEEDED(hr))
            {
                size_t bytesToCopy = sizeof(bunker::Vertex) * (std::min)(vBuffer.size(), static_cast<size_t>(4096));
                // Данные берутся строго из vBuffer.data()
                memcpy(mappedResource.pData, vBuffer.data(), bytesToCopy);
                g_pd3dDeviceContext->Unmap(g_pVertexBuffer, 0);
            }
            UINT stride = sizeof(bunker::Vertex);
            UINT offset = 0;
            g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
            g_pd3dDeviceContext->IASetInputLayout(g_pInputLayout);
            g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            g_pd3dDeviceContext->VSSetShader(g_pVertexShader, nullptr, 0);
            g_pd3dDeviceContext->PSSetShader(g_pPixelShader, nullptr, 0);
            D3D11_VIEWPORT vp;
            vp.Width = wWidth;
            vp.Height = wHeight;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = 0;
            vp.TopLeftY = 0;
            g_pd3dDeviceContext->RSSetViewports(1, &vp);
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            // Вызов отрисовки полигонов Direct3D 11
            g_pd3dDeviceContext->Draw(static_cast<UINT>(vBuffer.size()), 0);
        }
        // Вывод готового кадра под герцовку монитора (V-Sync)
        g_pSwapChain->Present(1, 0);
    }
    // 6. Полная деинициализация и освобождение памяти
    CleanupDeviceD3D();
    return static_cast<int>(msg.wParam);
}
