#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QMutex>
#include <QtWidgets>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void openImage();

    void handleSingleThread();
    void onHandleSingleThread(const QImage &image);

    void handleMulitThread();
    void onHandleMultiThread(const QImage &image);

    void handleConcurrent();

private:
    QList<QImage> asyncThumbnailCreatedByRun();
    QImage asyncThumbnailCreatedByMap();  // TODO: ?

    void handleConcurrentByRun();
    void handleConcurrentByMap();


private:
    Ui::MainWindow *ui;

    QTime time;
    QMutex mutex;

    QProgressDialog progressDialog;
};

#endif // MAINWINDOW_H
