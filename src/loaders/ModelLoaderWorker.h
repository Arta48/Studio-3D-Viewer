/**
 * @file ModelLoaderWorker.h
 * @brief Asynchronous worker thread executing file I/O and multithreaded decompression.
 */

#pragma once

#include <QThread>
#include <QString>
#include <memory>
#include <atomic>
#include "core/Models.h"

namespace StudioViewer {

    class ModelLoaderWorker : public QThread {
        Q_OBJECT
    public:
        explicit ModelLoaderWorker(const QString& filepath, QObject* parent = nullptr);
        void cancel();

    signals:
        void progress(const QString& msg);
        void finished(std::shared_ptr<ModelData> data);
        void error(const QString& errMsg);

    protected:
        void run() override;

    private:
        QString m_filepath;
        std::atomic<bool> m_cancelled{false};
    };

} // namespace StudioViewer
