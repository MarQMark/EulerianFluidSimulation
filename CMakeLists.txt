cmake_minimum_required(VERSION 3.20)
project(EulerianFluidSimulation)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

find_package(Git REQUIRED)

execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule status --recursive
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE SUBMODULE_STATUS
        RESULT_VARIABLE RESULT
)

if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to query submodule status")
endif()

# Git prefixes uninitialized submodules with '-'
string(REGEX MATCH "(^|\n)-" HAS_UNINITIALIZED "${SUBMODULE_STATUS}")

if(HAS_UNINITIALIZED)
    message(STATUS "Initializing submodules...")
    set(MAX_RETRIES 5)
    foreach(i RANGE 1 ${MAX_RETRIES})
        execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                RESULT_VARIABLE RESULT
                ERROR_VARIABLE ERROR
        )

        if(RESULT EQUAL 0)
            break()
        endif()
        message(STATUS "Submodule update failed (attempt ${i}), retrying...")
        execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1)
    endforeach()
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "${ERROR}")
    endif()
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <format>
int main() {
    auto s = std::format(\"{}\", 42);
}
" HAS_STD_FORMAT)


file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    message(STATUS "Cuda available")
    enable_language(CUDA)

#    set(CMAKE_CUDA_COMPILER "/usr/local/cuda/bin/nvcc")
    message(STATUS "nvcc version: ${CMAKE_CUDA_COMPILER_VERSION}")
    if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 13.0)
        set(CMAKE_CUDA_ARCHITECTURES 61 75 80)
    else()
        set(CMAKE_CUDA_ARCHITECTURES 75 80)
    endif()

    file(GLOB_RECURSE CUDA_SOURCES CONFIGURE_DEPENDS "src/*.cu")
endif()

add_executable(EulerianFluidSimulation ${SOURCES} ${CUDA_SOURCES})


if(CUDAToolkit_FOUND)

    target_compile_definitions(EulerianFluidSimulation PRIVATE HAS_CUDA)
    target_compile_options(EulerianFluidSimulation PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
    )
    target_link_libraries(EulerianFluidSimulation PRIVATE CUDA::cudart)

    set_target_properties(EulerianFluidSimulation PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED ON
            CUDA_SEPARABLE_COMPILATION ON
    )
endif()


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
    ${IMGUI_DIR}/misc/cpp
    external/ImGuiFileDialog
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})

add_library(implot STATIC
    external/implot/implot.cpp
    external/implot/implot_items.cpp
)

target_include_directories(implot PUBLIC external/implot)
target_link_libraries(implot PUBLIC imgui)

target_include_directories(EulerianFluidSimulation
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobjloader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuizmo/src
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/implot
)

add_compile_definitions(USE_GPU)

add_subdirectory("external/tinyobjloader")
add_subdirectory("external/json")
add_subdirectory("external/ImGuizmo")
target_link_libraries(imguizmo PUBLIC imgui)

find_package(PkgConfig REQUIRED)

pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)

target_link_libraries(EulerianFluidSimulation PRIVATE
        PkgConfig::AVFORMAT
        PkgConfig::AVCODEC
        PkgConfig::AVUTIL
        PkgConfig::SWSCALE
)

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
        PRIVATE implot
    )
endif(UNIX)

# --- Copy assets to build directory ---
add_custom_target(CopyAssets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
)

add_dependencies(EulerianFluidSimulation CopyAssets)
