cmake_minimum_required(VERSION 3.20)

project(denkis)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

# GLFW (Required for ImGui backend)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4 # Latest stable release
)

# Dear ImGui
FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.7-docking
)

# asio.cmake (Standalone Asio wrapper for CMake)
FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/abdes/asio.cmake.git
        GIT_TAG        master
)

# PhysFS
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(physfs
        GIT_REPOSITORY https://github.com/icculus/physfs.git
        GIT_TAG        release-3.2.0
)

# Download and configure standard CMake dependencies
FetchContent_MakeAvailable(glfw asio physfs)

# Fetch ImGui (only downloads it, doesn't build it yet)
FetchContent_MakeAvailable(imgui)

# Find system OpenGL
find_package(OpenGL REQUIRED)

# Create a static library for ImGui including the core files and your chosen backends
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Expose ImGui and backend headers to anything linking against this target
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

# Link ImGui to its dependencies
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp"
)

file(GLOB_RECURSE SERIAL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/serial/*.cpp"
)

add_executable(${CMAKE_PROJECT_NAME}
        ${CORE_SOURCES}
        ${APP_SOURCES}
        ${SERIAL_SOURCES}
)
add_dependencies(${CMAKE_PROJECT_NAME} GenerateVersionInfo)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/generated

        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/serial
)

# Link all dependencies to your main executable
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        imgui       # Pulls in glfw and OpenGL::GL implicitly
        asio        # Asio target provided by abdes/asio.cmake
        physfs      # PhysFS target
)