#include "loaders/ModelLoaderWorker.h"
#include "loaders/ObjLoader.h"
#include "loaders/GltfLoader.h"
#include "loaders/Texture.h"
#include "core/I18n.h"
#include "core/Constants.h"
#include <QFileInfo>
#include <QElapsedTimer>
#include <QtConcurrent>

namespace StudioViewer {

    ModelLoaderWorker::ModelLoaderWorker(const QString& filepath, QObject* parent)
    : QThread(parent), m_filepath(filepath) {}

    void ModelLoaderWorker::cancel() {
        m_cancelled = true;
    }

    void ModelLoaderWorker::run() {
        QElapsedTimer timer;
        timer.start();

        try {
            if (m_cancelled) return;

            QFileInfo fi(m_filepath);
            QString ext = fi.suffix().toLower();
            emit progress(I18n::tr("status_reading_geo", {{"ext", ext.toUpper()}}));

            ModelData model;
            if (ext == "obj") {
                model = ObjLoader::load(m_filepath.toStdString());
            } else if (ext == "glb" || ext == "gltf") {
                model = GltfLoader::load(m_filepath.toStdString());
            } else {
                emit error(I18n::tr("error_unsupported_format"));
                return;
            }

            if (m_cancelled) return;

            // Collect unique external textures for parallel decompression
            std::unordered_map<std::string, std::string> uniqueTextures;
            for (const auto& b : model.batches) {
                if (!b.diffusePath.empty()) uniqueTextures[b.diffuseKey] = b.diffusePath;
                if (!b.normalPath.empty()) uniqueTextures[b.normalKey] = b.normalPath;
                if (!b.mrPath.empty()) uniqueTextures[b.mrKey] = b.mrPath;
                if (!b.occlusionPath.empty()) uniqueTextures[b.occlusionKey] = b.occlusionPath;
                if (!b.emissivePath.empty()) uniqueTextures[b.emissiveKey] = b.emissivePath;
            }

            if (!uniqueTextures.empty()) {
                emit progress(I18n::tr("status_decompress_tex", {{"count", QString::number(uniqueTextures.size())}}));

                std::vector<std::pair<std::string, std::string>> texList(uniqueTextures.begin(), uniqueTextures.end());

                // Multithreaded decompression across all CPU worker threads
                auto decodedResults = QtConcurrent::blockingMapped(texList, [](const std::pair<std::string, std::string>& item) {
                    return std::make_pair(item.first, TextureLoader::decodeImage(item.second));
                });

                if (m_cancelled) return;

                std::unordered_map<std::string, std::shared_ptr<DecodedImage>> decodedMap;
                for (const auto& r : decodedResults) decodedMap[r.first] = r.second;

                for (auto& b : model.batches) {
                    if (decodedMap.count(b.diffuseKey)) b.diffuseDecoded = decodedMap[b.diffuseKey];
                    if (decodedMap.count(b.normalKey)) b.normalDecoded = decodedMap[b.normalKey];
                    if (decodedMap.count(b.mrKey)) b.mrDecoded = decodedMap[b.mrKey];
                    if (decodedMap.count(b.occlusionKey)) b.occlusionDecoded = decodedMap[b.occlusionKey];
                    if (decodedMap.count(b.emissiveKey)) b.emissiveDecoded = decodedMap[b.emissiveKey];
                }
            }

            if (m_cancelled) return;

            model.loadTime = timer.elapsed() / 1000.0;
            emit finished(std::make_shared<ModelData>(std::move(model)));

        } catch (const std::exception& e) {
            if (!m_cancelled) emit error(QString::fromUtf8(e.what()));
        }
    }

} // namespace StudioViewer
