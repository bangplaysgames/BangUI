#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

#include <filesystem>
#include <iostream>

namespace BangUI::Standalone {

bool LoadImageRGBA(const std::string& path, int& width, int& height, std::vector<uint8_t>& pixels) {
    width = 0;
    height = 0;
    pixels.clear();

    if (path.empty()) return false;

    std::string resolved = path;
    if (!std::filesystem::exists(resolved)) {
        std::filesystem::path attempt = std::filesystem::current_path() / path;
        if (std::filesystem::exists(attempt)) {
            resolved = attempt.string();
        }
    }

    int comp = 0;
    unsigned char* data = stbi_load(resolved.c_str(), &width, &height, &comp, 4);
    if (!data) {
        std::cerr << "Failed to load image: " << path << std::endl;
        return false;
    }

    pixels.assign(data, data + width * height * 4);
    stbi_image_free(data);
    return true;
}

bool ProbeImageSize(const std::string& path, int& width, int& height) {
    width = 0;
    height = 0;
    if (path.empty()) return false;

    std::string resolved = path;
    if (!std::filesystem::exists(resolved)) {
        std::filesystem::path attempt = std::filesystem::current_path() / path;
        if (std::filesystem::exists(attempt)) {
            resolved = attempt.string();
        }
    }

    int comp = 0;
    if (stbi_info(resolved.c_str(), &width, &height, &comp) == 0) {
        return false;
    }
    return true;
}

} // namespace BangUI::Standalone
