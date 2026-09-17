#include "ui/MainWindow.h"
#include "ui/VectorIcons.h"
#include "core/I18n.h"
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>

namespace StudioViewer {

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        setWindowTitle(I18n::tr("window_title"));
        resize(1280, 820);
        setAcceptDrops(true);
        setWindowIcon(QIcon(":/icon.png"));

        auto container = new QWidget(this);
        setCentralWidget(container);

        auto rootLayout = new QStackedLayout(container);
        rootLayout->setStackingMode(QStackedLayout::StackAll);

        m_viewport = new StudioViewport(this);
        rootLayout->addWidget(m_viewport);

        auto hudContainer = new QWidget(this);
        auto hudLayout = new QHBoxLayout(hudContainer);
        hudLayout->setContentsMargins(18, 18, 0, 0);
        hudLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        m_hud = new FloatingHUD(hudContainer);
        m_hud->setVisible(false);
        hudLayout->addWidget(m_hud);
        rootLayout->addWidget(hudContainer);

        m_emptyZone = new EmptyDropZone(this);
        connect(m_emptyZone, &EmptyDropZone::openClicked, this, &MainWindow::openFileDialog);
        rootLayout->addWidget(m_emptyZone);

        createToolbar();
        createStatusBar();
        applyTheme();
    }

    void MainWindow::createToolbar() {
        auto tb = addToolBar(I18n::tr("toolbar_main"));
        tb->setMovable(false);
        tb->setIconSize(QSize(18, 18));
        tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        auto actOpen = new QAction(VectorIcons::createIcon("open"), I18n::tr("action_open"), this);
        actOpen->setShortcut(QKeySequence("Ctrl+O"));
        connect(actOpen, &QAction::triggered, this, &MainWindow::openFileDialog);
        tb->addAction(actOpen);

        m_actReset = new QAction(VectorIcons::createIcon("focus"), I18n::tr("action_focus"), this);
        m_actReset->setShortcut(QKeySequence("Space"));
        connect(m_actReset, &QAction::triggered, m_viewport, &StudioViewport::resetCamera);
        tb->addAction(m_actReset);

        m_actFly = new QAction(VectorIcons::createIcon("fly"), I18n::tr("action_fly"), this);
        m_actFly->setCheckable(true);
        m_actFly->setShortcut(QKeySequence("F"));
        connect(m_actFly, &QAction::triggered, m_viewport, &StudioViewport::setFlyMode);
        connect(m_viewport, &StudioViewport::flyModeToggled, this, &MainWindow::onFlyModeToggled);
        tb->addAction(m_actFly);

        tb->addSeparator();

        auto actSmooth = new QAction(VectorIcons::createIcon("smooth"), I18n::tr("action_smooth"), this);
        actSmooth->setCheckable(true);
        connect(actSmooth, &QAction::toggled, [this](bool v) { m_viewport->smoothShading = v; m_viewport->update(); });
        tb->addAction(actSmooth);

        m_actTexFilter = new QAction(VectorIcons::createIcon("tex_filter"), I18n::tr("action_tex_filter"), this);
        m_actTexFilter->setCheckable(true);
        connect(m_actTexFilter, &QAction::toggled, m_viewport, &StudioViewport::setTextureFilterMode);
        tb->addAction(m_actTexFilter);

        auto actTex = new QAction(VectorIcons::createIcon("textures"), I18n::tr("action_textures"), this);
        actTex->setCheckable(true);
        actTex->setChecked(true);
        connect(actTex, &QAction::toggled, [this](bool v) { m_viewport->enableTextures = v; m_viewport->update(); });
        tb->addAction(actTex);

        auto actNorm = new QAction(VectorIcons::createIcon("normals"), I18n::tr("action_normals"), this);
        actNorm->setCheckable(true);
        actNorm->setChecked(true);
        connect(actNorm, &QAction::toggled, [this](bool v) { m_viewport->enableNormals = v; m_viewport->update(); });
        tb->addAction(actNorm);

        tb->addSeparator();

        auto actWire = new QAction(VectorIcons::createIcon("wireframe"), I18n::tr("action_wireframe"), this);
        actWire->setCheckable(true);
        connect(actWire, &QAction::toggled, [this](bool v) { m_viewport->showWireframe = v; m_viewport->update(); });
        tb->addAction(actWire);

        auto actGrid = new QAction(VectorIcons::createIcon("grid"), I18n::tr("action_floor"), this);
        actGrid->setCheckable(true);
        actGrid->setChecked(true);
        connect(actGrid, &QAction::toggled, [this](bool v) { m_viewport->showGrid = v; m_viewport->update(); });
        tb->addAction(actGrid);

        tb->addSeparator();

        auto actAbout = new QAction(VectorIcons::createIcon("about"), I18n::tr("action_about"), this);
        connect(actAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
        tb->addAction(actAbout);
    }

    void MainWindow::onFlyModeToggled(bool enabled) {
        m_actFly->setChecked(enabled);
        m_actReset->setShortcut(enabled ? QKeySequence() : QKeySequence("Space"));
    }

    void MainWindow::createStatusBar() {
        auto sb = statusBar();
        m_lblStatus = new QLabel(I18n::tr("status_ready"), this);
        sb->addWidget(m_lblStatus);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setMaximumWidth(140);
        m_progressBar->setMaximumHeight(10);
        m_progressBar->setTextVisible(false);
        m_progressBar->setVisible(false);
        sb->addPermanentWidget(m_progressBar);
    }

    void MainWindow::applyTheme() {
        setStyleSheet(
            "QMainWindow { background-color: #0e0f12; }"
            "QToolBar { background-color: #14161a; border-bottom: 1px solid #1f2228; padding: 3px 6px; spacing: 4px; }"
            "QToolButton { color: #d1d5db; background-color: transparent; border: 1px solid transparent; padding: 5px 7px; border-radius: 4px; font-weight: 500; font-size: 12px; }"
            "QToolButton:hover { background-color: #1f232b; color: #ffffff; }"
            "QToolButton:checked { background-color: #232c3d; border: 1px solid #3b5f94; color: #6ba6ff; }"
            "QStatusBar { background-color: #0e0f12; color: #656b78; border-top: 1px solid #1a1c22; font-size: 11px; }"
            "QProgressBar { border: none; background-color: #1c1f26; border-radius: 5px; }"
            "QProgressBar::chunk { background-color: #4f8bf9; border-radius: 5px; }"
            "QMessageBox { background-color: #14161a; }"
            "QMessageBox QLabel { color: #d1d5db; font-size: 12px; }"
            "QMessageBox QPushButton { background-color: #232c3d; border: 1px solid #3b5f94; color: #ffffff; padding: 5px 18px; border-radius: 4px; font-weight: 600; }"
            "QMessageBox QPushButton:hover { background-color: #2d3b52; }"
        );
    }

    void MainWindow::openFileDialog() {
        QString path = QFileDialog::getOpenFileName(this, I18n::tr("dialog_open_title"), "", I18n::tr("dialog_filter"));
        if (!path.isEmpty()) loadModel(path);
    }

    void MainWindow::showAboutDialog() {
        QMessageBox msg(this);
        msg.setWindowTitle(I18n::tr("about_title"));
        msg.setText(I18n::tr("about_text"));

        QPixmap iconPix(":/icon.png");
        if (iconPix.isNull()) {
            iconPix = QPixmap("assets/icon.png");
        }
        if (iconPix.isNull()) {
            iconPix = windowIcon().pixmap(64, 64);
        }

        if (!iconPix.isNull()) {
            msg.setIconPixmap(iconPix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            msg.setIconPixmap(VectorIcons::renderPixmap("about", "#4f8bf9", 64));
        }

        msg.exec();
    }

    void MainWindow::loadModel(const QString& path) {
        if (m_worker) {
            if (m_worker->isRunning()) {
                m_worker->cancel();
                m_worker->wait();
            }
            delete m_worker;
            m_worker = nullptr;
        }

        QFileInfo fi(path);
        m_lblStatus->setText(I18n::tr("status_loading", {{"filename", fi.fileName()}}));
        m_progressBar->setRange(0, 0);
        m_progressBar->setVisible(true);

        m_worker = new ModelLoaderWorker(path, this);
        connect(m_worker, &ModelLoaderWorker::progress, m_lblStatus, &QLabel::setText);
        connect(m_worker, &ModelLoaderWorker::finished, this, &MainWindow::onModelLoaded);
        connect(m_worker, &ModelLoaderWorker::error, this, &MainWindow::onModelError);
        m_worker->start();
    }

    void MainWindow::onModelLoaded(std::shared_ptr<ModelData> data) {
        m_progressBar->setVisible(false);
        m_emptyZone->setVisible(false);

        // Synchronize filtering setting with model specifications (e.g. Blockbench nearest)
        bool hasNearestSpec = false;
        for (const auto& b : data->batches) {
            if (b.magFilter == GL_NEAREST) {
                hasNearestSpec = true;
                break;
            }
        }
        if (hasNearestSpec) {
            m_actTexFilter->setChecked(false);
            m_viewport->enableFiltering = false;
        } else {
            m_viewport->enableFiltering = m_actTexFilter->isChecked();
        }

        m_viewport->setModel(data);
        m_hud->updateInfo(*data);
        m_hud->setVisible(true);
        m_lblStatus->setText(I18n::tr("status_loaded", {{"tri_count", QString::number(data->numTriangles)}}));
    }

    void MainWindow::onModelError(const QString& errMsg) {
        m_progressBar->setVisible(false);
        m_lblStatus->setText(I18n::tr("status_error"));
        QMessageBox::critical(this, I18n::tr("error_load_title"), I18n::tr("error_load_msg", {{"error", errMsg}}));
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        if (m_worker && m_worker->isRunning()) {
            m_worker->cancel();
            m_worker->wait();
        }
        m_viewport->cleanupGL();
        QMainWindow::closeEvent(event);
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
        if (event->mimeData()->hasUrls()) {
            for (const auto& u : event->mimeData()->urls()) {
                QString path = u.toLocalFile().toLower();
                if (path.endsWith(".obj") || path.endsWith(".glb") || path.endsWith(".gltf")) {
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }

    void MainWindow::dropEvent(QDropEvent* event) {
        for (const auto& u : event->mimeData()->urls()) {
            QString p = u.toLocalFile();
            QString lp = p.toLower();
            if (lp.endsWith(".obj") || lp.endsWith(".glb") || lp.endsWith(".gltf")) {
                loadModel(p);
                break;
            }
        }
    }

} // namespace StudioViewer
