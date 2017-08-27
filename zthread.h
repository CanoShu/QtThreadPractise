#ifndef ZTHREAD_H
#define ZTHREAD_H

#include <QThread>
#include <QImage>

class ZThread : public QThread
{
    Q_OBJECT
public:
    ZThread(const QStringList &paths, const QSize &size);

signals:
    void handleImageFinished(const QImage &image);

protected:
    void run();

private:
    QStringList imagePaths;
    QSize imageSize;

};

#endif // ZTHREAD_H
