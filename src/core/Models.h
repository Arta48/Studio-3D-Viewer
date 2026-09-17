/**
 * @file Models.h
 * @brief Strongly typed dataclasses representing GPU render batches and 3D scenes.
 */

#pragma once

#include "core/Math3D.h"
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <QOpenGLFunctions_3_3_Core>

namespace StudioViewer {

    /**
     * @brief Contiguous 48-byte aligned vertex structure matching interleaved VBO layout.
     */
    struct alignas(16) Vertex {
        float px, py, pz;     // 12 Bytes: Model-space Position
        float nx, ny, nz;     // 12 Bytes: Normal vector
        float u, v;           //  8 Bytes: UV texture coordinates
        float tx, ty, tz, tw; // 16 Bytes: Tangent vector + Handedness sign W
    };
    static_assert(sizeof(Vertex) == 48, "Vertex layout must strictly equal 48 bytes");

    /**
     * @brief In-memory raw RGBA decoded bitmap staged for GPU upload.
     */
    struct DecodedImage {
        int width{0};
        int height{0};
        std::vector<uint8_t> rgba;
    };

    /**
     * @brief Represents an independently drawable geometry batch with PBR properties.
     */
    struct RenderBatch {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Math::Vec3 centroid{0.0f, 0.0f, 0.0f}; // Used for Back-to-Front sorting

        // Texture file references
        std::string diffusePath;
        std::string normalPath;
        std::string mrPath;
        std::string occlusionPath;
        std::string emissivePath;

        // Deduplication cache keys
        std::string diffuseKey;
        std::string normalKey;
        std::string mrKey;
        std::string occlusionKey;
        std::string emissiveKey;

        // Physical PBR coefficients
        std::array<float, 4> baseColor{0.70f, 0.72f, 0.75f, 1.0f};
        std::array<float, 3> emissiveFactor{0.0f, 0.0f, 0.0f};
        float roughness{1.0f};
        float metallic{0.0f};

        // Texture filtering preferences (from glTF samplers specification)
        GLenum minFilter{GL_LINEAR_MIPMAP_LINEAR};
        GLenum magFilter{GL_LINEAR};

        size_t triCount{0};
        bool isTransparent{false};
        float alphaCutoff{0.001f};

        // Staged CPU image buffers
        std::shared_ptr<DecodedImage> diffuseDecoded;
        std::shared_ptr<DecodedImage> normalDecoded;
        std::shared_ptr<DecodedImage> mrDecoded;
        std::shared_ptr<DecodedImage> occlusionDecoded;
        std::shared_ptr<DecodedImage> emissiveDecoded;

        // Allocated OpenGL texture IDs
        GLuint diffTexId{0};
        GLuint normTexId{0};
        GLuint mrTexId{0};
        GLuint occlusionTexId{0};
        GLuint emissiveTexId{0};

        // GPU buffer object handles
        GLuint vao{0};
        GLuint vbo{0};
        GLuint ebo{0};
        size_t indexCount{0};
    };

    /**
     * @brief Encapsulates a complete loaded 3D asset.
     */
    struct ModelData {
        std::string name;
        std::vector<RenderBatch> batches;
        Math::Vec3 center{0.0f, 0.0f, 0.0f};
        float radius{1.0f};
        size_t numVertices{0};
        size_t numTriangles{0};
        size_t numTextures{0};
        size_t numNormals{0};
        double loadTime{0.0};
    };

} // namespace StudioViewer
