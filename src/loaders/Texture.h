/**
 * @file Texture.h
 * @brief Image decoding and path resolution utilities supporting stb_image and QImage.
 */

#pragma once

#include "core/Models.h"
#include <string>
#include <memory>

namespace StudioViewer {

    class TextureLoader {
    public:
        static std::string resolveLocalTexture(const std::string& baseDir, const std::string& filename);
        static std::shared_ptr<DecodedImage> decodeImage(const std::string& filepath);
        static std::shared_ptr<DecodedImage> decodeMemory(const uint8_t* buffer, size_t len);
    };

} // namespace StudioViewer
