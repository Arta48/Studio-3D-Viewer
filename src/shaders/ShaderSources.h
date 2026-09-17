/**
 * @file ShaderSources.h
 * @brief GLSL 3.30 Core Profile Cook-Torrance PBR and Floor Grid shader sources.
 */

#pragma once

namespace StudioViewer::Shaders {

    inline const char* VERTEX_SHADER_SRC = R"(#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec4 in_tangent; // [Tx, Ty, Tz, Handedness W]

out vec3 v_view_pos;
out vec3 v_normal;
out vec3 v_tangent;
out vec3 v_bitangent;
out vec2 v_uv;

uniform mat4 u_model_view;
uniform mat4 u_projection;
uniform mat3 u_normal_matrix;

void main() {
    vec4 pos = u_model_view * vec4(in_position, 1.0);
    v_view_pos = pos.xyz;

    v_normal = normalize(u_normal_matrix * in_normal);

    // Tangent orthogonalization with fallback
    vec3 t_raw = u_normal_matrix * in_tangent.xyz;
    if (dot(t_raw, t_raw) < 0.0001) {
        t_raw = abs(v_normal.y) < 0.99 ? cross(v_normal, vec3(0.0, 1.0, 0.0)) : cross(v_normal, vec3(1.0, 0.0, 0.0));
    }
    vec3 t = normalize(t_raw);
    t = normalize(t - dot(t, v_normal) * v_normal);

    v_tangent = t;
    v_bitangent = cross(v_normal, t) * in_tangent.w;
    v_uv = in_texcoord;

    gl_Position = u_projection * pos;
}
)";

inline const char* FRAGMENT_SHADER_SRC = R"(#version 330 core

in vec3 v_view_pos;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_uv;

out vec4 frag_color;

// Texture Samplers
uniform sampler2D u_diffuse_map;
uniform sampler2D u_normal_map;
uniform sampler2D u_mr_map;
uniform sampler2D u_occlusion_map;
uniform sampler2D u_emissive_map;

// Uniform Flags & Modes
uniform bool u_has_diffuse;
uniform bool u_has_normal;
uniform bool u_has_mr;
uniform bool u_has_occlusion;
uniform bool u_has_emissive;
uniform vec3 u_emissive_factor;
uniform bool u_enable_textures;
uniform bool u_enable_normals;
uniform bool u_smooth_shading;
uniform bool u_clay_mode;
uniform bool u_wireframe_mode;

// Material Parameters
uniform vec4 u_base_color;
uniform float u_roughness;
uniform float u_metallic;
uniform float u_alpha_cutoff;

const float PI = 3.14159265359;

// Studio 3-Point Light Rig (View Space)
const vec3 L_KEY_DIR  = normalize(vec3(0.85, 1.20, 1.00));
const vec3 L_KEY_COL  = vec3(1.0, 0.98, 0.95) * 2.8;

const vec3 L_FILL_DIR = normalize(vec3(-1.00, 0.50, 0.80));
const vec3 L_FILL_COL = vec3(0.75, 0.85, 1.00) * 1.2;

const vec3 L_BACK_DIR = normalize(vec3(0.00, 1.20, -1.00));
const vec3 L_BACK_COL = vec3(0.85, 0.92, 1.00) * 0.8;

// Hemisphere Studio Ambient Colors
const vec3 SKY_COL    = vec3(0.30, 0.32, 0.38);
const vec3 GROUND_COL = vec3(0.12, 0.13, 0.15);

// Normal Distribution Function: GGX / Trowbridge-Reitz
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 0.00001);
}

// Geometric Shadowing: Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.00001);
}

