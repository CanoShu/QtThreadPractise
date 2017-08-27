#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtConcurrent>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QMutexLocker>

#include "zthread.h"

using namespace QtConcurrent;

const int tableRows = 50;
const int tableColumns = 2;

QSize g_size(100, 100);

QImage scaled(const QImage &image)
{
    return image.scaledToWidth(g_size.width(), Qt::SmoothTransformation);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btn_load, SIGNAL(clicked()), this, SLOT(openImage()));
    connect(ui->btn_singleThread, SIGNAL(clicked()), this, SLOT(handleSingleThread()));
    connect(ui->btn_multiThread, SIGNAL(clicked()), this, SLOT(handleMulitThread()));
    connect(ui->btn_concurrent, SIGNAL(clicked()), this, SLOT(handleConcurrent()));

    ui->tableWidget->setRowCount(tableRows);
    ui->tableWidget->setColumnCount(tableColumns);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (int i = 0; i < tableRows; ++i) {
        ui->tableWidget->setRowHeight(i, ui->tableWidget->width() / 2);
    }

    progressDialog.setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setWindowTitle("Please Wait");
    progressDialog.setLabelText(QString("Progressing using %1 thread(s)..."
                                        "\nCreating thumbnail.....").arg(QThread::idealThreadCount()));
    progressDialog.setCancelButton(0);
    progressDialog.setMinimumDuration(0);
    progressDialog.setRange(0, tableRows * tableColumns);
    progressDialog.reset();  // hide the first show?

    ui->lineEdit_path->setText(":/images/galactic_earth_4k_8k-wide.jpg");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openImage()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Choose Image"));
    ui->lineEdit_path->setText(path);
}

void MainWindow::handleSingleThread()
{
    ui->tableWidget->clear();

    QStringList paths;
    for (int i = 0; i < tableRows * tableColumns; ++i) {
        paths.append(ui->lineEdit_path->text());
    }

    QSize size(ui->tableWidget->width() / 2, ui->tableWidget->width() / 2);
    ZThread *thread = new ZThread(paths, size);
    connect(thread, SIGNAL(handleImageFinished(QImage)), this, SLOT(onHandleSingleThread(QImage)));

    connect(thread, SIGNAL(finished()), &progressDialog, SLOT(reset()));

    time.restart();
    thread->start();

    progressDialog.setValue(0);
}

void MainWindow::onHandleSingleThread(const QImage &image)
{
    for (int row = 0; row < tableRows; ++row) {
        for (int column = 0; column < tableColumns; ++column) {
            if (ui->tableWidget->item(row, column) == NULL) {
                QTableWidgetItem *item = new QTableWidgetItem;
                item->setData(Qt::DecorationRole, QPixmap::fromImage(image));
                ui->tableWidget->setItem(row, column, item);

                progressDialog.setValue(progressDialog.value() + 1);

                if (row == (tableRows - 1) && column == (tableColumns - 1) ) {
                    ui->textBrowser->append(QString(tr("Handle singleThread elapsed: %1 milliseconds.")).arg(time.elapsed()));
                }
                return;
            }
        }
    }
}

void MainWindow::handleMulitThread()
{
    ui->tableWidget->clear();

    time.restart();

    for (int i = 0; i < tableRows * tableColumns; ++i) {
        QSize size(ui->tableWidget->width() / 2, ui->tableWidget->width() / 2);
        ZThread *thread = new ZThread(QStringList() << ui->lineEdit_path->text(), size);
        connect(thread, SIGNAL(handleImageFinished(QImage)), this, SLOT(onHandleMultiThread(QImage)));
        thread->start();
    }

    progressDialog.setValue(0);
}

void MainWindow::onHandleMultiThread(const QImage &image)
{
    //    QMutexLocker locker(&mutex);  // TODO: why deadlock when mulitThread?

    for (int row = 0; row < tableRows; ++row) {
        for (int column = 0; column < tableColumns; ++column) {
            if (ui->tableWidget->item(row, column) == NULL) {
                QTableWidgetItem *item = new QTableWidgetItem;
                item->setData(Qt::DecorationRole, QPixmap::fromImage(image));
                ui->tableWidget->setItem(row, column, item);

                progressDialog.setValue(progressDialog.value() + 1);

                if (row == (tableRows - 1) && column == (tableColumns - 1) ) {
                    progressDialog.reset();
                    ui->textBrowser->append(QString(tr("Handle multiThread elapsed: %1 milliseconds.")).arg(time.elapsed()));
                }
                return;
            }
        }
    }
}

