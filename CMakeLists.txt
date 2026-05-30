cmake_minimum_required(VERSION 3.20)
project(EulerianFluidSimulation)

set(CMAKE_CXX_STANDARD 20)


find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()


add_executable(EulerianFluidSimulation
        src/main.cpp
        src/Simulator.cpp

        src/renderer/Window.cpp
        src/renderer/Renderer.cpp
        src/renderer/Texture2D.cpp

        src/views/Views.cpp
        src/views/SimulationView.cpp
        src/views/PropertiesView.cpp
        src/views/FPSView.cpp
        src/views/FieldViewerView.cpp

        src/simulation/Grid.cpp
        src/simulation/Properties.cpp
        src/simulation/VecField.cpp
        src/simulation/ScalField.cpp

        src/solver/LinearSolver.cpp
        src/solver/ConjugateGradient.cpp
        src/solver/ConjugateGradient.h

        src/util/Time.cpp
        src/util/GuiHelper.cpp
        src/views/StatsView.cpp
)


# DearImGUI
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})


target_include_directories(EulerianFluidSimulation
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external
)

add_compile_definitions(GLEW_STATIC)

if (UNIX)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    target_link_libraries(EulerianFluidSimulation GL GLEW imgui glut glfw dl X11 pthread)
endif(UNIX)
