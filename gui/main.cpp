#include <iostream>
#include <vector>
#include <string>
#include <mutex>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLFW
#include <GLFW/glfw3.h>

// PhysFS
#include <physfs.h>

// Zenoh
#include <zenoh.hpp>

// Generated version header
#include "version.h"

// Custom Utilities
#include "serial_utils.h"
#include "resource_manager.h"
#include "custom_gui_controls.h"

// Global state for Zenoh communication
static std::string terminal_text;
static std::mutex terminal_mtx;
static std::unique_ptr<zenoh::Session> z_session;

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
        return -1;
    }
    PHYSFS_mount(argv[0], "/", 1);

    // -------------------------------------------------------------------------
    // 2. Initialize Zenoh (The "Glue" between GUI and Serial Process)
    // -------------------------------------------------------------------------
    auto config = zenoh::Config::create_default();
    auto session_res = zenoh::open(std::move(config));
    
    if (std::holds_alternative<zenoh::Error>(session_res)) {
        std::cerr << "Failed to open Zenoh session!" << std::endl;
        return -1;
    }
    
    z_session = std::make_unique<zenoh::Session>(std::move(std::get<zenoh::Session>(session_res)));

    // Subscribe to serial data from the backend process
    auto sub = z_session->declare_subscriber("denkis/serial/rx", [&](const zenoh::Sample& sample) {
        std::string payload = sample.get_payload().as_string();
        std::lock_guard<std::mutex> lock(terminal_mtx);
        terminal_text += payload;
        
        // Keep terminal buffer from growing infinitely (e.g., last 10k chars)
        if (terminal_text.size() > 10000) {
            terminal_text.erase(0, terminal_text.size() - 10000);
        }
    });

    // Publisher to send commands back to the serial process
    auto pub_tx = z_session->declare_publisher("denkis/serial/tx");

    // -------------------------------------------------------------------------
    // 3. Initialize GLFW & OpenGL
    // -------------------------------------------------------------------------
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return -1;

    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Denkis GUI (Decoupled)", NULL, NULL);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    // -------------------------------------------------------------------------
    // 4. Initialize ImGui
    // -------------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Resources
    ImFont* main_font = resource_manager::load_font("fonts/Roboto-Regular.ttf", 16.0f);
    auto theme = custom_gui::default_theme();
    theme.custom_font = main_font;

    // -------------------------------------------------------------------------
    // 5. Main Loop
    // -------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a full-screen docking space
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MainWorkspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        ImGui::Text("System Status: GUI CONNECTED TO ZENOH");
        ImGui::Separator();

        if (ImGui::BeginTable("LayoutTable", 2, ImGuiTableFlags_Resizable))
        {
            ImGui::TableNextColumn();
            ImGui::Text("Controls");
            
            if (custom_gui::button("Send Ping", ImVec2(0, 0), theme)) {
                pub_tx.put("PING\n");
            }

            ImGui::TableNextColumn();
            ImGui::Text("Serial Terminal (via Zenoh)");
            
            ImGui::BeginChild("TerminalRegion", ImVec2(0, 0), true);
            {
                std::lock_guard<std::mutex> lock(terminal_mtx);
                ImGui::TextUnformatted(terminal_text.c_str());
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();

            ImGui::EndTable();
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