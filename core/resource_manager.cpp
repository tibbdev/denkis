#include "resource_manager.h"

// ImGui and OpenGL
#include "imgui.h"
#include <GLFW/glfw3.h>

// PhysFS
#include <physfs.h>
// Standard libraries
#include <iostream>
#include <vector>

// Windows ships with ancient GL headers. 
// Manually define the GL 1.2 Clamp-to-Edge constant if it's missing.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Define STB_IMAGE_IMPLEMENTATION only once in your project to compile the library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace resource_manager
{
    ImFont* load_font(const std::string& virtual_path, float size_pixels)
    {
        if (!PHYSFS_exists(virtual_path.c_str()))
        {
            std::cerr << "Resource Manager Error: Font not found at " << virtual_path << std::endl;
            return nullptr;
        }

        PHYSFS_File* file = PHYSFS_openRead(virtual_path.c_str());
        PHYSFS_sint64 file_size = PHYSFS_fileLength(file);

        // ImGui takes ownership of font memory and will call free() on it.
        // Therefore, we MUST allocate it using ImGui::MemAlloc instead of standard new/malloc.
        void* font_data = ImGui::MemAlloc(file_size);
        PHYSFS_readBytes(file, font_data, file_size);
        PHYSFS_close(file);

        ImGuiIO& io = ImGui::GetIO();
        
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = true; // Tell ImGui to free the memory when it's done

        return io.Fonts->AddFontFromMemoryTTF(font_data, static_cast<int>(file_size), size_pixels, &font_cfg);
    }

    Image load_image(const std::string& virtual_path)
    {
        Image img = {0, 0, 0, 0};

        if (!PHYSFS_exists(virtual_path.c_str()))
        {
            std::cerr << "Resource Manager Error: Image not found at " << virtual_path << std::endl;
            return img;
        }

        PHYSFS_File* file = PHYSFS_openRead(virtual_path.c_str());
        PHYSFS_sint64 file_size = PHYSFS_fileLength(file);

        // Read the image file from the PhysFS archive into a temporary memory buffer
        std::vector<unsigned char> buffer(file_size);
        PHYSFS_readBytes(file, buffer.data(), file_size);
        PHYSFS_close(file);

        // Decode the image from memory using stb_image
        unsigned char* image_data = stbi_load_from_memory(buffer.data(), static_cast<int>(file_size), &img.width, &img.height, &img.channels, 4);

        if (image_data == nullptr)
        {
            std::cerr << "Resource Manager Error: Failed to decode image " << virtual_path << std::endl;
            return img;
        }

        // Generate an OpenGL texture
        glGenTextures(1, &img.texture_id);
        glBindTexture(GL_TEXTURE_2D, img.texture_id);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Upload image data to the GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

        // Free the decoded image from system RAM now that it's on the GPU
        stbi_image_free(image_data);

        return img;
    }
}
