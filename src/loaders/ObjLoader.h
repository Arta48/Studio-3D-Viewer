/**
 * @file ObjLoader.h
 * @brief High-performance streaming Wavefront OBJ & MTL file ingestion engine.
 */

#pragma once

#include "core/Models.h"
#include <string>

namespace StudioViewer {

    class ObjLoader {
    public:
        static ModelData load(const std::string& filepath);
    };

} // namespace StudioViewer
