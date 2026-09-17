#include "gl/StudioViewport.h"
#include "shaders/ShaderSources.h"
#include "core/Constants.h"
#include <QCursor>
#include <QElapsedTimer>
#include <QDateTime>
#include <iostream>

namespace StudioViewer {

    StudioViewport::StudioViewport(QWidget* parent) : QOpenGLWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);

        m_flyTimer = new QTimer(this);
        m_flyTimer->setInterval(16);
        connect(m_flyTimer, &QTimer::timeout, this, &StudioViewport::updateFlyMovement);
    }

    StudioViewport::~StudioViewport() {
        cleanupGL();
    }

    void StudioViewport::initializeGL() {
        initializeOpenGLFunctions();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        initShaders();
        initGridBuffers();
    }

    void StudioViewport::initShaders() {
        auto compileStage = [this](const char* src, GLenum type) -> GLuint {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint status;
            glGetShaderiv(s, GL_COMPILE_STATUS, &status);
            if (!status) {
                char log[1024];
                glGetShaderInfoLog(s, 1024, nullptr, log);
                std::cerr << "Shader compile error: " << log << std::endl;
            }
            return s;
        };

        GLuint vs = compileStage(Shaders::VERTEX_SHADER_SRC, GL_VERTEX_SHADER);
        GLuint fs = compileStage(Shaders::FRAGMENT_SHADER_SRC, GL_FRAGMENT_SHADER);
        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vs);
        glAttachShader(m_shaderProgram, fs);
        glLinkProgram(m_shaderProgram);

        // Cache Uniform Locations once during initialization
        m_locs.modelView = glGetUniformLocation(m_shaderProgram, "u_model_view");
        m_locs.projection = glGetUniformLocation(m_shaderProgram, "u_projection");
        m_locs.normalMatrix = glGetUniformLocation(m_shaderProgram, "u_normal_matrix");
        m_locs.diffuseMap = glGetUniformLocation(m_shaderProgram, "u_diffuse_map");
        m_locs.normalMap = glGetUniformLocation(m_shaderProgram, "u_normal_map");
        m_locs.mrMap = glGetUniformLocation(m_shaderProgram, "u_mr_map");
        m_locs.occlusionMap = glGetUniformLocation(m_shaderProgram, "u_occlusion_map");
        m_locs.emissiveMap = glGetUniformLocation(m_shaderProgram, "u_emissive_map");
        m_locs.hasDiffuse = glGetUniformLocation(m_shaderProgram, "u_has_diffuse");
        m_locs.hasNormal = glGetUniformLocation(m_shaderProgram, "u_has_normal");
        m_locs.hasMr = glGetUniformLocation(m_shaderProgram, "u_has_mr");
        m_locs.hasOcclusion = glGetUniformLocation(m_shaderProgram, "u_has_occlusion");
        m_locs.hasEmissive = glGetUniformLocation(m_shaderProgram, "u_has_emissive");
        m_locs.emissiveFactor = glGetUniformLocation(m_shaderProgram, "u_emissive_factor");
        m_locs.enableTextures = glGetUniformLocation(m_shaderProgram, "u_enable_textures");
        m_locs.enableNormals = glGetUniformLocation(m_shaderProgram, "u_enable_normals");
        m_locs.smoothShading = glGetUniformLocation(m_shaderProgram, "u_smooth_shading");
        m_locs.clayMode = glGetUniformLocation(m_shaderProgram, "u_clay_mode");
        m_locs.wireframeMode = glGetUniformLocation(m_shaderProgram, "u_wireframe_mode");
        m_locs.baseColor = glGetUniformLocation(m_shaderProgram, "u_base_color");
        m_locs.roughness = glGetUniformLocation(m_shaderProgram, "u_roughness");
        m_locs.metallic = glGetUniformLocation(m_shaderProgram, "u_metallic");
        m_locs.alphaCutoff = glGetUniformLocation(m_shaderProgram, "u_alpha_cutoff");

        // Floor Grid Shaders
        GLuint gvs = compileStage(Shaders::GRID_VERTEX_SRC, GL_VERTEX_SHADER);
        GLuint gfs = compileStage(Shaders::GRID_FRAGMENT_SRC, GL_FRAGMENT_SHADER);
        m_gridProgram = glCreateProgram();
        glAttachShader(m_gridProgram, gvs);
        glAttachShader(m_gridProgram, gfs);
        glLinkProgram(m_gridProgram);
        m_gridMvpLoc = glGetUniformLocation(m_gridProgram, "u_mvp");
    }

    void StudioViewport::initGridBuffers() {
        std::vector<float> gridData;
        float size = 3.2f;
        int steps = 16;
        float cGray[3] = {0.18f, 0.20f, 0.24f};

        for (int i = -steps; i <= steps; ++i) {
            float val = (static_cast<float>(i) / steps) * size;
            gridData.insert(gridData.end(), {val, -1.0f, -size, cGray[0], cGray[1], cGray[2]});
            gridData.insert(gridData.end(), {val, -1.0f, size, cGray[0], cGray[1], cGray[2]});
            gridData.insert(gridData.end(), {-size, -1.0f, val, cGray[0], cGray[1], cGray[2]});
            gridData.insert(gridData.end(), {size, -1.0f, val, cGray[0], cGray[1], cGray[2]});
        }

        // Central RGB Coordinate Axis Markers
        gridData.insert(gridData.end(), {0, -1, 0, 0.85f, 0.25f, 0.25f, 0.35f, -1, 0, 0.85f, 0.25f, 0.25f});
        gridData.insert(gridData.end(), {0, -1, 0, 0.25f, 0.85f, 0.35f, 0.65f, -1, 0, 0.25f, 0.85f, 0.35f});
        gridData.insert(gridData.end(), {0, -1, 0, 0.25f, 0.50f, 0.95f, 0, -1, 0.35f, 0.25f, 0.50f, 0.95f});

        m_gridVertexCount = static_cast<int>(gridData.size()) / 6;

        glGenVertexArrays(1, &m_gridVao);
        glGenBuffers(1, &m_gridVbo);

        glBindVertexArray(m_gridVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_gridVbo);
        glBufferData(GL_ARRAY_BUFFER, gridData.size() * sizeof(float), gridData.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    GLuint StudioViewport::uploadTexture(const std::string& key, const std::shared_ptr<DecodedImage>& decoded, GLenum minFilter, GLenum magFilter) {
        if (key.empty()) return 0;
        // Check cache first to avoid re-uploading shared textures
        if (m_textureCache.count(key)) return m_textureCache[key];
        if (!decoded) return 0;

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);

        GLenum effMin = enableFiltering ? minFilter : GL_NEAREST;
        GLenum effMag = enableFiltering ? magFilter : GL_NEAREST;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, effMin);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, effMag);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, decoded->width, decoded->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, decoded->rgba.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        m_textureCache[key] = texId;
        return texId;
    }

    void StudioViewport::setModel(std::shared_ptr<ModelData> modelData) {
        makeCurrent();
        cleanupBuffers();

        // Free previous GPU textures before clearing cache
        for (auto& [key, texId] : m_textureCache) {
            if (texId) glDeleteTextures(1, &texId);
        }
        m_textureCache.clear();

        m_model = modelData;
        if (m_model) {
            for (auto& b : m_model->batches) {
                b.diffTexId = uploadTexture(b.diffuseKey, b.diffuseDecoded, b.minFilter, b.magFilter);
                b.normTexId = uploadTexture(b.normalKey, b.normalDecoded, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                b.mrTexId = uploadTexture(b.mrKey, b.mrDecoded, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                b.occlusionTexId = uploadTexture(b.occlusionKey, b.occlusionDecoded, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                b.emissiveTexId = uploadTexture(b.emissiveKey, b.emissiveDecoded, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

                // Free CPU RAM immediately after VRAM upload
                b.diffuseDecoded.reset();
                b.normalDecoded.reset();
                b.mrDecoded.reset();
                b.occlusionDecoded.reset();
                b.emissiveDecoded.reset();

                // Interleaved VBO setup
                glGenVertexArrays(1, &b.vao);
                glBindVertexArray(b.vao);

                glGenBuffers(1, &b.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
                glBufferData(GL_ARRAY_BUFFER, b.vertices.size() * sizeof(Vertex), b.vertices.data(), GL_STATIC_DRAW);

                glGenBuffers(1, &b.ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, b.indices.size() * sizeof(uint32_t), b.indices.data(), GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE_BYTES, (void*)OFFSET_POSITION);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE_BYTES, (void*)OFFSET_NORMAL);
                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, VERTEX_STRIDE_BYTES, (void*)OFFSET_TEXCOORD);
                glEnableVertexAttribArray(3);
                glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, VERTEX_STRIDE_BYTES, (void*)OFFSET_TANGENT);

                glBindVertexArray(0);
            }
            resetCamera();
        }
        update();
    }

    void StudioViewport::cleanupBuffers() {
        if (!m_model) return;
        for (auto& b : m_model->batches) {
            if (b.vao) { glDeleteVertexArrays(1, &b.vao); b.vao = 0; }
            if (b.vbo) { glDeleteBuffers(1, &b.vbo); b.vbo = 0; }
            if (b.ebo) { glDeleteBuffers(1, &b.ebo); b.ebo = 0; }
        }
    }

    void StudioViewport::cleanupGL() {
        makeCurrent();
        cleanupBuffers();
        for (auto& [k, id] : m_textureCache) {
            glDeleteTextures(1, &id);
        }
        m_textureCache.clear();

        if (m_gridVao) { glDeleteVertexArrays(1, &m_gridVao); m_gridVao = 0; }
        if (m_gridVbo) { glDeleteBuffers(1, &m_gridVbo); m_gridVbo = 0; }
        if (m_shaderProgram) { glDeleteProgram(m_shaderProgram); m_shaderProgram = 0; }
        if (m_gridProgram) { glDeleteProgram(m_gridProgram); m_gridProgram = 0; }
    }

    void StudioViewport::resetCamera() {
        if (flyMode) return;
        m_distance = 2.8f; m_yaw = 35.0f; m_pitch = 20.0f;
        m_panOffset = {0.0f, 0.0f, 0.0f};
        update();
    }

    void StudioViewport::setTextureFilterMode(bool enabled) {
        enableFiltering = enabled;
        makeCurrent();
        if (m_model) {
            for (const auto& b : m_model->batches) {
                struct TexTarget { GLuint id; GLenum minF; GLenum magF; };
                TexTarget targets[] = {
                    {b.diffTexId, b.minFilter, b.magFilter},
                    {b.normTexId, (GLenum)GL_LINEAR_MIPMAP_LINEAR, (GLenum)GL_LINEAR},
                    {b.mrTexId, (GLenum)GL_LINEAR_MIPMAP_LINEAR, (GLenum)GL_LINEAR},
                    {b.occlusionTexId, (GLenum)GL_LINEAR_MIPMAP_LINEAR, (GLenum)GL_LINEAR},
                    {b.emissiveTexId, (GLenum)GL_LINEAR_MIPMAP_LINEAR, (GLenum)GL_LINEAR}
                };

                for (const auto& t : targets) {
                    if (t.id) {
                        glBindTexture(GL_TEXTURE_2D, t.id);
                        GLenum effMin = enabled ? (t.minF != GL_NEAREST ? t.minF : (GLenum)GL_LINEAR_MIPMAP_LINEAR) : (GLenum)GL_NEAREST;
                        GLenum effMag = enabled ? (t.magF != GL_NEAREST ? t.magF : (GLenum)GL_LINEAR) : (GLenum)GL_NEAREST;
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, effMin);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, effMag);
                    }
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        update();
    }

    void StudioViewport::resizeGL(int w, int h) {
        glViewport(0, 0, w, h);
    }

    void StudioViewport::paintGL() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int w = std::max(1, width());
        int h = std::max(1, height());
        Math::Mat4 proj = Math::perspective(Math::toRadians(42.0f), static_cast<float>(w) / h, 0.05f, 1000.0f);

        Math::Mat4 view;
        Math::Vec3 camPos;

        if (flyMode) {
            camPos = m_flyPos;
            view = Math::rotationX(Math::toRadians(m_pitch)) *
            Math::rotationY(Math::toRadians(m_yaw)) *
            Math::translation(-m_flyPos.x, -m_flyPos.y, -m_flyPos.z);
        } else {
            float yawR = Math::toRadians(m_yaw);
            float pitchR = Math::toRadians(m_pitch);
            camPos = {
                m_distance * std::sin(yawR) * std::cos(pitchR) - m_panOffset.x,
                -m_distance * std::sin(pitchR) - m_panOffset.y,
                m_distance * std::cos(yawR) * std::cos(pitchR)
            };
            view = Math::translation(m_panOffset.x, m_panOffset.y, -m_distance) *
            Math::rotationX(pitchR) *
            Math::rotationY(yawR);
        }

        if (showGrid) {
            renderStudioGrid(proj * view);
        }

        if (!m_model) return;

        glUseProgram(m_shaderProgram);

        float normMat[9] = {
            view.m[0], view.m[1], view.m[2],
            view.m[4], view.m[5], view.m[6],
            view.m[8], view.m[9], view.m[10]
        };

        glUniformMatrix4fv(m_locs.modelView, 1, GL_TRUE, view.m);
        glUniformMatrix4fv(m_locs.projection, 1, GL_TRUE, proj.m);
        glUniformMatrix3fv(m_locs.normalMatrix, 1, GL_TRUE, normMat);

        bool isClayMode = !enableTextures;
        glUniform1i(m_locs.diffuseMap, 0);
        glUniform1i(m_locs.normalMap, 1);
        glUniform1i(m_locs.mrMap, 2);
        glUniform1i(m_locs.occlusionMap, 3);
        glUniform1i(m_locs.emissiveMap, 4);
        glUniform1i(m_locs.enableTextures, enableTextures);
        glUniform1i(m_locs.enableNormals, enableNormals);
        glUniform1i(m_locs.smoothShading, smoothShading);
        glUniform1i(m_locs.clayMode, isClayMode);

        std::vector<RenderBatch*> opaque;
        std::vector<RenderBatch*> transparent;

        for (auto& b : m_model->batches) {
            if (b.isTransparent) transparent.push_back(&b);
            else opaque.push_back(&b);
        }

        // Back-to-Front depth sorting for transparent batches
        std::sort(transparent.begin(), transparent.end(), [&](RenderBatch* a, RenderBatch* b) {
            return Math::length(camPos - a->centroid) > Math::length(camPos - b->centroid);
        });

        if (showWireframe) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
        }

        // ------------------------------------------------------------------------
        // PASS 1: Opaque Batches (Depth Write: ON)
        // ------------------------------------------------------------------------
        glDepthMask(GL_TRUE);
        for (auto* b : opaque) {
            glUniform1i(m_locs.hasDiffuse, b->diffTexId > 0);
            glUniform1i(m_locs.hasNormal, b->normTexId > 0);
            glUniform1i(m_locs.hasMr, b->mrTexId > 0);
            glUniform1i(m_locs.hasOcclusion, b->occlusionTexId > 0);
            glUniform1i(m_locs.hasEmissive, b->emissiveTexId > 0);
            glUniform3fv(m_locs.emissiveFactor, 1, b->emissiveFactor.data());
            glUniform4fv(m_locs.baseColor, 1, b->baseColor.data());
            glUniform1f(m_locs.roughness, b->roughness);
            glUniform1f(m_locs.metallic, b->metallic);
            glUniform1f(m_locs.alphaCutoff, b->alphaCutoff);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, b->diffTexId);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, b->normTexId);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, b->mrTexId);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, b->occlusionTexId);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, b->emissiveTexId);

            glBindVertexArray(b->vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(b->indexCount), GL_UNSIGNED_INT, nullptr);
        }

        // ------------------------------------------------------------------------
        // PASS 2: Transparent Batches (Depth Write: OFF, Back-to-Front Sorted)
        // ------------------------------------------------------------------------
        if (!isClayMode) {
            glDepthMask(GL_FALSE);
            for (auto* b : transparent) {
                glUniform1i(m_locs.hasDiffuse, b->diffTexId > 0);
                glUniform1i(m_locs.hasNormal, b->normTexId > 0);
                glUniform1i(m_locs.hasMr, b->mrTexId > 0);
                glUniform1i(m_locs.hasOcclusion, b->occlusionTexId > 0);
                glUniform1i(m_locs.hasEmissive, b->emissiveTexId > 0);
                glUniform3fv(m_locs.emissiveFactor, 1, b->emissiveFactor.data());
                glUniform4fv(m_locs.baseColor, 1, b->baseColor.data());
                glUniform1f(m_locs.roughness, b->roughness);
                glUniform1f(m_locs.metallic, b->metallic);
                glUniform1f(m_locs.alphaCutoff, b->alphaCutoff);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, b->diffTexId);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, b->normTexId);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, b->mrTexId);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, b->occlusionTexId);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, b->emissiveTexId);

                glBindVertexArray(b->vao);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(b->indexCount), GL_UNSIGNED_INT, nullptr);
            }
            glDepthMask(GL_TRUE);
        }

        if (showWireframe) {
            glDisable(GL_POLYGON_OFFSET_FILL);

            // --------------------------------------------------------------------
            // PASS 3: Wireframe Edge Overlay Pass
            // --------------------------------------------------------------------
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.0f);
            glDepthMask(GL_FALSE);

            glUniform1i(m_locs.wireframeMode, 1);
            for (auto& b : m_model->batches) {
                if (isClayMode && b.isTransparent) continue;
                glBindVertexArray(b.vao);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(b.indexCount), GL_UNSIGNED_INT, nullptr);
            }
            glUniform1i(m_locs.wireframeMode, 0);

            glDepthMask(GL_TRUE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void StudioViewport::renderStudioGrid(const Math::Mat4& mvp) {
        glUseProgram(m_gridProgram);
        glUniformMatrix4fv(m_gridMvpLoc, 1, GL_TRUE, mvp.m);
        glBindVertexArray(m_gridVao);
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void StudioViewport::setFlyMode(bool enabled) {
        flyMode = enabled;
        if (enabled) {
            float pitchRad = Math::toRadians(m_pitch);
            float yawRad = Math::toRadians(m_yaw);

            float vx = m_panOffset.x; float vy = m_panOffset.y; float vz = -m_distance;
            float cp = std::cos(pitchRad); float sp = std::sin(pitchRad);
            float x1 = vx;
            float y1 = cp * vy + sp * vz;
            float z1 = -sp * vy + cp * vz;

            float cy = std::cos(yawRad); float sy = std::sin(yawRad);
            float x2 = cy * x1 - sy * z1;
            float y2 = y1;
            float z2 = sy * x1 + cy * z1;

            m_flyPos = {-x2, -y2, -z2};

            grabMouse(Qt::BlankCursor);
            QPoint center = rect().center();
            m_lastPos = center;
            QCursor::setPos(mapToGlobal(center));

            m_lastFlyTime = QDateTime::currentMSecsSinceEpoch();
            m_flyTimer->start();
            setFocus();
        } else {
            releaseMouse();
            unsetCursor();
            m_flyTimer->stop();
            m_activeActions.clear();
            m_keyActionMap.clear();
        }

        emit flyModeToggled(enabled);
        update();
    }

    void StudioViewport::keyPressEvent(QKeyEvent* event) {
        if (!flyMode) {
            QOpenGLWidget::keyPressEvent(event);
            return;
        }

        if (event->key() == Qt::Key_Escape) {
            setFlyMode(false);
            return;
        }

        int key = event->key();
        QString text = event->text().toLower();

        std::string action = "";
        if (key == Qt::Key_W || key == Qt::Key_Up || text == "w" || text == "ц") action = "forward";
        else if (key == Qt::Key_S || key == Qt::Key_Down || text == "s" || text == "ы") action = "backward";
        else if (key == Qt::Key_A || key == Qt::Key_Left || text == "a" || text == "ф") action = "left";
        else if (key == Qt::Key_D || key == Qt::Key_Right || text == "d" || text == "в") action = "right";
        else if (key == Qt::Key_E || key == Qt::Key_Space || text == "e" || text == "у" || text == " ") action = "up";
        else if (key == Qt::Key_Q || key == Qt::Key_Control || text == "q" || text == "й") action = "down";

        if (!action.empty()) {
            m_activeActions.insert(action);
            m_keyActionMap[key] = action;
        }
    }

    void StudioViewport::keyReleaseEvent(QKeyEvent* event) {
        if (!flyMode) {
            QOpenGLWidget::keyReleaseEvent(event);
            return;
        }
        int key = event->key();
        auto it = m_keyActionMap.find(key);
        if (it != m_keyActionMap.end()) {
            m_activeActions.erase(it->second);
            m_keyActionMap.erase(it);
        }
    }

    void StudioViewport::updateFlyMovement() {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        float dt = (now - m_lastFlyTime) / 1000.0f;
        m_lastFlyTime = now;

        if (!flyMode || m_activeActions.empty()) return;

        dt = std::min(dt, 0.1f); // Clamp delta-time against sudden lag spikes
        float speed = m_flySpeed * dt;

        float yawRad = Math::toRadians(m_yaw);
        float pitchRad = Math::toRadians(m_pitch);

        Math::Vec3 forward = {
            std::sin(yawRad) * std::cos(pitchRad),
            -std::sin(pitchRad),
            -std::cos(yawRad) * std::cos(pitchRad)
        };
        Math::Vec3 right = {std::cos(yawRad), 0.0f, std::sin(yawRad)};
        Math::Vec3 up = {0.0f, 1.0f, 0.0f};

        Math::Vec3 move{0.0f, 0.0f, 0.0f};
        if (m_activeActions.count("forward")) move += forward;
        if (m_activeActions.count("backward")) move -= forward;
        if (m_activeActions.count("right")) move += right;
        if (m_activeActions.count("left")) move -= right;
        if (m_activeActions.count("up")) move += up;
        if (m_activeActions.count("down")) move -= up;

        if (Math::length(move) > 0.001f) {
            m_flyPos += Math::normalize(move) * speed;
            update();
        }
    }

    void StudioViewport::mousePressEvent(QMouseEvent* event) {
        m_lastPos = event->pos();
    }

    void StudioViewport::mouseMoveEvent(QMouseEvent* event) {
        if (flyMode) {
            QPoint pos = event->pos();
            int dx = pos.x() - m_lastPos.x();
            int dy = pos.y() - m_lastPos.y();
            m_lastPos = pos;

            if (dx != 0 || dy != 0) {
                m_yaw += dx * 0.08f;
                m_pitch = std::clamp(m_pitch + dy * 0.08f, -89.0f, 89.0f);
                update();
            }

            // Center cursor warp preventing pointer escape
            QPoint center = rect().center();
            if (std::abs(pos.x() - center.x()) > 40 || std::abs(pos.y() - center.y()) > 40) {
                m_lastPos = center;
                QCursor::setPos(mapToGlobal(center));
            }
            return;
        }

        int dx = event->pos().x() - m_lastPos.x();
        int dy = event->pos().y() - m_lastPos.y();
        auto btns = event->buttons();

        if (btns & Qt::LeftButton) {
            m_yaw += dx * 0.35f;
            m_pitch = std::clamp(m_pitch + dy * 0.35f, -89.0f, 89.0f);
            update();
        } else if (btns & (Qt::RightButton | Qt::MiddleButton)) {
            float f = m_distance * 0.0015f;
            m_panOffset.x += dx * f;
            m_panOffset.y -= dy * f;
            update();
        }
        m_lastPos = event->pos();
    }

    void StudioViewport::wheelEvent(QWheelEvent* event) {
        int delta = event->angleDelta().y();
        if (flyMode) {
            float factor = delta > 0 ? 1.15f : 0.85f;
            m_flySpeed = std::clamp(m_flySpeed * factor, 0.1f, 50.0f);
        } else {
            float factor = delta > 0 ? 0.88f : 1.13f;
            m_distance = std::clamp(m_distance * factor, 0.05f, 500.0f);
        }
        update();
    }

    void StudioViewport::focusOutEvent(QFocusEvent* event) {
        if (flyMode) setFlyMode(false);
        QOpenGLWidget::focusOutEvent(event);
    }

} // namespace StudioViewer
