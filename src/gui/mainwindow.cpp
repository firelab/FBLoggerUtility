#include "mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>

#include <fstream>
#include <sstream>
#include <sys/stat.h>

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
    ui->useLegacyDataCheckBox->setCheckState(Qt::Unchecked);

    configFilePath = "";
    burnFilesDirectoryPath = "";

    windTunnelDataExists = false;

    applicationPath = qApp->applicationDirPath() + "/";
    windTunnelDataTablePath = applicationPath + "TG201811071345.txt";

    if(QFileInfo::exists(windTunnelDataTablePath))
    {
        qDebug() << "wind tunnel data exists\n";;
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
        qDebug() << msgText;
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
        qDebug() << "ini file exists\n";
        readIniFile();
        qDebug() << "ini file read\n";
    }
    else
    {
        qDebug() << "ini file does not exist\n";
        QFile file(iniFilePath);
        file.open(QIODevice::ReadWrite | QIODevice::Text);
        qDebug() << "ini file created\n";
        file.close();
        updateIniFile();
        qDebug() << "ini file updated\n";
    }

    ui->configFileLineEdit->setText(configFilePath);
    ui->burnDataDirectoryLineEdit->setText(burnFilesDirectoryPath);

    connect(ui->burnDataDirectoryPushButton, &QPushButton::clicked, this, &MainWindow::burnDataDirectoryBrowsePressed);
    connect(ui->configFilePushButton, &QPushButton::clicked, this, &MainWindow::configFileBrowsePressed);
    connect(ui->convertButton, &QPushButton::clicked, this, &MainWindow::convertPressed);

    workerThread = nullptr;
    loggerDataWorker = nullptr;

    sharedData = new SharedData();
    legacySharedData = new LegacySharedData();
}

MainWindow::~MainWindow()
{
    sharedData->reset();
    delete sharedData;
    legacySharedData->reset();
    delete legacySharedData;
    delete ui;
}

void MainWindow::setUpProgressDialog()
{
    int progressHeight = progressDialog->height();
    int progressWidth = 300;
    connect(progressDialog, &QProgressDialog::canceled, this, &MainWindow::progressCancelClicked);

    progressDialog->resize(progressWidth, progressHeight);
    progressDialog->setMinimumWidth(progressWidth);
    progressDialog->setMinimumDuration(1);
    progressDialog->setAutoClose(false);
    progressDialog->setAutoReset(false);
    progressDialog->setWindowModality(Qt::ApplicationModal);
    progressDialog->cancel();
}

