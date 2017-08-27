#include "zthread.h"

#include <QDebug>

ZThread::ZThread(const QStringList &paths, const QSize &size)
{
    imagePaths = paths;
    imageSize.setWidth(size.width());
    imageSize.setHeight(size.height());

    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void ZThread::run()
{
    foreach (const QString &imagePath, imagePaths) {
        QImage image(imagePath);
        QImage scaledImage = image.scaledToWidth(imageSize.width(), Qt::SmoothTransformation);
        emit handleImageFinished(scaledImage);
    }
}
