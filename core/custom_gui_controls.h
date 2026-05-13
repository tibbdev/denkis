#pragma once

#include "imgui.h"

// A configuration payload for custom controls
struct ControlTheme
{
    ImVec4 color_bg;
    ImVec4 color_hover;
    ImVec4 color_active;
    ImVec4 color_text;
    float rounding;
    ImFont* custom_font;
};

namespace custom_gui
{
    
    namespace sweetie16
    {
        // Array indexing matches standard Sweetie 16 order
        const ImVec4 Sweetie16_ImVec4[16] = 
        {
            ImVec4(0.102f, 0.110f, 0.173f, 1.000f), // Blackish (#1a1c2c)
            ImVec4(0.365f, 0.153f, 0.365f, 1.000f), // Purple (#5d275d)
            ImVec4(0.694f, 0.243f, 0.325f, 1.000f), // Red/Pink (#b13e53)
            ImVec4(0.937f, 0.490f, 0.341f, 1.000f), // Orange (#ef7d57)
            ImVec4(1.000f, 0.804f, 0.459f, 1.000f), // Yellow (#ffcd75)
            ImVec4(0.655f, 0.941f, 0.439f, 1.000f), // Light Green (#a7f070)
            ImVec4(0.220f, 0.718f, 0.392f, 1.000f), // Green (#38b764)
            ImVec4(0.145f, 0.443f, 0.475f, 1.000f), // Dark Teal (#257179)
            ImVec4(0.161f, 0.212f, 0.435f, 1.000f), // Dark Blue (#29366f)
            ImVec4(0.231f, 0.365f, 0.788f, 1.000f), // Blue (#3b5dc9)
            ImVec4(0.255f, 0.651f, 0.965f, 1.000f), // Light Blue (#41a6f6)
            ImVec4(0.451f, 0.937f, 0.969f, 1.000f), // Cyan (#73eff7)
            ImVec4(0.957f, 0.957f, 0.957f, 1.000f), // Off-White (#f4f4f4)
            ImVec4(0.580f, 0.690f, 0.761f, 1.000f), // Light Gray-Blue (#94b0c2)
            ImVec4(0.337f, 0.424f, 0.525f, 1.000f), // Gray-Blue (#566c86)
            ImVec4(0.200f, 0.235f, 0.341f, 1.000f)  // Dark Blue-Gray (#333c57)
        };

        const ImU32 Sweetie16_ImU32[16] = 
        {
            0xFF2C1C1A, // #1a1c2c
            0xFF5D275D, // #5d275d
            0xFF533EB1, // #b13e53
            0xFF577DEF, // #ef7d57
            0xFF75CDFF, // #ffcd75
            0xFF70F0A7, // #a7f070
            0xFF64B738, // #38b764
            0xFF797125, // #257179
            0xFF6F3629, // #29366f
            0xFFC95D3B, // #3b5dc9
            0xFFF6A641, // #41a6f6
            0xFFF7EF73, // #73eff7
            0xFFF4F4F4, // #f4f4f4
            0xFFC2B094, // #94b0c2
            0xFF866C56, // #566c86
            0xFF573C33  // #333c57
        };
        
        enum class SweetieColor
        {
            Blackish   = 0,
            Purple     = 1,
            Red        = 2,
            Orange     = 3,
            Yellow     = 4,
            LightGreen = 5,
            Green      = 6,
            Teal       = 7,
            DarkBlue   = 8,
            Blue       = 9,
            LightBlue  = 10,
            Cyan       = 11,
            OffWhite   = 12,
            LightGray  = 13,
            Gray       = 14,
            DarkGray   = 15
        };

        inline ImVec4 GetSweetieColor(SweetieColor colorId)
        {
            return Sweetie16_ImVec4[static_cast<int>(colorId)];
        }
    } // namespace sweetie16

    // Generates a default blue theme so you don't have to populate every field manually
    ControlTheme default_theme();

    // Custom Button
    bool button(const char* label, const ImVec2& size, const ControlTheme& theme);

    // Custom Dropdown (Combo)
    bool dropdown(const char* label, int* current_item, const char* const items[], int items_count, const ControlTheme& theme);
}
