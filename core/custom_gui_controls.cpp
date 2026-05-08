#include "custom_gui_controls.h"

namespace custom_gui
{
    ControlTheme default_theme()
    {
        ControlTheme theme;
        theme.color_bg = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
        theme.color_hover = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        theme.color_active = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        theme.color_text = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        theme.rounding = 6.0f; // Soft rounded corners
        theme.custom_font = nullptr; // Fallback to ImGui default
        return theme;
    }

    bool button(const char* label, const ImVec2& size, const ControlTheme& theme)
    {
        // 1. Push Custom Font (if provided)
        if (theme.custom_font != nullptr)
        {
            ImGui::PushFont(theme.custom_font);
        }

        // 2. Push Colors
        ImGui::PushStyleColor(ImGuiCol_Button, theme.color_bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme.color_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme.color_active);
        ImGui::PushStyleColor(ImGuiCol_Text, theme.color_text);

        // 3. Push Layout Variables (Rounding)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, theme.rounding);

        // 4. Render the native ImGui button
        bool clicked = ImGui::Button(label, size);

        // 5. Cleanup the stack (MUST match the number of Pushes)
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(4);

        if (theme.custom_font != nullptr)
        {
            ImGui::PopFont();
        }

        return clicked;
    }

    bool dropdown(const char* label, int* current_item, const char* const items[], int items_count, const ControlTheme& theme)
    {
        if (theme.custom_font != nullptr)
        {
            ImGui::PushFont(theme.custom_font);
        }

        ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.color_bg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme.color_hover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme.color_active);
        ImGui::PushStyleColor(ImGuiCol_Text, theme.color_text);
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, theme.rounding);

        // Render Combo
        bool changed = ImGui::Combo(label, current_item, items, items_count);

        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(4);

        if (theme.custom_font != nullptr)
        {
            ImGui::PopFont();
        }

        return changed;
    }
}
