/**
 * @file VectorIcons.h
 * @brief Zero-asset procedural vector icon engine rendering through QPainterPath.
 */

#pragma once

#include <QIcon>
#include <QPixmap>
#include <QString>

namespace StudioViewer {

    class VectorIcons {
    public:
        static QPixmap renderPixmap(const QString& kind, const QString& colorHex, int size = 32);
        static QIcon createIcon(const QString& kind);
    };

} // namespace StudioViewer