void MainWindow::handleConcurrent()
{
    ui->tableWidget->clear();  // TODO: why can't clear immediately?

//    handleConcurrentByRun();
    handleConcurrentByMap();

}

QList<QImage> MainWindow::asyncThumbnailCreatedByRun()
{
    QList<QImage> images;
    for (int row = 0; row < tableRows; ++row) {
        for (int column = 0; column < tableColumns; ++column) {
            QImage image(ui->lineEdit_path->text());
            QSize size(ui->tableWidget->width() / 2, ui->tableWidget->width() / 2);
            images.append(image.scaledToWidth(size.width(), Qt::SmoothTransformation));
        }
    }

    return images;
}


void MainWindow::handleConcurrentByRun()
{
    // Create a QFutureWatcher and connect signals and slots.
    QFutureWatcher<QList<QImage> > futureWatcher;
    connect(&futureWatcher, SIGNAL(finished()), &progressDialog, SLOT(reset()));
    connect(&progressDialog, SIGNAL(canceled()), &futureWatcher, SLOT(cancel()));
    connect(&futureWatcher, SIGNAL(progressRangeChanged(int,int)), &progressDialog, SLOT(setRange(int,int)));
    connect(&futureWatcher, SIGNAL(progressValueChanged(int)), &progressDialog, SLOT(setValue(int)));

    time.restart();

    // Start the computation.
    futureWatcher.setFuture(QtConcurrent::run(this, &MainWindow::asyncThumbnailCreatedByRun));

    // Display the dialog and start the event loop.
    progressDialog.exec();

    futureWatcher.waitForFinished();

    ui->textBrowser->append(QString(tr("Handle concurrent elapsed: %1 milliseconds.")).arg(time.elapsed()));

    // Handle result
    for (int i = 0; i < futureWatcher.result().count(); ++i) {
        progressDialog.setValue(progressDialog.value() + 1);
        int row = i / tableColumns;
        int column = i % tableColumns;
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setData(Qt::DecorationRole, QPixmap::fromImage(futureWatcher.result().at(i)));
        ui->tableWidget->setItem(row, column, item);
    }
    progressDialog.reset();
}

void MainWindow::handleConcurrentByMap()
{
    // Create a QFutureWatcher and connect signals and slots.
    QFutureWatcher<QImage> futureWatcher;
    connect(&futureWatcher, SIGNAL(finished()), &progressDialog, SLOT(reset()));
    connect(&progressDialog, SIGNAL(canceled()), &futureWatcher, SLOT(cancel()));
    connect(&futureWatcher, SIGNAL(progressRangeChanged(int,int)), &progressDialog, SLOT(setRange(int,int)));
    connect(&futureWatcher, SIGNAL(progressValueChanged(int)), &progressDialog, SLOT(setValue(int)));

    time.restart();

    // Prepare the vector.
    QList<QImage> images;
    for (int i = 0; i < tableRows * tableColumns; ++i) {
        QImage image(ui->lineEdit_path->text());
        images.append(image);
    }

    // Start the computation.
    g_size = QSize(ui->tableWidget->width() / 2, ui->tableWidget->width() / 2);
    futureWatcher.setFuture(QtConcurrent::mapped(images, scaled));

    futureWatcher.waitForFinished();
    // Display the dialog and start the event loop.
    progressDialog.exec();


    ui->textBrowser->append(QString(tr("Handle concurrent elapsed: %1 milliseconds.")).arg(time.elapsed()));

    // Handle result
    for (int i = 0; i < tableRows * tableColumns; ++i) {
        progressDialog.setValue(progressDialog.value() + 1);
        int row = i / tableColumns;
        int column = i % tableColumns;
        QTableWidgetItem *item = new QTableWidgetItem;
        item->setData(Qt::DecorationRole, QPixmap::fromImage(futureWatcher.resultAt(i)));
        ui->tableWidget->setItem(row, column, item);
    }
    progressDialog.reset();
}
