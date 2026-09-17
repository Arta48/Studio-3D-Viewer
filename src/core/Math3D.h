/**
 * @file Math3D.h
 * @brief Vectorized 3D linear algebra, transformation matrices, and tangent generators.
 */

#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <cstring>
#include <cstdint>

namespace StudioViewer::Math {

    constexpr float PI = 3.14159265358979323846f;

    inline float toRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    struct Vec2 {
        float x{0.0f};
        float y{0.0f};
    };

    struct Vec3 {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};

        Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
        Vec3 operator*(float s) const       { return {x * s, y * s, z * s}; }
        Vec3& operator+=(const Vec3& o)     { x += o.x; y += o.y; z += o.z; return *this; }
        Vec3& operator-=(const Vec3& o)     { x -= o.x; y -= o.y; z -= o.z; return *this; }
        Vec3& operator/=(float s)           { x /= s; y /= s; z /= s; return *this; }
    };

    struct Vec4 {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        float w{1.0f};
    };

    inline float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline float length(const Vec3& v) {
        return std::sqrt(dot(v, v));
    }

    inline Vec3 normalize(const Vec3& v) {
        float len = length(v);
        return (len < 1e-8f) ? Vec3{0.0f, 1.0f, 0.0f} : v * (1.0f / len);
    }

    /**
     * @brief 4x4 homogenous transformation matrix (Row-Major memory layout).
     * Passed to OpenGL uniforms with transpose flag set to GL_TRUE.
     */
    struct Mat4 {
        float m[16]{0.0f};

        static Mat4 identity() {
            Mat4 r;
            r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
            return r;
        }

        Mat4 operator*(const Mat4& o) const {
            Mat4 res;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    float sum = 0.0f;
                    for (int k = 0; k < 4; ++k) {
                        sum += m[row * 4 + k] * o.m[k * 4 + col];
                    }
                    res.m[row * 4 + col] = sum;
                }
            }
            return res;
        }

        Vec3 transformPoint(const Vec3& p) const {
            return {
                m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3],
                m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
                m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11]
            };
        }
    };

    inline Mat4 perspective(float fovy_rad, float aspect, float near_z, float far_z) {
        float f = 1.0f / std::tan(fovy_rad / 2.0f);
        Mat4 res;
        res.m[0] = f / aspect;
        res.m[5] = f;
        res.m[10] = (far_z + near_z) / (near_z - far_z);
        res.m[11] = (2.0f * far_z * near_z) / (near_z - far_z);
        res.m[14] = -1.0f;
        return res;
    }

    inline Mat4 translation(float x, float y, float z) {
        Mat4 r = Mat4::identity();
        r.m[3] = x;
        r.m[7] = y;
        r.m[11] = z;
        return r;
    }

    inline Mat4 rotationX(float rad) {
        Mat4 r = Mat4::identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c;  r.m[6] = -s;
        r.m[9] = s;  r.m[10] = c;
        return r;
    }

    inline Mat4 rotationY(float rad) {
        Mat4 r = Mat4::identity();
        float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;   r.m[2] = s;
        r.m[8] = -s;  r.m[10] = c;
        return r;
    }

    /**
     * @brief Accumulates area-weighted vertex normals vectorially across polygon faces.
     */
    inline std::vector<Vec3> computeSmoothNormals(const std::vector<Vec3>& verts, const std::vector<uint32_t>& faces) {
        std::vector<Vec3> norms(verts.size(), Vec3{0.0f, 0.0f, 0.0f});
        size_t numFaces = faces.size() / 3;

        for (size_t i = 0; i < numFaces; ++i) {
            uint32_t i0 = faces[i * 3 + 0];
            uint32_t i1 = faces[i * 3 + 1];
            uint32_t i2 = faces[i * 3 + 2];
            if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;

            // Cross-product magnitude is naturally proportional to triangle surface area
            Vec3 fn = cross(verts[i1] - verts[i0], verts[i2] - verts[i0]);
            norms[i0] += fn;
            norms[i1] += fn;
            norms[i2] += fn;
        }

        for (auto& n : norms) {
            n = normalize(n);
        }
        return norms;
    }

    /**
     * @brief Derives 4D tangent vectors with handedness sign (w = ±1.0) via Gram-Schmidt.
     */
    inline std::vector<Vec4> computeTangents(const std::vector<Vec3>& verts,
                                             const std::vector<Vec3>& norms,
                                             const std::vector<Vec2>& uvs,
                                             const std::vector<uint32_t>& indices) {
        size_t numVerts = verts.size();
        if (numVerts == 0 || uvs.empty() || indices.empty()) {
            return std::vector<Vec4>(numVerts, {1.0f, 0.0f, 0.0f, 1.0f});
        }

        std::vector<Vec3> tangents(numVerts, {0.0f, 0.0f, 0.0f});
        std::vector<Vec3> bitangents(numVerts, {0.0f, 0.0f, 0.0f});
        size_t numTris = indices.size() / 3;

        for (size_t i = 0; i < numTris; ++i) {
            uint32_t i0 = indices[i * 3 + 0];
            uint32_t i1 = indices[i * 3 + 1];
            uint32_t i2 = indices[i * 3 + 2];

            const Vec3& v0 = verts[i0]; const Vec3& v1 = verts[i1]; const Vec3& v2 = verts[i2];
            const Vec2& uv0 = uvs[i0];  const Vec2& uv1 = uvs[i1];  const Vec2& uv2 = uvs[i2];

            Vec3 edge1 = v1 - v0;
            Vec3 edge2 = v2 - v0;
            float du1 = uv1.x - uv0.x; float dv1 = uv1.y - uv0.y;
            float du2 = uv2.x - uv0.x; float dv2 = uv2.y - uv0.y;

            float det = du1 * dv2 - du2 * dv1;
            float invDet = (std::abs(det) < 1e-8f) ? 1.0f : (1.0f / det);

            Vec3 tanTri = (edge1 * dv2 - edge2 * dv1) * invDet;
            Vec3 bitanTri = (edge2 * du1 - edge1 * du2) * invDet;

            tangents[i0] += tanTri; tangents[i1] += tanTri; tangents[i2] += tanTri;
            bitangents[i0] += bitanTri; bitangents[i1] += bitanTri; bitangents[i2] += bitanTri;
        }

        std::vector<Vec4> out(numVerts);
        for (size_t i = 0; i < numVerts; ++i) {
            const Vec3& n = norms[i];
            const Vec3& t = tangents[i];
            const Vec3& b = bitangents[i];

            // Gram-Schmidt orthogonalization: T' = normalize(T - N * dot(N, T))
            Vec3 tOrtho = normalize(t - n * dot(n, t));
            // Calculate bitangent handedness sign: w = sign(dot(cross(N, T), B))
            float w = (dot(cross(n, tOrtho), b) < 0.0f) ? -1.0f : 1.0f;
            out[i] = {tOrtho.x, tOrtho.y, tOrtho.z, w};
        }
        return out;
    }

} // namespace StudioViewer::Math
