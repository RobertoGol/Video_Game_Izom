cmake_minimum_required(VERSION 3.15)
project(BunkerProtocolEngine VERSION 15.0 LANGUAGES CXX)

# ==============================================================================
# 1. СИСТЕМНЫЕ НАСТРОЙКИ КОМПИЛЯТОРА И СТАНДАРТА C++
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Конфигурация строгого режима компиляции MSVC (Убираем ворнинги, включаем NOMINMAX)
if(MSVC)
    add_compile_options(
        /W4 
        /WX 
        /wd4100 # Игнорировать предупреждение о неиспользованных аргументах функций
        /wd4996 # Игнорировать предупреждения об устаревших POSIX вызовах
        /wd4244 # Игнорировать предупреждения о приведении типов double -> float
        /wd4267 # Игнорировать предупреждения о конвертации size_t -> UINT
        /NOMINMAX
    )
endif()

# ==============================================================================
# 2. ОПРЕДЕЛЕНИЕ ЗАГOЛОВОЧНЫХ ДИРЕКТОРИЙ (SEARCH PATHS)
# ==============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Data
    ${CMAKE_CURRENT_SOURCE_DIR}/Player
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/Physics
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/Physics/Technik
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/World/Maps
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/bodies
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/Network
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/HUD
    
)

# ==============================================================================
# 3. ПОЛНЫЙ ПУЛ ИСХOДНЫХ КОДОВ ЯДРА ИГРЫ (ENGINE CORE STRUCTURE)
# ==============================================================================
set(BUNKER_ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/Types.hpp

    
    # Модули Рендеринга, Шейдеров и Камеры
    # Все остальные пути ниже, такие как Data/Camera.cpp и т.д., 
    # у вас уже написаны правильно относительно корня, их менять не нужно!
    Data/Camera.cpp
    Data/Camera.hpp
    Data/Renderer.cpp
    Data/Renderer.hpp
    Data/RetroShader.hpp
    
    # Управление Игроком, Контроллер Ввода и Инвентарь
    Data/PlayerController.cpp
    Data/PlayerController.hpp
    Player/Player.cpp
    Player/Player.hpp
    Player/Inventory.cpp
    Player/Inventory.hpp

    # Системные Бинарные Сейвы (State of Decay style)
    Player/SaveSystem.cpp
    Player/SaveSystem.hpp
    
    # Физика, Тактики Пилота и Коллизии Масс
    Data/Physics/Tactics.cpp
    Data/Physics/Tactics.hpp
    Data/Physics/Tank.cpp
    Data/Physics/Tank.hpp
    Data/Physics/Collisions.cpp
    Data/Physics/Collisions.hpp
    Data/Physics/Mechanics.cpp
    Data/Physics/Mechanics.hpp
    Data/Physics/Grid.cpp
    Data/Physics/Grid.hpp
    
    # Симуляция Ретро-Техники (Паромобиль, Мотоцикл)
    Data/Physics/Technik/SteamCar.hpp
    Data/Physics/Technik/Motorcycle.hpp
    Data/Physics/Technik/VehicleManager.cpp
    Data/Physics/Technik/VehicleManager.hpp
    
    # Архитектура Мира, Сетко-Генератор и Автогенерация Лута
    Data/World/Map.cpp
    Data/World/Map.hpp
    Data/World/LootSystem.cpp
    Data/World/LootSystem.hpp
    
    # Сетевой winsock2 Драйвер Репликации на 20 Человек
    Data/Network/NetManager.cpp
    Data/Network/NetManager.hpp
    
    # База Данных Профилей Врагов и Манекенов .aut
    Data/bodies/Enemies.cpp
    Data/bodies/Enemies.hpp
    
    # Монохромные Интерфейсы Pip-Pad HUD и ЭЛТ Эффекты
    Data/HUD/HUD.cpp
    Data/HUD/HUD.hpp
    Data/HUD/UIEffects.cpp
    Data/HUD/UIEffects.hpp
)

# ==============================================================================
# 4. СБOРКА ОСНОВНОГО ИСПОЛНЯЕМОГО ФАЙЛА ИГРЫ (WIN32 SUBSYSTEM)
# ==============================================================================
add_executable(BunkerProtocol WIN32 ${BUNKER_ENGINE_SOURCES})

# Жесткая линковка низкоуровневых библиотек DirectX 11, Шейдеров и Сети Winsock2
target_link_libraries(BunkerProtocol
    PRIVATE
    d3d11       # Графический интерфейс DirectX 11
    d3dcompiler # Аппаратный компилятор HLSL шейдеров
    dxgi        # Инфраструктура обмена графических конвейеров (SwapChain)
    user32      # Системные окна Windows API, GetAsyncKeyState
    gdi32       # Графический интерфейс ОС
    winmm       # Высокоточный таймер deltaTime
    Ws2_32      # Winsock2 — Сеть на 20 игроков без лагов
)

# ==============================================================================
# 5. ИЗОЛИРОВАННАЯ СБОРКА МОДУЛЯ РЕДАКТОРА (Editor/src -> BunkerArchitect)
# ==============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Editor/src")
    file(GLOB_RECURSE EDITOR_SOURCES "Editor/src/*.cpp" "Editor/src/*.hpp")
    if(EDITOR_SOURCES)
        add_executable(BunkerEditor WIN32 ${EDITOR_SOURCES} ${BUNKER_ENGINE_SOURCES})
        target_link_libraries(BunkerEditor PRIVATE d3d11 d3dcompiler dxgi user32 gdi32 winmm Ws2_32)
        message(STATUS "[CMAKE] Модуль редактора карт 'BunkerArchitect' успешно интегрирован в сборку.")
    endif()
endif()

# ==============================================================================
# 6. СБОРКА АВТОТЕСТОВ МАТЕМАТИКИ ДВИЖКА (tests/ -> EngineUnitTests)
# ==============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    file(GLOB TEST_SOURCES "tests/*.cpp")
    if(TEST_SOURCES)
        add_executable(EngineUnitTests ${TEST_SOURCES} ${BUNKER_ENGINE_SOURCES})
        target_link_libraries(EngineUnitTests PRIVATE d3d11 d3dcompiler dxgi user32 gdi32 winmm Ws2_32)
        message(STATUS "[CMAKE] Сконфигурирован автоматический юнит-тестировщик физики.")
    endif()
endif()