// Combined Smith Geometric Shadowing
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick with Roughness Attenuation
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Cook-Torrance Microfacet Evaluation
vec3 EvaluatePBRPointLight(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlickRoughness(max(dot(H, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// Filmic ACES Tone Mapping Curve
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    if (u_wireframe_mode) {
        frag_color = vec4(0.12, 0.14, 0.18, 0.85);
        return;
    }

    vec4 albedo = u_base_color;

    // Diffuse map sampling with hardware sRGB to Linear conversion
    if (u_has_diffuse && u_enable_textures && !u_clay_mode) {
        vec4 tex_col = texture(u_diffuse_map, v_uv);
        tex_col.rgb = pow(tex_col.rgb, vec3(2.2));
        albedo = tex_col * u_base_color;
    }

    // Alpha Cutoff test (protects transparent glass in BLEND mode)
    if (u_alpha_cutoff > 0.0001 && albedo.a < u_alpha_cutoff) {
        discard;
    }

    float roughness = clamp(u_roughness, 0.04, 1.0);
    float metallic  = clamp(u_metallic, 0.0, 1.0);

    // glTF Metallic-Roughness map (Green = Roughness, Blue = Metallic)
    if (u_has_mr && u_enable_textures && !u_clay_mode) {
        vec4 mr_sample = texture(u_mr_map, v_uv);
        roughness = clamp(roughness * mr_sample.g, 0.04, 1.0);
        metallic  = clamp(metallic  * mr_sample.b, 0.0, 1.0);
    }

    // Studio Clay Inspection Mode Override
    if (u_clay_mode) {
        albedo.rgb = pow(vec3(0.68, 0.70, 0.74), vec3(2.2));
        roughness = 0.45;
        metallic = 0.0;
    }

    // Surface Normal Resolution
    vec3 N;
    if (!u_smooth_shading) {
        // Flat shading via hardware screen-space derivatives
        N = normalize(cross(dFdx(v_view_pos), dFdy(v_view_pos)));
    } else {
        N = normalize(v_normal);
        if (u_has_normal && u_enable_normals && !u_clay_mode) {
            vec3 n_map = texture(u_normal_map, v_uv).rgb * 2.0 - 1.0;
            n_map.xy *= 0.45;
            n_map = normalize(n_map);
            if (length(v_tangent) > 0.1 && length(v_bitangent) > 0.1) {
                mat3 TBN = mat3(normalize(v_tangent), normalize(v_bitangent), N);
                N = normalize(TBN * n_map);
            }
        }
    }

    vec3 V = normalize(-v_view_pos);
    float NdotV = max(dot(N, V), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    // Direct Studio Lights (Key, Fill, Back)
    vec3 Lo = vec3(0.0);
    Lo += EvaluatePBRPointLight(N, V, L_KEY_DIR,  L_KEY_COL,  albedo.rgb, roughness, metallic, F0);
    Lo += EvaluatePBRPointLight(N, V, L_FILL_DIR, L_FILL_COL, albedo.rgb, roughness, metallic, F0);
    Lo += EvaluatePBRPointLight(N, V, L_BACK_DIR, L_BACK_COL, albedo.rgb, roughness, metallic, F0);

    // Ambient Diffuse
    vec3 ambient_dir = mix(GROUND_COL, SKY_COL, N.y * 0.5 + 0.5);
    vec3 kD_ambient = (vec3(1.0) - F0) * (1.0 - metallic);
    vec3 ambient_diffuse = kD_ambient * albedo.rgb * ambient_dir;

    // Ambient Specular Reflection with Horizon Fade
    vec3 ambient_specular = vec3(0.0);
    if (NdotV > 0.0) {
        vec3 R = reflect(-V, N);
        vec3 env_reflection = mix(GROUND_COL, SKY_COL, R.y * 0.5 + 0.5) * 1.5;
        vec3 kS_ambient = FresnelSchlickRoughness(NdotV, F0, roughness);
        float spec_ambient_factor = (1.0 - roughness) * (1.0 - roughness);
        float horizon_fade = smoothstep(0.0, 0.08, NdotV);
        ambient_specular = kS_ambient * env_reflection * spec_ambient_factor * horizon_fade;
    }

    // Ambient Occlusion
    float ao = 1.0;
    if (u_has_occlusion && u_enable_textures && !u_clay_mode) {
        ao = texture(u_occlusion_map, v_uv).r;
    }
    vec3 ambient = (ambient_diffuse + ambient_specular) * ao;

    // Emissive Radiance
    vec3 emissive = vec3(0.0);
    if (u_enable_textures && !u_clay_mode) {
        emissive = u_emissive_factor;
        if (u_has_emissive) {
            vec3 emissive_tex = texture(u_emissive_map, v_uv).rgb;
            emissive_tex = pow(emissive_tex, vec3(2.2));
            emissive *= emissive_tex;
        }
    }

    vec3 color = Lo + ambient + emissive;

    // Filmic ACES Tone Mapping & Hardware Display Gamma (1.0 / 2.2)
    vec3 mapped = ACESFilm(color);
    frag_color = vec4(pow(mapped, vec3(1.0 / 2.2)), albedo.a);
}
)";

inline const char* GRID_VERTEX_SRC = R"(#version 330 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
uniform mat4 u_mvp;
out vec3 v_color;
void main() {
    v_color = in_color;
    gl_Position = u_mvp * vec4(in_position, 1.0);
}
)";

inline const char* GRID_FRAGMENT_SRC = R"(#version 330 core
in vec3 v_color;
out vec4 frag_color;
void main() {
    frag_color = vec4(v_color, 1.0);
}
)";

} // namespace StudioViewer::Shaders
