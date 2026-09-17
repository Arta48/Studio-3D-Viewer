/**
 * @file Widgets.h
 * @brief Floating statistic HUD overlay and empty drop zone placeholder widgets.
 */

#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include "core/Models.h"

namespace StudioViewer {

    class FloatingHUD : public QFrame {
        Q_OBJECT
    public:
        explicit FloatingHUD(QWidget* parent = nullptr);
        void updateInfo(const ModelData& data);

    private:
        QLabel* m_lblTitle;
        QLabel* m_lblStats;
        QLabel* m_lblMaterials;
        QLabel* m_lblTime;
    };

    class EmptyDropZone : public QWidget {
        Q_OBJECT
    public:
        explicit EmptyDropZone(QWidget* parent = nullptr);

    signals:
        void openClicked();
    };

} // namespace StudioViewer
