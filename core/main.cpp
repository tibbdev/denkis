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

// Generated version header
#include "version.h"

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
    if (!PHYSFS_init(argv[0])) {
        std::cerr << "PhysFS Initialization Error: " << PHYSFS_getLastErrorCode() << std::endl;
        return -1;
    }
    PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 1);

    // -------------------------------------------------------------------------
    // 2. Initialize Asio
    // -------------------------------------------------------------------------
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Denkis App", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        PHYSFS_deinit();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Maximize the window on startup
    glfwMaximizeWindow(window);

    // -------------------------------------------------------------------------
    // 4. Initialize Dear ImGui
    // -------------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Variables for our dummy Serial UI
    int current_port_idx = 0;
    const char* ports[] = { "COM1", "COM2", "COM3", "COM4" };
    int current_baud_idx = 1;
    const char* bauds[] = { "9600", "115200", "256000" };

    // Format our version string once before the loop starts
    char version_text[256];
    snprintf(version_text, sizeof(version_text), "v%s | Branch: %s | Commit: %s %s", 
             GIT_TAG, GIT_BRANCH, GIT_SHA, GIT_DIRTY ? "(Dirty)" : "");

    // -------------------------------------------------------------------------
    // 5. Main Application Loop
    // -------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- FULLSCREEN WORKSPACE SETUP ---
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        // Remove window decorations so it acts like a background canvas
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("MainWorkspace", nullptr, window_flags);

        // --- CUSTOM TITLE / TOOL BAR ---
        // Create a child window 40 pixels high with a border
        ImGui::BeginChild("TopBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
        
        // Vertically center the elements in the 40px bar
        ImGui::AlignTextToFramePadding(); 
        
        // Left Side: Serial Controls
        ImGui::Text("Port:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::Combo("##port", &current_port_idx, ports, IM_ARRAYSIZE(ports));
        
        ImGui::SameLine();
        ImGui::Text("Baud:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::Combo("##baud", &current_baud_idx, bauds, IM_ARRAYSIZE(bauds));

        ImGui::SameLine();
        if (ImGui::Button("Connect")) {
            // Serial connect logic will go here
            std::cout << "Attempting to connect to " << ports[current_port_idx] << std::endl;
        }

        // Right Side: Version Information
        float version_text_width = ImGui::CalcTextSize(version_text).x;
        // Calculate the starting X position for the right-aligned text
        float right_align_x = ImGui::GetWindowWidth() - version_text_width - ImGui::GetStyle().WindowPadding.x;
        
        ImGui::SameLine(right_align_x);
        ImGui::TextDisabled("%s", version_text); // Draw it slightly faded

        ImGui::EndChild(); // End TopBar

        // --- MAIN CONTENT AREA ---
        ImGui::Spacing();
        ImGui::Text("System Status");
        ImGui::Separator();
        ImGui::Text("PhysFS Initialized: Yes");
        ImGui::Text("Base Dir: %s", PHYSFS_getBaseDir());
        ImGui::Text("Asio I/O Context stopped: %s", io_context.stopped() ? "True" : "False");

        ImGui::End(); // End MainWorkspace

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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
