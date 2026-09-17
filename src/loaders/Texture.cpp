#include "loaders/Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"
#include <QFileInfo>
#include <QDir>
#include <QImage>

namespace StudioViewer {

    std::string TextureLoader::resolveLocalTexture(const std::string& baseDir, const std::string& filename) {
        if (filename.empty()) return "";

        QString clean = QString::fromStdString(filename).replace('\\', '/').trimmed();
        if (clean.startsWith('"') && clean.endsWith('"')) {
            clean = clean.mid(1, clean.length() - 2);
        }

        QFileInfo fi(clean);
        QString pName = fi.fileName();
        QDir base(QString::fromStdString(baseDir));

        QStringList candidates = {
            base.filePath(clean),
            base.filePath(pName),
            base.filePath("textures/" + clean),
            base.filePath("textures/" + pName)
        };

        for (const auto& c : candidates) {
            if (QFileInfo::exists(c)) return c.toStdString();
        }

        // Case-insensitive fallback for Linux environments
        QString lowerName = pName.toLower();
        for (const auto& subDir : {base.absolutePath(), base.filePath("textures")}) {
            QDir d(subDir);
            for (const auto& entry : d.entryInfoList(QDir::Files)) {
                if (entry.fileName().toLower() == lowerName) {
                    return entry.absoluteFilePath().toStdString();
                }
            }
        }

        return "";
    }

    std::shared_ptr<DecodedImage> TextureLoader::decodeImage(const std::string& filepath) {
        if (filepath.empty()) return nullptr;

        stbi_set_flip_vertically_on_load(1);
        int w, h, comp;
        unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &comp, 4);
        if (data) {
            auto img = std::make_shared<DecodedImage>();
            img->width = w;
            img->height = h;
            img->rgba.resize(w * h * 4);
            std::memcpy(img->rgba.data(), data, w * h * 4);
            stbi_image_free(data);
            return img;
        }

        // Fallback to QImage for WebP and extended formats
        QImage qimg;
        if (qimg.load(QString::fromStdString(filepath))) {
            #if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
            qimg = qimg.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
            #else
            qimg = qimg.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
            #endif
            auto img = std::make_shared<DecodedImage>();
            img->width = qimg.width();
            img->height = qimg.height();
            img->rgba.resize(img->width * img->height * 4);
            std::memcpy(img->rgba.data(), qimg.constBits(), img->rgba.size());
            return img;
        }

        return nullptr;
    }

    std::shared_ptr<DecodedImage> TextureLoader::decodeMemory(const uint8_t* buffer, size_t len) {
        if (!buffer || len == 0) return nullptr;

        stbi_set_flip_vertically_on_load(1);
        int w, h, comp;
        unsigned char* data = stbi_load_from_memory(buffer, static_cast<int>(len), &w, &h, &comp, 4);
        if (data) {
            auto img = std::make_shared<DecodedImage>();
            img->width = w;
            img->height = h;
            img->rgba.resize(w * h * 4);
            std::memcpy(img->rgba.data(), data, w * h * 4);
            stbi_image_free(data);
            return img;
        }

        // Fallback to QImage
        QImage qimg;
        if (qimg.loadFromData(buffer, static_cast<int>(len))) {
            #if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
            qimg = qimg.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
            #else
            qimg = qimg.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
            #endif
            auto img = std::make_shared<DecodedImage>();
            img->width = qimg.width();
            img->height = qimg.height();
            img->rgba.resize(img->width * img->height * 4);
            std::memcpy(img->rgba.data(), qimg.constBits(), img->rgba.size());
            return img;
        }

        return nullptr;
    }

} // namespace StudioViewer
