#pragma once

#include <string>

// Forward declarations for ImGui so we don't need to include the whole header here
struct ImFont;

namespace resource_manager
{
    struct Image
    {
        unsigned int texture_id;
        int width;
        int height;
        int channels;
    };

    // Loads a TTF font directly into the ImGui font atlas
    ImFont* load_font(const std::string& virtual_path, float size_pixels);

    // Loads an image into OpenGL and returns the texture details for ImGui::Image()
    Image load_image(const std::string& virtual_path);
}
