/**
 * @file Constants.h
 * @brief Engine-wide execution constants, PBR defaults, and vertex buffer layout.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <thread>
#include <algorithm>

namespace StudioViewer {

    /**
     * @brief Returns hardware concurrency limit bounded to available CPU cores.
     */
    inline unsigned int getMaxWorkers() {
        unsigned int cores = std::thread::hardware_concurrency();
        return (cores == 0) ? 4 : cores;
    }

    // Fallback PBR material attributes (applied when meshes lack definitions)
    constexpr float DEFAULT_BASE_COLOR[4] = {0.70f, 0.72f, 0.75f, 1.0f}; // Daylight neutral gray
    constexpr float DEFAULT_ROUGHNESS = 1.00f;                          // Fully matte surface
    constexpr float DEFAULT_METALLIC = 0.00f;                           // Pure dielectric (F0 = 0.04)
    constexpr float DEFAULT_EMISSIVE_FACTOR[3] = {0.0f, 0.0f, 0.0f};    // Zero self-illumination
    constexpr float DEFAULT_ALPHA_CUTOFF = 0.001f;                      // Alpha test threshold

    // ============================================================================
    // CONTIGUOUS INTERLEAVED VERTEX BUFFER SPECIFICATION (48 Bytes Stride)
    // ============================================================================
    // Layout:
    //   Offset  0 (12B): Position (vec3: X, Y, Z)
    //   Offset 12 (12B): Normal   (vec3: X, Y, Z)
    //   Offset 24  (8B): TexCoord (vec2: U, V)
    //   Offset 32 (16B): Tangent  (vec4: X, Y, Z, Handedness W)
    // ============================================================================
    constexpr size_t VERTEX_STRIDE_BYTES = 48;
    constexpr size_t OFFSET_POSITION = 0;
    constexpr size_t OFFSET_NORMAL = 12;
    constexpr size_t OFFSET_TEXCOORD = 24;
    constexpr size_t OFFSET_TANGENT = 32;

} // namespace StudioViewer
