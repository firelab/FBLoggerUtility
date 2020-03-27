#include "mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>

#include <fstream>
#include <sstream>

#ifdef OMP_SUPPORT
#include <omp.h>
#endif

#ifdef _WIN32
#include <win/dirent.h>
#else
#include <dirent.h>
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->createRawDataCheckBox->setCheckState(Qt::Unchecked);

    configFilePath = "";
    burnFilesDirectoryPath = "";

    windTunnelDataExists = false;

    applicationPath = qApp->applicationDirPath() + "/";
    windTunnelDataTablePath = applicationPath + "TG201811071345.txt";

    if(QFileInfo::exists(windTunnelDataTablePath))
    {
        qDebug() << "wind tunnel data exists" << endl;
        windTunnelDataExists = true;
    }
    else
    {
        QMessageBox msgBox;
        QString msgText = "Wind tunnel data does not exist at:\n" + windTunnelDataTablePath + "\n";
        msgText += "Please contact William Chatham at RMRS Fire Sciences Lab for support.\n";
        msgText += "This is unrecoverable error, application will now close\n";
        msgBox.setText(msgText);
        msgBox.exec();
        qDebug() << msgText << endl;
    }

    if(!windTunnelDataExists)
    {
        exit(1);
    }

    progressDialog = new QProgressDialog("File conversion in progress...", "Cancel", 0, 100, this, (windowFlags() & ~Qt::WindowContextHelpButtonHint));
    setUpProgressDialog();

    iniFilePath = applicationPath + "FBLoggerUtility.ini";
    qDebug() << "Application path : " << qApp->applicationDirPath();
    qDebug() << "ini file path : " << iniFilePath;

    if(QFileInfo::exists(iniFilePath))
    {
        qDebug() << "ini file exists" << endl;
        readIniFile();
        qDebug() << "ini file read" << endl;
    }
    else
    {
        qDebug() << "ini file does not exist" << endl;
        QFile file(iniFilePath);
        file.open(QIODevice::ReadWrite | QIODevice::Text);
        qDebug() << "ini file created" << endl;
        file.close();
        updateIniFile();
        qDebug() << "ini file updated" << endl;
    }


    ui->configFileLineEdit->setText(configFilePath);
    ui->burnDataDirectoryLineEdit->setText(burnFilesDirectoryPath);

    connect(ui->burnDataDirectoryPushButton, &QPushButton::clicked, this, &MainWindow::burnDataDirectoryBrowsePressed);
    connect(ui->configFilePushButton, &QPushButton::clicked, this, &MainWindow::configFileBrowsePressed);
    connect(ui->convertButton, &QPushButton::clicked, this, &MainWindow::convertPressed);

    workerThread = nullptr;
    loggerDataWorker = nullptr;

    sharedData = new SharedData();
}

MainWindow::~MainWindow()
{
    resetSharedData();
    delete sharedData;
    delete ui;
}

void MainWindow::setUpProgressDialog()
{
    int progressHeight = progressDialog->height();
    int progressWidth = 300;
    connect(progressDialog, &QProgressDialog::canceled, this, &MainWindow::progressCancelClicked);

    progressDialog->resize(progressWidth, progressHeight);
    progressDialog->setMinimumDuration(1);
    progressDialog->setAutoClose(false);
    progressDialog->setAutoReset(false);
    progressDialog->setWindowModality(Qt::ApplicationModal);
    progressDialog->cancel();
}

void MainWindow::progressCancelClicked()
{
    qDebug() << "Progress bar cancel clicked!";
    QApplication::postEvent(loggerDataWorker, new CancelWorkEvent());
}

void MainWindow::configFileBrowsePressed()
{
    qDebug() << "Burn directory selection button pressed";
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.exec();
   
    QStringList selectedFiles = dialog.selectedFiles();
    if(selectedFiles.size() > 0)
    {
        configFilePath = selectedFiles.at(0);
        ui->configFileLineEdit->setText(configFilePath);
        qDebug() << "Selected config file is" << configFilePath;
        updateIniFile();
    }
}

void MainWindow::burnDataDirectoryBrowsePressed()
{
    qDebug() << "Burn directory selection button pressed";
    QFileDialog dialog;
    dialog.setDirectory("");
    dialog.setFileMode(QFileDialog::Directory);
    dialog.exec();

    QStringList selectedFiles = dialog.selectedFiles();
    if(selectedFiles.size() > 0)
    {
        burnFilesDirectoryPath = selectedFiles.at(0);
        ui->burnDataDirectoryLineEdit->setText(burnFilesDirectoryPath);
        qDebug() << "Selected directory is" << burnFilesDirectoryPath;
        updateIniFile();
    }
}

