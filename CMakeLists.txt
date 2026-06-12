cmake_minimum_required(VERSION 3.15)
project(Video_Game_Izom CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Указываем, что собираем стандартное оконное приложение Windows (без консоли на заднем плане)
add_executable(${PROJECT_NAME} WIN32 
    main.cpp
)

# Линкуем нативные библиотеки DirectX 11, которые есть в любой Windows системе
target_link_libraries(${PROJECT_NAME}
    d3d11
    dxgi
    d3dcompiler
    user32
    gdi32
)
