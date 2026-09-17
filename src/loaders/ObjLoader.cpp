#include "loaders/ObjLoader.h"
#include "loaders/Texture.h"
#include "core/Constants.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <QFileInfo>
#include <QDir>

namespace StudioViewer {

    struct ObjFaceCorner {
        int v{-1};
        int vt{-1};
        int vn{-1};

        bool operator==(const ObjFaceCorner& o) const {
            return v == o.v && vt == o.vt && vn == o.vn;
        }
    };

    struct CornerHasher {
        size_t operator()(const ObjFaceCorner& c) const {
            return std::hash<int>()(c.v) ^ (std::hash<int>()(c.vt) << 1) ^ (std::hash<int>()(c.vn) << 2);
        }
    };

    static int parseObjIndex(const std::string& tok, int count) {
        if (tok.empty()) return -1;
        try {
            int val = std::stoi(tok);
            int idx = (val > 0) ? (val - 1) : (count + val);
            return (idx >= 0 && idx < count) ? idx : -1;
        } catch (...) {
            return -1;
        }
    }

    ModelData ObjLoader::load(const std::string& filepath) {
        ModelData model;
        QFileInfo fi(QString::fromStdString(filepath));
        model.name = fi.fileName().toStdString();
        std::string baseDir = fi.dir().path().toStdString();

        std::vector<Math::Vec3> rawV;
        std::vector<Math::Vec2> rawVt;
        std::vector<Math::Vec3> rawVn;
        std::unordered_map<std::string, std::vector<std::array<ObjFaceCorner, 3>>> matFaces;
        std::vector<std::string> mtlFiles;
        std::string currentMat = "__default__";

        // Streaming line-by-line ingestion avoids allocating multi-gigabyte string buffers
        std::ifstream file(filepath, std::ios::in);
        if (!file.is_open()) return model;

        std::string line;
        while (std::getline(file, line)) {
            size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos || line[start] == '#') continue;

            const char* ptr = line.c_str() + start;

            if (ptr[0] == 'v' && ptr[1] == ' ') {
                std::istringstream ss(ptr + 2);
                Math::Vec3 v;
                if (ss >> v.x >> v.y >> v.z) rawV.push_back(v);
            } else if (ptr[0] == 'v' && ptr[1] == 't' && ptr[2] == ' ') {
                std::istringstream ss(ptr + 3);
                Math::Vec2 vt;
                if (ss >> vt.x >> vt.y) rawVt.push_back(vt);
            } else if (ptr[0] == 'v' && ptr[1] == 'n' && ptr[2] == ' ') {
                std::istringstream ss(ptr + 3);
                Math::Vec3 vn;
                if (ss >> vn.x >> vn.y >> vn.z) rawVn.push_back(vn);
            } else if (std::strncmp(ptr, "usemtl ", 7) == 0) {
                std::string mat = ptr + 7;
                size_t end = mat.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) mat = mat.substr(0, end + 1);
                currentMat = mat;
            } else if (std::strncmp(ptr, "mtllib ", 7) == 0) {
                std::string mtl = ptr + 7;
                size_t end = mtl.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) mtl = mtl.substr(0, end + 1);
                mtlFiles.push_back(mtl);
            } else if (ptr[0] == 'f' && ptr[1] == ' ') {
                std::istringstream ss(ptr + 2);
                std::string token;
                std::vector<ObjFaceCorner> poly;

                int lenV = static_cast<int>(rawV.size());
                int lenVt = static_cast<int>(rawVt.size());
                int lenVn = static_cast<int>(rawVn.size());

                while (ss >> token) {
                    ObjFaceCorner c;
                    size_t p1 = token.find('/');
                    if (p1 == std::string::npos) {
                        c.v = parseObjIndex(token, lenV);
                    } else {
                        c.v = parseObjIndex(token.substr(0, p1), lenV);
                        size_t p2 = token.find('/', p1 + 1);
                        if (p2 == std::string::npos) {
                            c.vt = parseObjIndex(token.substr(p1 + 1), lenVt);
                        } else {
                            if (p2 > p1 + 1) {
                                c.vt = parseObjIndex(token.substr(p1 + 1, p2 - p1 - 1), lenVt);
                            }
                            if (p2 + 1 < token.size()) {
                                c.vn = parseObjIndex(token.substr(p2 + 1), lenVn);
                            }
                        }
                    }
                    if (c.v >= 0) poly.push_back(c);
                }

                // Fan triangulation for arbitrary n-gons: (0, i, i + 1)
                if (poly.size() >= 3) {
                    for (size_t i = 1; i < poly.size() - 1; ++i) {
                        matFaces[currentMat].push_back({poly[0], poly[i], poly[i + 1]});
                    }
                }
            }
        }
        file.close();

        if (rawV.empty()) return model;

        // Parse Wavefront Material Libraries (.mtl)
        struct MtlProps {
            std::array<float, 4> Kd{0.70f, 0.72f, 0.75f, 1.0f};
            float roughness{DEFAULT_ROUGHNESS};
            float metallic{DEFAULT_METALLIC};
            std::string mapKd;
            std::string mapBump;
        };
        std::unordered_map<std::string, MtlProps> materials;

        for (const auto& mtlName : mtlFiles) {
            std::string mtlPath = TextureLoader::resolveLocalTexture(baseDir, mtlName);
            if (mtlPath.empty()) continue;

            std::ifstream mf(mtlPath);
            if (!mf.is_open()) continue;

            std::string mline;
            MtlProps* currMtl = nullptr;

            while (std::getline(mf, mline)) {
                size_t mstart = mline.find_first_not_of(" \t\r\n");
                if (mstart == std::string::npos || mline[mstart] == '#') continue;

                std::istringstream mss(mline.substr(mstart));
                std::string key;
                mss >> key;
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                if (key == "newmtl") {
                    std::string name;
                    mss >> name;
                    materials[name] = MtlProps();
                    currMtl = &materials[name];
                } else if (currMtl) {
                    if (key == "kd") {
                        mss >> currMtl->Kd[0] >> currMtl->Kd[1] >> currMtl->Kd[2];
                    } else if (key == "d" || key == "tr") {
                        float val;
                        if (mss >> val) currMtl->Kd[3] = (key == "tr") ? (1.0f - val) : val;
                    } else if (key == "pr") {
                        mss >> currMtl->roughness;
                    } else if (key == "pm") {
                        mss >> currMtl->metallic;
                    } else if (key == "ns") {
                        float ns;
                        if (mss >> ns) {
                            currMtl->roughness = std::clamp(1.0f - std::sqrt(ns / 1000.0f), 0.04f, 1.0f);
                        }
                    } else if (key == "map_kd") {
                        std::string p;
                        mss >> p;
                        currMtl->mapKd = TextureLoader::resolveLocalTexture(baseDir, p);
                    } else if (key == "bump" || key == "map_bump" || key == "norm" || key == "map_kn") {
                        std::string p;
                        mss >> p;
                        currMtl->mapBump = TextureLoader::resolveLocalTexture(baseDir, p);
                    }
                }
            }
        }

        // Local directory fallback if MTL omitted maps or file lacks material libraries
        bool hasAnyTexture = false;
        for (const auto& [name, m] : materials) {
            if (!m.mapKd.empty()) {
                hasAnyTexture = true;
                break;
            }
        }

        if (!hasAnyTexture) {
            QDir dir(QString::fromStdString(baseDir));
            QFileInfoList candidates = dir.entryInfoList({"*.png", "*.jpg", "*.jpeg", "*.PNG", "*.JPG", "*.JPEG"}, QDir::Files);
            for (const auto& c : candidates) {
                QString lower = c.fileName().toLower();
                if (!lower.contains("preview") && !lower.contains("normal") && !lower.contains("bump")) {
                    materials["__default__"].mapKd = c.absoluteFilePath().toStdString();
                    break;
                }
            }
        }

        // Compute bounding sphere and center scene coordinates
        Math::Vec3 minB = rawV[0], maxB = rawV[0];
        for (const auto& p : rawV) {
            minB.x = std::min(minB.x, p.x); minB.y = std::min(minB.y, p.y); minB.z = std::min(minB.z, p.z);
            maxB.x = std::max(maxB.x, p.x); maxB.y = std::max(maxB.y, p.y); maxB.z = std::max(maxB.z, p.z);
        }
        model.center = (minB + maxB) * 0.5f;
        float maxSpan = std::max({maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z});
        model.radius = std::max(maxSpan * 0.5f, 0.001f);
        float scale = 1.0f / model.radius;

        // Generate smooth vertex normals if file lacked vn tokens
        bool hasVn = !rawVn.empty();
        if (!hasVn) {
            std::vector<uint32_t> allIndices;
            for (auto& [name, tris] : matFaces) {
                for (auto& t : tris) {
                    allIndices.push_back(static_cast<uint32_t>(t[0].v));
                    allIndices.push_back(static_cast<uint32_t>(t[1].v));
                    allIndices.push_back(static_cast<uint32_t>(t[2].v));
                }
            }
            rawVn = Math::computeSmoothNormals(rawV, allIndices);
            hasVn = true;
        }

        bool hasVt = !rawVt.empty();

        for (auto& [matName, tris] : matFaces) {
            if (tris.empty()) continue;

            RenderBatch batch;
            std::unordered_map<ObjFaceCorner, uint32_t, CornerHasher> uniqueMap;
            std::vector<Math::Vec3> bVerts;
            std::vector<Math::Vec3> bNorms;
            std::vector<Math::Vec2> bUvs;

            for (const auto& t : tris) {
                for (const auto& corner : t) {
                    auto it = uniqueMap.find(corner);
                    if (it == uniqueMap.end()) {
                        uint32_t idx = static_cast<uint32_t>(uniqueMap.size());
                        uniqueMap[corner] = idx;

                        Math::Vec3 pos = (rawV[corner.v] - model.center) * scale;
                        bVerts.push_back(pos);

                        if (hasVt && corner.vt >= 0 && corner.vt < static_cast<int>(rawVt.size())) {
                            bUvs.push_back(rawVt[corner.vt]);
                        } else {
                            bUvs.push_back({0.0f, 0.0f});
                        }

                        if (hasVn) {
                            if (corner.vn >= 0 && corner.vn < static_cast<int>(rawVn.size())) {
                                bNorms.push_back(rawVn[corner.vn]);
                            } else {
                                bNorms.push_back(rawVn[corner.v]);
                            }
                        } else {
                            bNorms.push_back({0.0f, 1.0f, 0.0f});
                        }

                        batch.indices.push_back(idx);
                    } else {
                        batch.indices.push_back(it->second);
                    }
                }
            }

            std::vector<Math::Vec4> tangents = Math::computeTangents(bVerts, bNorms, bUvs, batch.indices);

            Math::Vec3 centerAccum{0.0f, 0.0f, 0.0f};
            batch.vertices.resize(bVerts.size());
            for (size_t i = 0; i < bVerts.size(); ++i) {
                centerAccum += bVerts[i];
                batch.vertices[i] = {
                    bVerts[i].x, bVerts[i].y, bVerts[i].z,
                    bNorms[i].x, bNorms[i].y, bNorms[i].z,
                    bUvs[i].x, bUvs[i].y,
                    tangents[i].x, tangents[i].y, tangents[i].z, tangents[i].w
                };
            }
            if (!bVerts.empty()) batch.centroid = centerAccum * (1.0f / bVerts.size());

            auto mIt = materials.find(matName);
            if (mIt != materials.end()) {
                batch.baseColor = mIt->second.Kd;
                batch.roughness = mIt->second.roughness;
                batch.metallic = mIt->second.metallic;
                batch.diffusePath = mIt->second.mapKd;
                batch.normalPath = mIt->second.mapBump;
            } else if (materials.count("__default__")) {
                const auto& defMtl = materials["__default__"];
                batch.baseColor = defMtl.Kd;
                batch.roughness = defMtl.roughness;
                batch.metallic = defMtl.metallic;
                batch.diffusePath = defMtl.mapKd;
                batch.normalPath = defMtl.mapBump;
            }

            if (batch.diffusePath.empty() && materials.count("__default__")) {
                batch.diffusePath = materials["__default__"].mapKd;
            }

            batch.diffuseKey = batch.diffusePath;
            batch.normalKey = batch.normalPath;

            batch.isTransparent = (batch.baseColor[3] < 0.99f);
            if (batch.isTransparent) {
                batch.alphaCutoff = 0.0f; // Transparent batches are never discarded
                if (batch.baseColor[3] < 0.05f) {
                    batch.baseColor[3] = 0.25f;
                }
            } else {
                batch.alphaCutoff = DEFAULT_ALPHA_CUTOFF;
            }

            batch.triCount = tris.size();
            batch.indexCount = batch.indices.size();

            model.batches.push_back(std::move(batch));
            model.numTriangles += tris.size();
            model.numVertices += bVerts.size();
        }

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