void MainWindow::convertPressed()
{
    conversionTimer.start();

    bool configFileExists = false;
    bool burnDataDirectoryExists = false;

    configFilePath = ui->configFileLineEdit->text();
    if(QFileInfo::exists(configFilePath))
    {
        qDebug() << "Config file exists" << endl;
        configFileExists = true;
        updateIniFile();
    }
    else
    {
        QMessageBox msgBox;
        QString msgText = "Config file does not exist";
        msgBox.setText(msgText);
        msgBox.exec();
        qDebug() << msgText << endl;
    }
    if(QFileInfo::exists(burnFilesDirectoryPath))
    {
        qDebug() << "Burn data directory exists" << endl;
        burnDataDirectoryExists = true;
        updateIniFile();
    }
    else
    {
        QMessageBox msgBox;
        QString msgText = "Burn data directory does not exist";
        msgBox.setText(msgText);
        msgBox.exec();
        qDebug() << msgText << endl;
    }

    string burnName = ui->burnNameLineEdit->text().toStdString();
    if(burnName != "")
    {
        burnName.erase(std::remove_if(burnName.begin(),
            burnName.end(),
            [](const QChar& c) { return !c.isLetterOrNumber(); }),
            burnName.end());
        sharedData->burnName = burnName;
    }

    sharedData->windTunnelDataTablePath = windTunnelDataTablePath.toStdString();
    sharedData->configFilePath = configFilePath.toStdString();
    sharedData->burnName = "";
    sharedData->dataPath = burnFilesDirectoryPath.toStdString();
    sharedData->inputFilePathList = new vector<string>();
    //vector<string> datFileNameList;
    DIR* dir;
    struct dirent* ent;
    string fileName = "";
    int pos = -1;
    string extension = "";

    if((dir = opendir(burnFilesDirectoryPath.toStdString().c_str())) != nullptr)
    {
        /* read all the files and directories within directory */
        while((ent = readdir(dir)) != nullptr)
        {
            fileName = ent->d_name;

            pos = fileName.find_last_of('.');
            if(pos != string::npos)
            {
                extension = fileName.substr(pos, fileName.size());
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                if(extension == ".dat")
                {
                    //datFileNameList.push_back(directoryName);
                    sharedData->inputFilePathList->push_back(burnFilesDirectoryPath.toStdString() + "/" + fileName);
                }
            }
        }
        closedir(dir);
    }
    else
    {
        /* could not open directory */
        QMessageBox msgBox;
        QString msgText = "Error, could not open burn data directory";
        msgBox.setText(msgText);
        msgBox.exec();
        qDebug() << msgText << endl;
    }

    if(ui->createRawDataCheckBox->checkState() == Qt::Checked)
    {
        sharedData->printRaw = true;
    }

    if(configFileExists && burnDataDirectoryExists)
    {
        ui->convertButton->setEnabled(false);

        progressDialog->show();

        setUpLoggerDataWorker();
        operate(sharedData);
    }
}

void MainWindow::threadFinished()
{
    qDebug() << "Worker thread finished";
}

void MainWindow::setUpLoggerDataWorker()
{
    qDebug() << "Creating logger data worker";
    loggerDataWorker = new LoggerDataWorker(this);
    workerThread = new QThread;
    loggerDataWorker->moveToThread(workerThread);
    connect(workerThread, &QThread::finished, this, &MainWindow::threadFinished);
    connect(workerThread, &QThread::finished, loggerDataWorker, &QObject::deleteLater);
    connect(this, &MainWindow::operate, loggerDataWorker, &LoggerDataWorker::doWork);
    connect(loggerDataWorker, &LoggerDataWorker::resultReady, this, &MainWindow::handleResults);

    workerThread->start();
}

void MainWindow::destroyLoggerDataWorker()
{
    if((loggerDataWorker != nullptr) && (workerThread != nullptr))
    {
        workerThread->quit(); // implicit call to FireWorker's destructor via signal/slot
        workerThread->wait();
    }

    workerThread = nullptr;
    loggerDataWorker = nullptr;
}

