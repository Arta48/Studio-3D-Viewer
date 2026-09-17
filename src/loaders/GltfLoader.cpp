#include "loaders/GltfLoader.h"
#include "loaders/Texture.h"
#include "core/Constants.h"
#define CGLTF_IMPLEMENTATION
#include "thirdparty/cgltf.h"
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QByteArray>
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace StudioViewer {

    // Transform surface normals via 3x3 Inverse-Transpose (Cofactor matrix)
    static void transformNormals(cgltf_accessor* normalAcc, const Math::Mat4& m, std::vector<Math::Vec3>& out) {
        if (!normalAcc) return;

        float a = m.m[0], b = m.m[1], c = m.m[2];
        float d = m.m[4], e = m.m[5], f = m.m[6];
        float g = m.m[8], h = m.m[9], k = m.m[10];

        float det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
        float invDet = (std::abs(det) > 1e-8f) ? (1.0f / det) : 1.0f;

        float nm[9];
        nm[0] =  (e * k - f * h) * invDet;
        nm[1] = -(d * k - f * g) * invDet;
        nm[2] =  (d * h - e * g) * invDet;

        nm[3] = -(b * k - c * h) * invDet;
        nm[4] =  (a * k - c * g) * invDet;
        nm[5] = -(a * h - b * g) * invDet;

        nm[6] =  (b * f - c * e) * invDet;
        nm[7] = -(a * f - c * d) * invDet;
        nm[8] =  (a * e - b * d) * invDet;

        // Flip normal direction if reflection/mirror scale is present (negative determinant)
        float sign = (det < 0.0f) ? -1.0f : 1.0f;

        out.reserve(out.size() + normalAcc->count);
        for (size_t i = 0; i < normalAcc->count; ++i) {
            float n[3];
            cgltf_accessor_read_float(normalAcc, i, n, 3);
            Math::Vec3 tn = {
                (nm[0] * n[0] + nm[1] * n[1] + nm[2] * n[2]) * sign,
                (nm[3] * n[0] + nm[4] * n[1] + nm[5] * n[2]) * sign,
                (nm[6] * n[0] + nm[7] * n[1] + nm[8] * n[2]) * sign
            };
            out.push_back(Math::normalize(tn));
        }
    }

    ModelData GltfLoader::load(const std::string& filepath) {
        ModelData model;
        QFileInfo fi(QString::fromStdString(filepath));
        model.name = fi.fileName().toStdString();
        std::string baseDir = fi.dir().path().toStdString();

        cgltf_options options{};
        cgltf_data* data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
        if (result != cgltf_result_success) return model;

        result = cgltf_load_buffers(&options, data, filepath.c_str());
        if (result != cgltf_result_success) {
            cgltf_free(data);
            return model;
        }

        struct RawBatch {
            std::vector<Math::Vec3> verts;
            std::vector<Math::Vec3> norms;
            std::vector<Math::Vec2> uvs;
            std::vector<uint32_t> indices;
            cgltf_material* material{nullptr};
        };
        std::vector<RawBatch> extracted;

        // Recursively flatten glTF scene graph hierarchy
        auto processNode = [&](auto& self, cgltf_node* node, const Math::Mat4& parentMat) -> void {
            float colMajor[16];
            cgltf_node_transform_local(node, colMajor);

            // TRANSPOSE Column-Major (glTF spec) to Row-Major (Math::Mat4)
            Math::Mat4 localMat;
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    localMat.m[r * 4 + c] = colMajor[c * 4 + r];
                }
            }

            Math::Mat4 currentMat = parentMat * localMat;

            if (node->mesh) {
                for (size_t p = 0; p < node->mesh->primitives_count; ++p) {
                    cgltf_primitive& prim = node->mesh->primitives[p];
                    if (prim.type != cgltf_primitive_type_triangles) continue;

                    RawBatch rb;
                    rb.material = prim.material;

                    cgltf_accessor* posAcc = nullptr;
                    cgltf_accessor* normAcc = nullptr;
                    cgltf_accessor* uvAcc = nullptr;

                    for (size_t a = 0; a < prim.attributes_count; ++a) {
                        if (prim.attributes[a].type == cgltf_attribute_type_position) posAcc = prim.attributes[a].data;
                        else if (prim.attributes[a].type == cgltf_attribute_type_normal) normAcc = prim.attributes[a].data;
                        else if (prim.attributes[a].type == cgltf_attribute_type_texcoord) uvAcc = prim.attributes[a].data;
                    }

                    if (!posAcc) continue;

                    rb.verts.resize(posAcc->count);
                    for (size_t k = 0; k < posAcc->count; ++k) {
                        float pOut[3];
                        cgltf_accessor_read_float(posAcc, k, pOut, 3);
                        rb.verts[k] = currentMat.transformPoint({pOut[0], pOut[1], pOut[2]});
                    }

                    if (normAcc) {
                        transformNormals(normAcc, currentMat, rb.norms);
                    }

                    if (uvAcc) {
                        rb.uvs.resize(uvAcc->count);
                        for (size_t k = 0; k < uvAcc->count; ++k) {
                            float uvOut[2];
                            cgltf_accessor_read_float(uvAcc, k, uvOut, 2);
                            // Invert V coordinate (1.0 - V) to match OpenGL texture origin
                            rb.uvs[k] = {uvOut[0], 1.0f - uvOut[1]};
                        }
                    } else {
                        rb.uvs.assign(rb.verts.size(), {0.0f, 0.0f});
                    }

                    if (prim.indices) {
                        rb.indices.resize(prim.indices->count);
                        for (size_t k = 0; k < prim.indices->count; ++k) {
                            rb.indices[k] = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, k));
                        }
                    } else {
                        rb.indices.resize(rb.verts.size());
                        for (size_t k = 0; k < rb.indices.size(); ++k) {
                            rb.indices[k] = static_cast<uint32_t>(k);
                        }
                    }

                    if (rb.norms.empty()) {
                        rb.norms = Math::computeSmoothNormals(rb.verts, rb.indices);
                    }

                    extracted.push_back(std::move(rb));
                }
            }

            for (size_t c = 0; c < node->children_count; ++c) {
                self(self, node->children[c], currentMat);
            }
        };

        Math::Mat4 rootMat = Math::Mat4::identity();
        cgltf_scene* scene = data->scene ? data->scene : (data->scenes_count > 0 ? &data->scenes[0] : nullptr);
        if (scene) {
            for (size_t n = 0; n < scene->nodes_count; ++n) {
                processNode(processNode, scene->nodes[n], rootMat);
            }
        } else {
            for (size_t n = 0; n < data->nodes_count; ++n) {
                if (!data->nodes[n].parent) {
                    processNode(processNode, &data->nodes[n], rootMat);
                }
            }
        }

        if (extracted.empty()) {
            cgltf_free(data);
            return model;
        }

        // Bounding Box calculation
        bool hasVerts = false;
        Math::Vec3 minB{0, 0, 0}, maxB{0, 0, 0};
        for (const auto& rb : extracted) {
            for (const auto& p : rb.verts) {
                if (!hasVerts) {
                    minB = maxB = p;
                    hasVerts = true;
                } else {
                    minB.x = std::min(minB.x, p.x); minB.y = std::min(minB.y, p.y); minB.z = std::min(minB.z, p.z);
                    maxB.x = std::max(maxB.x, p.x); maxB.y = std::max(maxB.y, p.y); maxB.z = std::max(maxB.z, p.z);
                }
            }
        }
        model.center = (minB + maxB) * 0.5f;
        float maxSpan = std::max({maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z});
        model.radius = std::max(maxSpan * 0.5f, 0.001f);
        float scale = 1.0f / model.radius;

        // Texture decompression cache to prevent redundant decoding of identical images
        std::unordered_map<const cgltf_image*, std::shared_ptr<DecodedImage>> embeddedCache;

        auto resolveTexture = [&](const cgltf_texture_view& view, std::string& pathOut, std::string& keyOut, std::shared_ptr<DecodedImage>& decodedOut, GLenum& minF, GLenum& magF) {
            if (!view.texture) return;
            if (view.texture->sampler) {
                if (view.texture->sampler->min_filter) minF = view.texture->sampler->min_filter;
                if (view.texture->sampler->mag_filter) magF = view.texture->sampler->mag_filter;
            }
            cgltf_image* img = view.texture->image;
            if (!img) return;

            if (img->buffer_view && img->buffer_view->buffer && img->buffer_view->buffer->data) {
                keyOut = "embedded_" + std::to_string(reinterpret_cast<uintptr_t>(img));
                if (embeddedCache.count(img)) {
                    decodedOut = embeddedCache[img];
                } else {
                    const uint8_t* ptr = static_cast<const uint8_t*>(img->buffer_view->buffer->data) + img->buffer_view->offset;
                    decodedOut = TextureLoader::decodeMemory(ptr, img->buffer_view->size);
                    embeddedCache[img] = decodedOut;
                }
            } else if (img->uri) {
                QString uriStr = QString::fromUtf8(img->uri);
                if (uriStr.startsWith("data:")) {
                    // Decode base64 embedded data URI
                    int commaIdx = uriStr.indexOf(',');
                    if (commaIdx != -1) {
                        QByteArray binaryData = QByteArray::fromBase64(uriStr.mid(commaIdx + 1).toUtf8());
                        keyOut = "data_uri_" + std::to_string(reinterpret_cast<uintptr_t>(img));
                        decodedOut = TextureLoader::decodeMemory(reinterpret_cast<const uint8_t*>(binaryData.constData()), binaryData.size());
                    }
                } else {
                    // Decode percent-encoded relative URI
                    QString decodedUri = QUrl::fromPercentEncoding(uriStr.toUtf8());
                    pathOut = TextureLoader::resolveLocalTexture(baseDir, decodedUri.toStdString());
                    if (pathOut.empty()) pathOut = TextureLoader::resolveLocalTexture(baseDir, uriStr.toStdString());
                    keyOut = pathOut;
                }
            }
        };

        // Convert raw extracted batches into RenderBatches
        for (auto& rb : extracted) {
            RenderBatch b;
            b.vertices.resize(rb.verts.size());
            b.indices = std::move(rb.indices);
            b.triCount = b.indices.size() / 3;
            b.indexCount = b.indices.size();

            for (auto& p : rb.verts) p = (p - model.center) * scale;
            std::vector<Math::Vec4> tangents = Math::computeTangents(rb.verts, rb.norms, rb.uvs, b.indices);

            Math::Vec3 centerAccum{0.0f, 0.0f, 0.0f};
            for (size_t i = 0; i < rb.verts.size(); ++i) {
                centerAccum += rb.verts[i];
                b.vertices[i] = {
                    rb.verts[i].x, rb.verts[i].y, rb.verts[i].z,
                    rb.norms[i].x, rb.norms[i].y, rb.norms[i].z,
                    rb.uvs[i].x, rb.uvs[i].y,
                    tangents[i].x, tangents[i].y, tangents[i].z, tangents[i].w
                };
            }
            if (!rb.verts.empty()) b.centroid = centerAccum * (1.0f / rb.verts.size());

            if (rb.material) {
                cgltf_material* m = rb.material;
                if (m->has_pbr_metallic_roughness) {
                    auto& pbr = m->pbr_metallic_roughness;
                    b.baseColor = {pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]};
                    b.roughness = pbr.roughness_factor;
                    b.metallic = pbr.metallic_factor;

                    resolveTexture(pbr.base_color_texture, b.diffusePath, b.diffuseKey, b.diffuseDecoded, b.minFilter, b.magFilter);
                    resolveTexture(pbr.metallic_roughness_texture, b.mrPath, b.mrKey, b.mrDecoded, b.minFilter, b.magFilter);
                } else if (m->has_pbr_specular_glossiness) {
                    auto& sg = m->pbr_specular_glossiness;
                    b.baseColor = {sg.diffuse_factor[0], sg.diffuse_factor[1], sg.diffuse_factor[2], sg.diffuse_factor[3]};
                    b.roughness = 1.0f - sg.glossiness_factor;
                    resolveTexture(sg.diffuse_texture, b.diffusePath, b.diffuseKey, b.diffuseDecoded, b.minFilter, b.magFilter);
                }

                GLenum dummyMin = GL_LINEAR_MIPMAP_LINEAR, dummyMag = GL_LINEAR;
                resolveTexture(m->normal_texture, b.normalPath, b.normalKey, b.normalDecoded, dummyMin, dummyMag);
                resolveTexture(m->occlusion_texture, b.occlusionPath, b.occlusionKey, b.occlusionDecoded, dummyMin, dummyMag);
                resolveTexture(m->emissive_texture, b.emissivePath, b.emissiveKey, b.emissiveDecoded, dummyMin, dummyMag);

                b.emissiveFactor = {m->emissive_factor[0], m->emissive_factor[1], m->emissive_factor[2]};

                // Support glTF transmission extension (KHR_materials_transmission)
                bool hasTransmission = (m->has_transmission && m->transmission.transmission_factor > 0.01f);

                // Strict transparency check (matches Python 1:1, prevents opaque interior from bleeding)
                if (hasTransmission) {
                    b.isTransparent = true;
                    float trans = m->transmission.transmission_factor;
                    // Realistic dark automotive glass tint
                    b.baseColor[3] = std::clamp(b.baseColor[3] * (1.0f - trans * 0.65f), 0.35f, 0.55f);
                    b.roughness = std::min(b.roughness, 0.05f);
                    b.metallic = 0.0f;
                    b.alphaCutoff = 0.0f;
                } else if (b.baseColor[3] < 0.99f) {
                    b.isTransparent = true;
                    if (b.baseColor[3] < 0.10f) {
                        b.baseColor[3] = 0.35f;
                    }
                    b.alphaCutoff = 0.0f;
                } else {
                    b.isTransparent = false;
                    b.baseColor[3] = 1.0f;
                    b.alphaCutoff = (m->alpha_mode == cgltf_alpha_mode_mask) ? m->alpha_cutoff : DEFAULT_ALPHA_CUTOFF;
                }
            }

            model.batches.push_back(std::move(b));
            model.numTriangles += b.triCount;
            model.numVertices += rb.verts.size();
        }

        cgltf_free(data);

        // Count unique textures & normal maps
        std::unordered_map<std::string, bool> uTex, uNorm;
        for (const auto& b : model.batches) {
            if (!b.diffuseKey.empty()) uTex[b.diffuseKey] = true;
            if (!b.normalKey.empty()) uNorm[b.normalKey] = true;
        }
        model.numTextures = uTex.size();
        model.numNormals = uNorm.size();

        return model;
    }

} // namespace StudioViewer
