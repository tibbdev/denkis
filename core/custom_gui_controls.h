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
    // Generates a default blue theme so you don't have to populate every field manually
    ControlTheme default_theme();

    // Custom Button
    bool button(const char* label, const ImVec2& size, const ControlTheme& theme);

    // Custom Dropdown (Combo)
    bool dropdown(const char* label, int* current_item, const char* const items[], int items_count, const ControlTheme& theme);
}