void MainWindow::resetSharedData()
{
    sharedData->totalFilesProcessed = 0;
    sharedData->totalInvalidInputFiles = 0;
    sharedData->totalErrors = 0;

    sharedData->aborted = false;
    sharedData->printRaw = false;
    sharedData->configFileGood = false;
    sharedData->gpsFileGood = false;

    sharedData->burnName = "";
    sharedData->logFilePath = "";
    sharedData->gpsFilePath = "";
    sharedData->configFilePath = "";
    sharedData->dataPath = "";
    sharedData->windTunnelDataTablePath = "";
 
    if(sharedData->configMap != nullptr)
    {
        sharedData->configMap->clear();
        delete sharedData->configMap;
        sharedData->configMap = nullptr;
    }
    if(sharedData->heatFlux_X_VoltageOffsetMap != nullptr)
    {
        sharedData->heatFlux_X_VoltageOffsetMap->clear();
        delete sharedData->heatFlux_X_VoltageOffsetMap;
        sharedData->heatFlux_X_VoltageOffsetMap = nullptr;
    }
    if(sharedData->heatFlux_Y_VoltageOffsetMap != nullptr)
    {
        sharedData->heatFlux_Y_VoltageOffsetMap->clear();
        delete sharedData->heatFlux_Y_VoltageOffsetMap;
        sharedData->heatFlux_Y_VoltageOffsetMap = nullptr;
    }
    if(sharedData->heatFlux_Z_VoltageOffsetMap != nullptr)
    {
        sharedData->heatFlux_Z_VoltageOffsetMap->clear();
        delete sharedData->heatFlux_Z_VoltageOffsetMap;
        sharedData->heatFlux_Z_VoltageOffsetMap = nullptr;
    }
    if(sharedData->sensorBearingMap != nullptr)
    {
        sharedData->sensorBearingMap->clear();
        delete sharedData->sensorBearingMap;
        sharedData->sensorBearingMap = nullptr;
    }
    if(sharedData->statsFileMap != nullptr)
    {
        sharedData->statsFileMap->clear();
        delete sharedData->statsFileMap;
        sharedData->statsFileMap = nullptr;
    }
    if(sharedData->angles != nullptr)
    {
        sharedData->angles->clear();
        delete sharedData->angles;
        sharedData->angles = nullptr;
    }
    if(sharedData->ReynloldsNumbers != nullptr)
    {
        sharedData->ReynloldsNumbers->clear();
        delete sharedData->ReynloldsNumbers;
        sharedData->ReynloldsNumbers = nullptr;
    }
    if(sharedData->pressureCoefficients != nullptr)
    {
        sharedData->pressureCoefficients->clear();
        delete sharedData->pressureCoefficients;
        sharedData->pressureCoefficients = nullptr;
    }
    if(sharedData->inputFilePathList != nullptr)
    {
        sharedData->inputFilePathList->clear();
        delete sharedData->inputFilePathList;
        sharedData->inputFilePathList = nullptr;
    }
}


void MainWindow::customEvent(QEvent* event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == MY_PROGRESS_UPDATE_EVENT)
    {
        handleProgressEvent(static_cast<ProgressUpdateEvent*>(event));
    }

    // use more else ifs to handle other custom events
}

void MainWindow::handleProgressEvent(const ProgressUpdateEvent* event)
{
    int currentProgress = event->getProgress();
    //qDebug() << "current progress is " << currentProgress;
    if(progressDialog != nullptr)
    {
        if(progressDialog->value() < currentProgress)
        {
            progressDialog->setValue(currentProgress);
        }
    }
    else
    {
        //qDebug() << "Error: Progress dialog does not exist!";
    }
}

void MainWindow::handleResults()
{
    progressDialog->hide();
    progressDialog->setValue(0);
    destroyLoggerDataWorker();

    double elapsedMilliseconds = conversionTimer.elapsed();
    double elapsedSeconds = elapsedMilliseconds / 1000.0;
    elapsedSeconds = floor(elapsedSeconds * 100.0 + 0.5) / 100.0;

    // Display Messages Here
    QMessageBox msgBox;
    QString msgText = "Converted a total of " + QString::number(sharedData->totalFilesProcessed) + " .DAT files to .csv files to\n";
    msgText += burnFilesDirectoryPath + "\n";
    msgText += "in " + QString::number(elapsedSeconds) + " seconds\n";
    msgBox.setText(msgText);
    msgBox.exec();

    resetSharedData();
    ui->convertButton->setEnabled(true);
}

void MainWindow::updateIniFile()
{
    configFilePath = ui->configFileLineEdit->text();
    burnFilesDirectoryPath = ui->burnDataDirectoryLineEdit->text();

    if(QFileInfo::exists(iniFilePath))
    {
        QFile file(iniFilePath);
        file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);

        QString createRawStateText = "false";
        if(ui->createRawDataCheckBox->checkState() == Qt::Checked)
        {
            createRawStateText = "true";
        }
        QString iniFileContents = "";
        iniFileContents += "fb_data_path=" + burnFilesDirectoryPath + "\n";
        iniFileContents += "fb_config_file=" + configFilePath + "\n";
        iniFileContents += "create_raw=" + createRawStateText + "\n";
        file.write(iniFileContents.toUtf8());
        file.close();
    }
}

void MainWindow::readIniFile()
{
    bool isDataPathValid;
    bool isConfigurationTypeValid;

    QString fieldName = "";
    QString fieldData = "";

    QFile inputFile(iniFilePath);
    if(inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        while(!in.atEnd())
        {
            QString line = in.readLine();
            QStringList tokens = line.split("=");
            if(tokens.size() == 2)
            {
                fieldName = tokens.at(0);
                if(tokens.at(1).size() > 0)
                {
                    fieldData = tokens.at(1);
                }
                else
                {
                    fieldData = "";
                }
            }
            if(fieldName == "fb_data_path")
            {
                burnFilesDirectoryPath = fieldData;
            }
            else if(fieldName == "fb_config_file")
            {
                configFilePath = fieldData;
            }
            else if(fieldName == "create_raw")
            {
                if(fieldData == "true")
                {
                    ui->createRawDataCheckBox->setCheckState(Qt::Checked);
                }
            }
        }
    }
    inputFile.close();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    updateIniFile();
    event->accept();
}

void MainWindow::on_actionExit_triggered()
{
    updateIniFile();
    this->close();
}
