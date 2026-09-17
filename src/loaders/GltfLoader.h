/**
 * @file GltfLoader.h
 * @brief Modern glTF 2.0 and GLB binary parser with scene graph flattening.
 */

#pragma once

#include "core/Models.h"
#include <string>

namespace StudioViewer {

    class GltfLoader {
    public:
        static ModelData load(const std::string& filepath);
    };

} // namespace StudioViewer