void MainWindow::progressCancelClicked()
{
    qDebug() << "Progress bar cancel clicked!";
    progressDialog->setLabelText("Cancelling conversion...");
    progressDialog->show();
    progressDialog->setValue(0);
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

    useLegacyData = false;
    if(ui->useLegacyDataCheckBox->checkState() == Qt::Checked)
    {
        useLegacyData = true;
    }

    configFilePath = ui->configFileLineEdit->text();
    if(QFileInfo::exists(configFilePath))
    {
        struct stat s;
        if(stat(configFilePath.toStdString().c_str(), &s) == 0)
        {
            if(s.st_mode & S_IFDIR)
            {
                //it's a directory
                QMessageBox msgBox;
                QString msgText = "Config file does not exist\n";
                msgBox.setText(msgText);
                msgBox.exec();
                qDebug() << msgText;
            }
            else if(s.st_mode & S_IFREG)
            {
                //it's a file
                qDebug() << "Config file exists\n";
                configFileExists = true;
                updateIniFile();
            }
        }
        else
        {
            //error
            QMessageBox msgBox;
            QString msgText = "An unkown error occured reading config file\n";
            msgBox.setText(msgText);
            msgBox.exec();
            qDebug() << msgText;
        }
    }
    else
    {
        QMessageBox msgBox;
        QString msgText = "Config file does not exist\n";
        msgBox.setText(msgText);
        msgBox.exec();
        qDebug() << msgText;
    }

    if(QFileInfo::exists(burnFilesDirectoryPath))
    {
        struct stat s;
        if(stat(burnFilesDirectoryPath.toStdString().c_str(), &s) == 0)
        {
            if(s.st_mode & S_IFDIR)
            {
                //it's a directory
                qDebug() << "Burn data directory exists\n";
                burnDataDirectoryExists = true;
                updateIniFile();
            }
            else if(s.st_mode & S_IFREG)
            {
                //it's a file
                QMessageBox msgBox;
                QString msgText = "Burn data directory cannot be a file\n";
                msgBox.setText(msgText);
                msgBox.exec();
                qDebug() << msgText;
            }
        }
        else
        {
            //error
            QMessageBox msgBox;
            QString msgText = "An unkown error occured reading burn data directory\n";
            msgBox.setText(msgText);
            msgBox.exec();
            qDebug() << msgText;
        }
    }
    else
    {
        QMessageBox msgBox;
        QString msgText = "Burn data directory does not exist\n";
        msgBox.setText(msgText);
        msgBox.exec();
        qDebug() << msgText;
    }

    string burnName = ui->burnNameLineEdit->text().toStdString();
    if(burnName != "")
    {
        burnName.erase(std::remove_if(burnName.begin(),
            burnName.end(),
            [](const QChar& c) { return !c.isLetterOrNumber(); }),
            burnName.end());
       
    }

    if(useLegacyData)
    {
        legacySharedData->burnName = burnName;
        legacySharedData->windTunnelDataTablePath = windTunnelDataTablePath.toStdString();
        legacySharedData->configFilePath = configFilePath.toStdString();
        legacySharedData->burnName = "";
        legacySharedData->dataPath = burnFilesDirectoryPath.toStdString();
        legacySharedData->inputFilePathList = new vector<string>();
    }
    else
    {
        sharedData->burnName = burnName;
        sharedData->windTunnelDataTablePath = windTunnelDataTablePath.toStdString();
        sharedData->configFilePath = configFilePath.toStdString();
        sharedData->burnName = "";
        sharedData->dataPath = burnFilesDirectoryPath.toStdString();
        sharedData->inputFilePathList = new vector<string>();
    }

    DIR* dir;
    struct dirent* ent;
    string fileName = "";
    int pos = -1;
    string extension = "";

    if(configFileExists && burnDataDirectoryExists)
    {
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

                        if(useLegacyData)
                        {
                            legacySharedData->inputFilePathList->push_back(burnFilesDirectoryPath.toStdString() + "/" + fileName);
                        }
                        else
                        {
                            sharedData->inputFilePathList->push_back(burnFilesDirectoryPath.toStdString() + "/" + fileName);
                        }
                    }
                }
            }
            closedir(dir);
        }
        else
        {
            /* could not open directory */
            QMessageBox msgBox;
            QString msgText = "Error, could not open burn data directory\n";
            msgBox.setText(msgText);
            msgBox.exec();
            qDebug() << msgText;
        }

        if(ui->createRawDataCheckBox->checkState() == Qt::Checked)
        {
            if(useLegacyData)
            {
                legacySharedData->printRaw = true;
            }
            else
            {
                sharedData->printRaw = true;
            }
          
        }

        int inputFileListSize = 0;
        if(useLegacyData)
        {
            inputFileListSize = legacySharedData->inputFilePathList->size();
        }
        else
        {
            inputFileListSize = sharedData->inputFilePathList->size();
        }

        if(inputFileListSize > 0)
        {
            ui->convertButton->setEnabled(false);
            progressDialog->setLabelText("File conversion in progress...");
            progressDialog->show();

            setUpLoggerDataWorker();
            operate(useLegacyData, legacySharedData, sharedData);
        }
        else
        {
            /* No DAT files found */
            QMessageBox msgBox;
            QString msgText = "Error, no DAT files found in burn data directory\n";
            msgBox.setText(msgText);
            msgBox.exec();
            qDebug() << msgText;
        }
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
    QString msgText = "";

    bool aborted = false;
    bool gpsFileGood = false;
    int totalFilesProcessed = 0;
    string logFilePath = "";
    string gpsFilePath = "";

    if(useLegacyData)
    {
        aborted = legacySharedData->aborted;
        totalFilesProcessed = legacySharedData->totalFilesProcessed;
        gpsFileGood = legacySharedData->gpsFileGood;
        logFilePath = legacySharedData->logFilePath;
        gpsFilePath = legacySharedData->gpsFilePath;
    }
    else
    {
        aborted = sharedData->aborted;
        totalFilesProcessed = sharedData->totalFilesProcessed;
        gpsFileGood = sharedData->gpsFileGood;
        logFilePath = sharedData->logFilePath;
        gpsFilePath = sharedData->gpsFilePath;
    }

    if(!aborted)
    {
        msgText += "Converted a total of " + QString::number(totalFilesProcessed) + " DAT files to csv files to\n";
        msgText += burnFilesDirectoryPath + "\n";
        msgText += "in " + QString::number(elapsedSeconds) + " seconds\n\n";
    }
    else
    {
        msgText += "Conversion aborted by user\n\n";
    }

    if(gpsFileGood)
    {
        msgText += "A gps file was generated at\n" + QString::fromStdString(gpsFilePath) + "\n\n";
    }

    if(logFilePath != "")
    {
        msgText += "A log file was generated at\n" + QString::fromStdString(logFilePath);
    }

    msgBox.setText(msgText);
    msgBox.exec();

    sharedData->reset();
    legacySharedData->reset();
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

        QString useLegacyRawStateText = "false";
        if(ui->useLegacyDataCheckBox->checkState() == Qt::Checked)
        {
            useLegacyRawStateText = "true";
        }

        QString iniFileContents = "";
        iniFileContents += "fb_data_path=" + burnFilesDirectoryPath + "\n";
        iniFileContents += "fb_config_file=" + configFilePath + "\n";
        iniFileContents += "create_raw=" + createRawStateText + "\n";
        iniFileContents += "use_legacy=" + useLegacyRawStateText + "\n";
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
            else if(fieldName == "use_legacy")
            {
                if(fieldData == "true")
                {
                    ui->useLegacyDataCheckBox->setCheckState(Qt::Checked);
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
