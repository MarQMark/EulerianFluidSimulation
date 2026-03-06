cmake_minimum_required(VERSION 4.0)
project(EulerianFluidSimulation)

set(CMAKE_CXX_STANDARD 20)

add_executable(EulerianFluidSimulation
        src/main.cpp
        src/Simulator.cpp

        src/renderer/Window.cpp
)

target_include_directories(EulerianFluidSimulation
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external
)

if (UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(EulerianFluidSimulation glut glfw dl X11 pthread)
endif(UNIX)
