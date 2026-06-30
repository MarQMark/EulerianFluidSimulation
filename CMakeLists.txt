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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(EulerianFluidSimulation ${SOURCES}
        src/simulation/scene/nodes/FluidNode.cpp
        include/simulation/scene/nodes/FluidNode.h
        src/renderer/MarchingCubes.cpp
        include/renderer/MarchingCubes.h
        src/views/SceneView.cpp
        include/views/SceneView.h
        src/views/NodePropertiesView.cpp
        include/views/NodePropertiesView.h)

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

    external/ImGuiFileDialog/ImGuiFileDialog.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    external/ImGuiFileDialog
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})

target_include_directories(EulerianFluidSimulation
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjloader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuizmo/src
)

add_compile_definitions(USE_GPU)

add_subdirectory("external/tinyobjloader")
add_subdirectory("external/json")
add_subdirectory("external/ImGuizmo")
target_link_libraries(imguizmo PUBLIC imgui)

if (UNIX)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    target_link_libraries(EulerianFluidSimulation
        PRIVATE GL
        PRIVATE imgui
        PRIVATE glut
        PRIVATE glfw
        PRIVATE dl
        PRIVATE X11
        PRIVATE pthread
        PRIVATE tinyobjloader
        PRIVATE nlohmann_json::nlohmann_json
        PRIVATE imguizmo
    )
endif(UNIX)

# --- Copy assets to build directory ---
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
)

add_dependencies(EulerianFluidSimulation CopyAssets)
