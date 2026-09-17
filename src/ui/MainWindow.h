/**
 * @file MainWindow.h
 * @brief Application main window coordinating menus, viewports, workers, and events.
 */

#pragma once

#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
#include <QAction>
#include <QStackedLayout>
#include "gl/StudioViewport.h"
#include "ui/Widgets.h"
#include "loaders/ModelLoaderWorker.h"

namespace StudioViewer {

    class MainWindow : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget* parent = nullptr);
        void loadModel(const QString& path);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

    private:
        void createToolbar();
        void createStatusBar();
        void applyTheme();
        void openFileDialog();
        void showAboutDialog();

    private slots:
        void onFlyModeToggled(bool enabled);
        void onModelLoaded(std::shared_ptr<ModelData> data);
        void onModelError(const QString& errMsg);

    private:
        StudioViewport* m_viewport{nullptr};
        FloatingHUD* m_hud{nullptr};
        EmptyDropZone* m_emptyZone{nullptr};
        QProgressBar* m_progressBar{nullptr};
        QLabel* m_lblStatus{nullptr};
        QAction* m_actReset{nullptr};
        QAction* m_actFly{nullptr};
        QAction* m_actTexFilter{nullptr};
        ModelLoaderWorker* m_worker{nullptr};
    };

} // namespace StudioViewer
