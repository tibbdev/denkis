#include <iostream>
#include <vector>
#include <string>

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

// Serial Utilities
#include "serial_utils.h"

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
    if (!PHYSFS_init(argv[0])) 
    {
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
    if (!glfwInit()) 
    {
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
    if (window == nullptr) 
    {
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

    // -------------------------------------------------------------------------
    // 5. Dynamic Serial Port UI Setup
    // -------------------------------------------------------------------------
    std::vector<SerialPortInfo> enumerated_ports;
    std::vector<std::string> port_display_strings;
    std::vector<const char*> port_display_cstrs;
    int current_port_idx = 0;

    // Upgraded lambda to remember the selected port across refreshes
    auto refresh_serial_ports = [&]() 
    {
        // Remember currently selected port name (if any)
        std::string selected_port_name = "";
        if (!enumerated_ports.empty() && current_port_idx >= 0 && current_port_idx < enumerated_ports.size()) 
        {
            selected_port_name = enumerated_ports[current_port_idx].port_name;
        }

        enumerated_ports = enumerate_serial_ports();
        port_display_strings.clear();
        port_display_cstrs.clear();
        
        if (enumerated_ports.empty()) 
        {
            port_display_strings.push_back("No ports found");
            port_display_cstrs.push_back(port_display_strings.back().c_str());
            current_port_idx = 0;
        }
        else 
        {
            int new_idx = 0; // Default to the first item if the old port disappeared
            
            for (size_t i = 0; i < enumerated_ports.size(); ++i) 
            {
                const auto& port = enumerated_ports[i];
                port_display_strings.push_back(port.port_name + " (" + port.description + ")");
                port_display_cstrs.push_back(port_display_strings.back().c_str());
                
                // If we found the port the user had previously selected, update the index to match
                if (port.port_name == selected_port_name) 
                {
                    new_idx = static_cast<int>(i);
                }
            }
            
            current_port_idx = new_idx;
        }
    };

    // Run the initial scan
    refresh_serial_ports();
    double last_port_refresh_time = glfwGetTime();

    // Standard Baud Rates
    int current_baud_idx = 1;
    const char* bauds[] = { "9600", "115200", "256000" };

    // Format our version string once before the loop starts
    char version_text[256];
    snprintf(version_text, sizeof(version_text), "v%s | Branch: %s | Commit: %s %s", 
             GIT_TAG, GIT_BRANCH, GIT_SHA, GIT_DIRTY ? "(Dirty)" : "");

    // -------------------------------------------------------------------------
    // 6. Main Application Loop
    // -------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Handle Auto-Refresh every 0.5 seconds
        double current_time = glfwGetTime();
        if (current_time - last_port_refresh_time >= 0.5) 
        {
            refresh_serial_ports();
            last_port_refresh_time = current_time;
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- FULLSCREEN WORKSPACE SETUP ---
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("MainWorkspace", nullptr, window_flags);

        // --- CUSTOM TITLE / TOOL BAR ---
        ImGui::BeginChild("TopBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::AlignTextToFramePadding(); 
        
        // Left Side: Serial Controls
        ImGui::Text("Port:");
        ImGui::SameLine();
        
        ImGui::SetNextItemWidth(250);
        ImGui::Combo("##port", &current_port_idx, port_display_cstrs.data(), static_cast<int>(port_display_cstrs.size()));
        
        ImGui::SameLine();
        ImGui::Text("Baud:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::Combo("##baud", &current_baud_idx, bauds, IM_ARRAYSIZE(bauds));

        ImGui::SameLine();
        
        if (enumerated_ports.empty()) 
        {
            ImGui::BeginDisabled();
        }
        
        if (ImGui::Button("Connect")) 
        {
            std::cout << "Attempting to connect to " << enumerated_ports[current_port_idx].port_name 
                      << " at " << bauds[current_baud_idx] << " baud" << std::endl;
        }

        if (enumerated_ports.empty()) 
        {
            ImGui::EndDisabled();
        }

        // Right Side: Version Information
        float version_text_width = ImGui::CalcTextSize(version_text).x;
        float right_align_x = ImGui::GetWindowWidth() - version_text_width - ImGui::GetStyle().WindowPadding.x;
        
        ImGui::SameLine(right_align_x);
        ImGui::TextDisabled("%s", version_text);

        ImGui::EndChild(); // End TopBar

        // --- MAIN CONTENT AREA ---
        ImGui::Spacing();
        ImGui::Text("System Status");
        ImGui::Separator();
        ImGui::Text("PhysFS Initialized: Yes");
        ImGui::Text("Base Dir: %s", PHYSFS_getBaseDir());
        ImGui::Text("Asio I/O Context stopped: %s", io_context.stopped() ? "True" : "False");
        
        if (!enumerated_ports.empty() && current_port_idx < enumerated_ports.size())
        {
            ImGui::Text("Currently selected port: %s", enumerated_ports[current_port_idx].port_name.c_str());
        }

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
    // 7. Cleanup
    // -------------------------------------------------------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    PHYSFS_deinit();

    return 0;
}
