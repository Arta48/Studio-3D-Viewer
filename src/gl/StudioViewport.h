/**
 * @file StudioViewport.h
 * @brief OpenGL 3.3 Core Profile viewport managing camera controls, depth sorting, and rendering passes.
 */

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QTimer>
#include <QPoint>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <memory>
#include <unordered_map>
#include <set>
#include "core/Models.h"

namespace StudioViewer {

    /**
     * @brief Pre-queried uniform locations to eliminate per-frame glGetUniformLocation lookups.
     */
    struct ShaderLocations {
        GLint modelView{-1};
        GLint projection{-1};
        GLint normalMatrix{-1};
        GLint diffuseMap{-1};
        GLint normalMap{-1};
        GLint mrMap{-1};
        GLint occlusionMap{-1};
        GLint emissiveMap{-1};
        GLint hasDiffuse{-1};
        GLint hasNormal{-1};
        GLint hasMr{-1};
        GLint hasOcclusion{-1};
        GLint hasEmissive{-1};
        GLint emissiveFactor{-1};
        GLint enableTextures{-1};
        GLint enableNormals{-1};
        GLint smoothShading{-1};
        GLint clayMode{-1};
        GLint wireframeMode{-1};
        GLint baseColor{-1};
        GLint roughness{-1};
        GLint metallic{-1};
        GLint alphaCutoff{-1};
    };

    class StudioViewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
        Q_OBJECT
    public:
        explicit StudioViewport(QWidget* parent = nullptr);
        ~StudioViewport() override;

        void setModel(std::shared_ptr<ModelData> modelData);
        void resetCamera();
        void setTextureFilterMode(bool enabled);
        void setFlyMode(bool enabled);
        void cleanupGL();

        bool smoothShading{false};
        bool enableFiltering{false};
        bool enableTextures{true};
        bool enableNormals{true};
        bool showWireframe{false};
        bool showGrid{true};
        bool flyMode{false};

    signals:
        void flyModeToggled(bool enabled);

    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    private:
        void initShaders();
        void initGridBuffers();
        GLuint uploadTexture(const std::string& key, const std::shared_ptr<DecodedImage>& decoded, GLenum minFilter, GLenum magFilter);
        void cleanupBuffers();
        void renderStudioGrid(const Math::Mat4& mvp);

    private slots:
        void updateFlyMovement();

    private:
        std::shared_ptr<ModelData> m_model;
        GLuint m_shaderProgram{0};
        ShaderLocations m_locs;
        GLuint m_gridProgram{0};
        GLint m_gridMvpLoc{-1};

        GLuint m_gridVao{0};
        GLuint m_gridVbo{0};
        int m_gridVertexCount{0};

        std::unordered_map<std::string, GLuint> m_textureCache;

        // Turntable Orbit Camera State
        float m_yaw{35.0f};
        float m_pitch{22.0f};
        float m_distance{3.2f};
        Math::Vec3 m_panOffset{0.0f, 0.0f, 0.0f};
        QPoint m_lastPos;

        // 6-DOF FPS Free Flight State
        Math::Vec3 m_flyPos{0.0f, 0.0f, 3.2f};
        float m_flySpeed{2.0f};
        qint64 m_lastFlyTime{0};
        std::set<std::string> m_activeActions;
        std::unordered_map<int, std::string> m_keyActionMap;
        QTimer* m_flyTimer{nullptr};
    };

} // namespace StudioViewer
