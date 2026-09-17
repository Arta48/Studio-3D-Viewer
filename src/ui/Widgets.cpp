#include "ui/Widgets.h"
#include "core/I18n.h"
#include "core/Constants.h"

namespace StudioViewer {

    FloatingHUD::FloatingHUD(QWidget* parent) : QFrame(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setStyleSheet(
            "FloatingHUD { background-color: rgba(18, 20, 24, 0.85); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; }"
            "QLabel { color: #8e94a0; font-family: 'Segoe UI', system-ui, sans-serif; font-size: 11px; font-weight: 500; }"
            "QLabel#Title { color: #ffffff; font-size: 12px; font-weight: 600; }"
            "QLabel#Highlight { color: #4f8bf9; font-weight: 600; }"
        );

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 10, 12, 10);

        m_lblTitle = new QLabel(I18n::tr("hud_standby"), this);
        m_lblTitle->setObjectName("Title");
        m_lblStats = new QLabel(I18n::tr("hud_stats_placeholder"), this);
        m_lblMaterials = new QLabel(I18n::tr("hud_materials_placeholder"), this);
        m_lblTime = new QLabel(I18n::tr("hud_time_placeholder"), this);
        m_lblTime->setObjectName("Highlight");

        layout->addWidget(m_lblTitle);
        layout->addWidget(m_lblStats);
        layout->addWidget(m_lblMaterials);
        layout->addWidget(m_lblTime);
    }

    void FloatingHUD::updateInfo(const ModelData& data) {
        m_lblTitle->setText(QString::fromStdString(data.name));
        m_lblStats->setText(I18n::tr("hud_stats", {
            {"tri_count", QString::number(data.numTriangles)},
                                     {"vert_count", QString::number(data.numVertices)}
        }));
        m_lblMaterials->setText(I18n::tr("hud_materials", {
            {"batches", QString::number(data.batches.size())},
                                         {"textures", QString::number(data.numTextures)},
                                         {"normals", QString::number(data.numNormals)}
        }));
        m_lblTime->setText(I18n::tr("hud_time", {
            {"load_time", QString::number(data.loadTime, 'f', 2)},
                                    {"workers", QString::number(getMaxWorkers())}
        }));
    }

    EmptyDropZone::EmptyDropZone(QWidget* parent) : QWidget(parent) {
        auto layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(12);

        auto iconLbl = new QLabel("✦", this);
        iconLbl->setAlignment(Qt::AlignCenter);
        iconLbl->setStyleSheet("font-size: 36px; color: #4f8bf9;");

        auto textLbl = new QLabel(I18n::tr("drop_title"), this);
        textLbl->setAlignment(Qt::AlignCenter);
        textLbl->setStyleSheet("font-size: 15px; font-weight: 600; color: #ffffff;");

        auto subLbl = new QLabel(I18n::tr("drop_subtitle"), this);
        subLbl->setAlignment(Qt::AlignCenter);
        subLbl->setStyleSheet("font-size: 12px; color: #707582;");

        auto btnOpen = new QPushButton(I18n::tr("drop_button"), this);
        btnOpen->setCursor(Qt::PointingHandCursor);
        btnOpen->setStyleSheet(
            "QPushButton { background-color: #2b303c; border: 1px solid rgba(255,255,255,0.1); color: #fff; font-size: 12px; font-weight: 600; padding: 8px 20px; border-radius: 6px; }"
            "QPushButton:hover { background-color: #383e4e; }"
        );
        connect(btnOpen, &QPushButton::clicked, this, &EmptyDropZone::openClicked);

        layout->addWidget(iconLbl);
        layout->addWidget(textLbl);
        layout->addWidget(subLbl);
        layout->addWidget(btnOpen, 0, Qt::AlignCenter);
    }

} // namespace StudioViewer
