#include <iostream>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLFW (will automatically include standard OpenGL headers)
#include <GLFW/glfw3.h>

// PhysFS
#include <physfs.h>

// Asio
#include <asio.hpp>

// Your generated version header (optional, from the CMake setup)
// #include "version.h"

// GLFW Error Callback
static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int argc, char** argv)
{
    // -------------------------------------------------------------------------
    // 1. Initialize PhysFS
    // -------------------------------------------------------------------------
    // PhysFS requires the argv[0] to determine the base path of the executable.
    if (!PHYSFS_init(argv[0])) {
        std::cerr << "PhysFS Initialization Error: " << PHYSFS_getLastErrorCode() << std::endl;
        return -1;
    }

    // (Optional) Mount the base directory so you can read loose files during dev
    PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 1);

    // -------------------------------------------------------------------------
    // 2. Initialize Asio
    // -------------------------------------------------------------------------
    // Create an I/O context. This is the core object required for all Asio operations.
    asio::io_context io_context;

    // -------------------------------------------------------------------------
    // 3. Initialize GLFW & OpenGL Window
    // -------------------------------------------------------------------------
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        PHYSFS_deinit();
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Denkis App", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        PHYSFS_deinit();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // -------------------------------------------------------------------------
    // 4. Initialize Dear ImGui
    // -------------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Optional: Enable Keyboard Controls or Docking
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // -------------------------------------------------------------------------
    // 5. Main Application Loop
    // -------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Your UI Goes Here ---
        ImGui::Begin("System Status");
        ImGui::Text("Hello, World!");
        ImGui::Separator();

        // Display PhysFS Status
        ImGui::Text("PhysFS Initialized: Yes");
        ImGui::Text("Base Dir: %s", PHYSFS_getBaseDir());

        // Display Asio Status
        ImGui::Text("Asio I/O Context stopped: %s", io_context.stopped() ? "True" : "False");

        ImGui::End();
        // -------------------------

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark grey background
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // -------------------------------------------------------------------------
    // 6. Cleanup
    // -------------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    PHYSFS_deinit();

    return 0;
}