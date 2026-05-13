#include <iostream>
#include <vector>
#include <string>
#include <deque>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLFW
#include <GLFW/glfw3.h>

// PhysFS
#include <physfs.h>

// Generated version header
#include "version.h"

// Custom Utilities
#include "serial_utils.h"
#include "resource_manager.h"
#include "serial_manager.h"
#include "custom_gui_controls.h"

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
    
    if (!PHYSFS_mount(argv[0], "/", 1))
    {
        std::cerr << "Failed to mount executable as archive: " << PHYSFS_getLastErrorCode() << std::endl;
    }

    // -------------------------------------------------------------------------
    // 2. Initialize GLFW & OpenGL Window
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
    glfwSwapInterval(1);

    glfwMaximizeWindow(window);

    // -------------------------------------------------------------------------
    // 3. Initialize Dear ImGui
    // -------------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load custom font and apply it to our custom GUI theme
    ImFont* custom_font = resource_manager::load_font("fonts/Cousine/Cousine-Regular.ttf", 18.0f);
    if (custom_font != nullptr)
    {
        io.FontDefault = custom_font;
    }
    io.Fonts->Build();

    ControlTheme theme = custom_gui::default_theme();
    theme.custom_font = custom_font; // Ensure custom controls use the same font

    // -------------------------------------------------------------------------
    // 4. Application State Setup
    // -------------------------------------------------------------------------
    // UI Data
    std::vector<SerialPortInfo> enumerated_ports;
    std::vector<std::string> port_display_strings;
    std::vector<const char*> port_display_cstrs;
    int current_port_idx = 0;

    int current_baud_idx = 1;
    const char* bauds[] = { "9600", "115200", "256000", "1000000" };

    // Serial Manager Data
    SerialContext* serial_conn = nullptr;
    ThreadSafeBuffer rx_buffer;
    
    // Terminal Display Data
    std::string terminal_text;
    const size_t MAX_TERMINAL_CHARS = 10000; // Cap at 10,000 characters

    char version_text[256];
    snprintf(version_text, sizeof(version_text), "v%s | Branch: %s | Commit: %s %s", 
             GIT_TAG, GIT_BRANCH, GIT_SHA, GIT_DIRTY ? "(Dirty)" : "");

    auto refresh_serial_ports = [&]() 
    {
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
            int new_idx = 0;
            for (size_t i = 0; i < enumerated_ports.size(); ++i) 
            {
                const auto& port = enumerated_ports[i];
                port_display_strings.push_back(port.port_name + " (" + port.description + ")");
                port_display_cstrs.push_back(port_display_strings.back().c_str());
                
                if (port.port_name == selected_port_name) 
                {
                    new_idx = static_cast<int>(i);
                }
            }
            current_port_idx = new_idx;
        }
    };

    refresh_serial_ports();
    double last_port_refresh_time = glfwGetTime();

    // -------------------------------------------------------------------------
    // 5. Main Loop
    // -------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // Auto-Refresh ports if we are NOT currently connected
        double current_time = glfwGetTime();
        if ((serial_conn == nullptr || !serial_conn->is_connected) && current_time - last_port_refresh_time >= 0.5) 
        {
            refresh_serial_ports();
            last_port_refresh_time = current_time;
        }

        // Pull data from the background serial thread into our UI string
        std::vector<uint8_t> new_bytes = serial_manager::read_all_bytes(&rx_buffer);
        if (!new_bytes.empty())
        {
            terminal_text.append(new_bytes.begin(), new_bytes.end());
            
            // Cap the text display so we don't run out of RAM
            if (terminal_text.size() > MAX_TERMINAL_CHARS)
            {
                terminal_text.erase(0, terminal_text.size() - MAX_TERMINAL_CHARS);
            }
        }

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("MainWorkspace", nullptr, window_flags);

        // --- TOP BAR ---
        ImGui::BeginChild("TopBar", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::AlignTextToFramePadding(); 
        
        ImGui::Text("Port:");
        ImGui::SameLine();
        
        // Use custom dropdown
        ImGui::SetNextItemWidth(300);
        custom_gui::dropdown("##port", &current_port_idx, port_display_cstrs.data(), static_cast<int>(port_display_cstrs.size()), theme);
        
        ImGui::SameLine();
        ImGui::Text("Baud:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        custom_gui::dropdown("##baud", &current_baud_idx, bauds, IM_ARRAYSIZE(bauds), theme);

        ImGui::SameLine();
        
        if (enumerated_ports.empty()) 
        {
            ImGui::BeginDisabled();
        }
        
        // Connection Logic
        bool is_connected = (serial_conn != nullptr && serial_conn->is_connected);
        
        if (!is_connected)
        {
            if (custom_gui::button("Connect", ImVec2(100, 0), theme)) 
            {
                unsigned int baud = std::stoul(bauds[current_baud_idx]);
                serial_conn = serial_manager::connect(enumerated_ports[current_port_idx].port_name, baud);
                
                if (serial_conn != nullptr)
                {
                    serial_manager::subscribe(serial_conn, &rx_buffer);
                }
            }
        }
        else
        {
            // Alter theme slightly for a "Disconnect" button
            ControlTheme disconnect_theme = theme;
            disconnect_theme.color_bg    = custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::Red); // Red
            disconnect_theme.color_hover = custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::Orange);
            
            if (custom_gui::button("Disconnect", ImVec2(100, 0), disconnect_theme)) 
            {
                serial_manager::disconnect(serial_conn);
                serial_conn = nullptr;
            }
        }

        if (enumerated_ports.empty()) 
        {
            ImGui::EndDisabled();
        }

        float version_text_width = ImGui::CalcTextSize(version_text).x;
        float right_align_x = ImGui::GetWindowWidth() - version_text_width - ImGui::GetStyle().WindowPadding.x;
        ImGui::SameLine(right_align_x);
        ImGui::TextColored(GIT_DIRTY ? custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::Yellow) : custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::OffWhite),"%s", version_text);

        ImGui::EndChild(); // End TopBar

        // --- MAIN CONTENT AREA (2-Column Table) ---
        if (ImGui::BeginTable("MainSplit", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
        {
            // Setup columns (Left is fixed-ish, Right takes remaining space)
            ImGui::TableSetupColumn("System", ImGuiTableColumnFlags_WidthFixed, 300.0f);
            ImGui::TableSetupColumn("Terminal", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            // COLUMN 0: System Status
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("System Status");
            
            ImGui::Separator();
            
            ImGui::Text("PhysFS Initialized: ");
            ImGui::SameLine();
            ImGui::TextColored(custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::LightGreen), "YES");

            ImGui::Text("Mounted Archive:");
            ImGui::TextColored(custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::Gray), "%s", argv[0]);
            
            ImGui::Spacing();
            ImGui::Text("Serial State:");
            if (is_connected)
            {
                ImGui::TextColored(custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::LightGreen), "CONNECTED");
                ImGui::Text("Port: %s", enumerated_ports[current_port_idx].port_name.c_str());
                ImGui::Text("Baud: %s", bauds[current_baud_idx]);
            }
            else
            {
                ImGui::TextColored(custom_gui::sweetie16::GetSweetieColor(custom_gui::sweetie16::SweetieColor::Red), "DISCONNECTED");
            }

            // COLUMN 1: Terminal Display
            ImGui::TableSetColumnIndex(1);
            
            // Create a scrolling region for the text
            ImGuiWindowFlags terminal_flags = ImGuiWindowFlags_HorizontalScrollbar;
            ImGui::BeginChild("TerminalRegion", ImVec2(0, 0), false, terminal_flags);
            
            ImGui::TextUnformatted(terminal_text.c_str());
            
            // Auto-scroll to bottom if scrollbar is near the bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
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
    // Safely disconnect serial before destroying contexts
    if (serial_conn != nullptr)
    {
        serial_manager::disconnect(serial_conn);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    PHYSFS_deinit();

    return 0;
}
