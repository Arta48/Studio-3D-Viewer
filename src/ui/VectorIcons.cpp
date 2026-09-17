#include "ui/VectorIcons.h"
#include <QPainter>
#include <QPainterPath>

namespace StudioViewer {

    QPixmap VectorIcons::renderPixmap(const QString& kind, const QString& colorHex, int size) {
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);

        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);

        float scale = size / 32.0f;
        painter.scale(scale, scale);

        QPen pen(QColor(colorHex), 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        if (kind == "open") {
            QPainterPath p;
            p.moveTo(4, 9); p.lineTo(4, 25); p.lineTo(28, 25); p.lineTo(28, 12);
            p.lineTo(16, 12); p.lineTo(13, 9); p.closeSubpath();
            painter.drawPath(p);
            painter.drawLine(4, 15, 28, 15);
        } else if (kind == "focus") {
            painter.drawLine(5, 11, 5, 6); painter.drawLine(5, 6, 10, 6);
            painter.drawLine(22, 6, 27, 6); painter.drawLine(27, 6, 27, 11);
            painter.drawLine(5, 21, 5, 26); painter.drawLine(5, 26, 10, 26);
            painter.drawLine(22, 26, 27, 26); painter.drawLine(27, 26, 27, 21);
            painter.setBrush(QColor(colorHex));
            painter.drawEllipse(QPointF(16, 16), 2.2, 2.2);
        } else if (kind == "fly") {
            QPainterPath p;
            p.moveTo(6, 16); p.lineTo(26, 6); p.lineTo(16, 26); p.lineTo(14, 18);
            p.closeSubpath();
            painter.drawPath(p);
            painter.drawLine(14, 18, 26, 6);
        } else if (kind == "smooth") {
            painter.drawEllipse(QPointF(16, 16), 11, 11);
            painter.drawArc(QRectF(9, 9, 14, 14), 45 * 16, 90 * 16);
        } else if (kind == "tex_filter") {
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    QRectF rect(7 + c * 6, 7 + r * 6, 4.5, 4.5);
                    if ((r + c) % 2 == 0) painter.fillRect(rect, QColor(colorHex));
                    else painter.drawRect(rect);
                }
            }
        } else if (kind == "textures") {
            painter.drawRoundedRect(QRectF(10, 5, 17, 16), 3, 3);
            painter.setBrush(QColor(20, 22, 26, 220));
            painter.drawRoundedRect(QRectF(5, 11, 17, 16), 3, 3);
            painter.setBrush(Qt::NoBrush);
            QPainterPath p;
            p.moveTo(8, 23); p.lineTo(12, 16); p.lineTo(15, 19); p.lineTo(18, 15); p.lineTo(20, 23);
            painter.drawPath(p);
        } else if (kind == "normals") {
            painter.drawEllipse(QPointF(15, 17), 9, 9);
            painter.drawLine(15, 17, 24, 8);
            painter.drawLine(24, 8, 20, 8);
            painter.drawLine(24, 8, 24, 12);
        } else if (kind == "wireframe") {
            QPolygonF pts({QPointF(16, 5), QPointF(26, 11), QPointF(26, 21),
                QPointF(16, 27), QPointF(6, 21), QPointF(6, 11)});
            painter.drawPolygon(pts);
            painter.drawLine(16, 16, 16, 27);
            painter.drawLine(16, 16, 6, 11);
            painter.drawLine(16, 16, 26, 11);
        } else if (kind == "grid") {
            painter.drawLine(8, 9, 24, 9); painter.drawLine(4, 25, 28, 25);
            painter.drawLine(8, 9, 4, 25); painter.drawLine(24, 9, 28, 25);
            painter.drawLine(13, 9, 12, 25); painter.drawLine(19, 9, 20, 25);
            painter.drawLine(6, 17, 26, 17);
        } else if (kind == "about") {
            painter.drawEllipse(QPointF(16, 16), 11, 11);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(colorHex));
            painter.drawEllipse(QPointF(16.0, 11.5), 1.6, 1.6);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(QPointF(16.0, 15.0), QPointF(16.0, 21.0));
            painter.drawLine(QPointF(14.0, 21.0), QPointF(18.0, 21.0));
        }

        painter.end();
        return pix;
    }

    QIcon VectorIcons::createIcon(const QString& kind) {
        QIcon icon;
        icon.addPixmap(renderPixmap(kind, "#9da3af"), QIcon::Normal, QIcon::Off);
        icon.addPixmap(renderPixmap(kind, "#6ba6ff"), QIcon::Normal, QIcon::On);
        return icon;
    }

} // namespace StudioViewer
