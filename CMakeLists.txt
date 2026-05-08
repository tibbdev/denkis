cmake_minimum_required(VERSION 3.20)

# POLICY CMP0091: Allows us to easily set the MSVC static runtime via CMAKE_MSVC_RUNTIME_LIBRARY
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(denkis)
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# 1. Force static building for all dependencies
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

# 2. Force MSVC to use the Static C/C++ Runtime (/MT instead of /MD)
# This prevents the "VCRUNTIME140.dll was not found" error on Windows.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# -----------------------------------------------------------------------------
# Declare and Fetch Dependencies
# -----------------------------------------------------------------------------

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)

# Dear ImGui
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.7-docking)

# ASIO (Using the OlivierLDff wrapper, named 'asio_cmake' to avoid target collisions)
FetchContent_Declare(asio_cmake GIT_REPOSITORY https://github.com/OlivierLDff/asio.cmake.git GIT_TAG v1.3.1)

# PhysFS
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(physfs GIT_REPOSITORY https://github.com/icculus/physfs.git GIT_TAG release-3.2.0)

# Download and configure dependencies
FetchContent_MakeAvailable(glfw physfs asio_cmake)
FetchContent_MakeAvailable(imgui)

find_package(OpenGL REQUIRED)

# -----------------------------------------------------------------------------
# Setup Dear ImGui Target
# -----------------------------------------------------------------------------
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# -----------------------------------------------------------------------------
# Project Configuration
# -----------------------------------------------------------------------------
include(cmake/git-version.cmake OPTIONAL)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
add_custom_target(GenerateVersionInfo ALL
        COMMAND ${CMAKE_COMMAND}
        -D SRC_DIR=${CMAKE_CURRENT_LIST_DIR}
        -P ${CMAKE_CURRENT_LIST_DIR}/cmake/git-version.cmake
        BYPRODUCTS ${CMAKE_CURRENT_LIST_DIR}/generated/version.h
)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp")
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp")
file(GLOB_RECURSE SERIAL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/serial/*.cpp")

# If on Windows, prevent the console window from popping up in Release mode
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_executable(${CMAKE_PROJECT_NAME} WIN32 ${CORE_SOURCES} ${APP_SOURCES} ${SERIAL_SOURCES})
else()
    add_executable(${CMAKE_PROJECT_NAME} ${CORE_SOURCES} ${APP_SOURCES} ${SERIAL_SOURCES})
endif()

add_dependencies(${CMAKE_PROJECT_NAME} GenerateVersionInfo)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/serial
        ${physfs_SOURCE_DIR}/src
)
# 3. Static Linking Flags for GCC/Clang (Linux & MinGW)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Statically link libstdc++ and libgcc so users don't need them installed.
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
    
    # Workaround for MinGW multiple definition bug with type_info when linking static stdlib
    if(MINGW)
        target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,--allow-multiple-definition")
    endif()
endif()

# Link dependencies to your main executable
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE imgui asio physfs-static)

# 4. OS-Specific System Dependencies
if(WIN32)
    # Windows native socket, crypto, and windowing libraries needed for Asio and GLFW
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ws2_32 mswsock imm32 gdi32 shell32 bcrypt)

elseif(UNIX AND NOT APPLE)
    # Linux system libraries
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    # 'dl' is required for loading OpenGL dynamically at runtime. 
    # X11 and Threads are required by GLFW.
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dl Threads::Threads ${X11_LIBRARIES})

elseif(APPLE)
    # macOS system frameworks
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
        ${COCOA_FRAMEWORK} 
        ${IOKIT_FRAMEWORK} 
        ${COREVIDEO_FRAMEWORK}
    )
endif()
