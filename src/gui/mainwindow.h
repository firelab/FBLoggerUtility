#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QProgressDialog>
#include <QThread>
#include <QWidget>

#include "ui_mainwindow.h"
#include "my_custom_events.h"
#include "logger_data_reader.h"
#include "logger_data_worker.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void closeEvent(QCloseEvent* event);
    void on_actionExit_triggered();
    void configFileBrowsePressed();
    void burnDataDirectoryBrowsePressed();
    void convertPressed();
    void customEvent(QEvent* event);
    void handleProgressEvent(const ProgressUpdateEvent* event);
    void handleResults();
    void threadFinished();
    void progressCancelClicked();

signals:
    void setProgressValue(int currentProgress);
    void operate(SharedData* sharedData);
    void cancelWork();

private:
    void setUpProgressDialog();
    void updateIniFile();
    void readIniFile();
    void setUpLoggerDataWorker();
    void destroyLoggerDataWorker();
    
    bool windTunnelDataExists;

    Ui::MainWindow *ui;
    QString applicationPath;
    QString iniFilePath;
    QString configFilePath;
    QString burnFilesDirectoryPath;
    QString windTunnelDataTablePath;

    SharedData* sharedData;
    LoggerDataWorker* loggerDataWorker;
    QThread* workerThread;

    QProgressDialog* progressDialog;
    QElapsedTimer conversionTimer;
};

#endif // MAINWINDOW_H
