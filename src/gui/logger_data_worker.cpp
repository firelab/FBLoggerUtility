#include "logger_data_worker.h"
#include "fb_logger_log_file.h"
#include "gps_file.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <QCoreApplication>
#include <QDebug>

#include "mainwindow.h"

LoggerDataWorker::LoggerDataWorker(QWidget* myParent)
{
    isWorkCancelled = false;
    parent = myParent;
}

LoggerDataWorker::~LoggerDataWorker()
{
    qDebug() << "Destroying worker";
}

void LoggerDataWorker::customEvent(QEvent *event)
{
    // When we get here, we've crossed the thread boundary and are now
    // executing in the Qt object's thread

    if(event->type() == MY_CANCEL_WORK_EVENT)
    {
        handleCancelWorkEvent(static_cast<CancelWorkEvent*>(event));
    }

    // use more else ifs to handle other custom events
}

void LoggerDataWorker::handleCancelWorkEvent(const CancelWorkEvent *event)
{
    isWorkCancelled = true;
}

void LoggerDataWorker::doWork(SharedData* sharedData)
{
    /* ... here is the expensive or blocking operation ... */

    bool isGoodWindTunnelTable = false;
    
    string configFilePath = sharedData->configFilePath;
    clock_t startClock = clock();

    LoggerDataReader globalLoggerDataReader(*sharedData);
    FBLoggerLogFile logFile(sharedData->dataPath);
    GPSFile gpsFile(sharedData->dataPath);
    globalLoggerDataReader.SetConfigFile(sharedData->configFilePath);
   
    int totalNumberOfFiles = 0;

    vector<string> logFileLinesPerFile;
    vector<string> gpsFileLinesPerFile;

    // Shared accumulated data
    int totalInvalidInputFiles = 0;
    int totalFilesProcessed = 0;
    int totalNumErrors = 0;
    float flProgress = 0;

    int num_threads = 1;

    // Check log file, check config file, process input files, create output files 
    if(logFile.IsLogFileGood())
    {
        globalLoggerDataReader.CheckConfig();
        // Load data in from wind tunnel data table
        globalLoggerDataReader.SetWindTunnelDataTablePath(sharedData->windTunnelDataTablePath);
        isGoodWindTunnelTable = globalLoggerDataReader.CreateWindTunnelDataVectors();
        
        logFile.logFileLines_ = globalLoggerDataReader.GetLogFileLines();

        if(globalLoggerDataReader.IsConfigFileValid() && isGoodWindTunnelTable)
        {
            totalNumberOfFiles = globalLoggerDataReader.GetNumberOfInputFiles();
            SharedData shareDataLocal = *sharedData;
            globalLoggerDataReader.GetSharedData(shareDataLocal);

            if(totalNumberOfFiles > 0)
            {
                for(int i = 0; i < totalNumberOfFiles; i++)
                {
                    logFileLinesPerFile.push_back("");
                }

                for(int i = 0; i < totalNumberOfFiles; i++)
                {
                    gpsFileLinesPerFile.push_back("");
                }

#ifdef _OPENMP
                num_threads = omp_get_num_procs() - 1;
                if(num_threads < 1)
                {
                    num_threads = 1;
                }
                omp_set_num_threads(num_threads);

#pragma omp parallel for schedule(dynamic, 1) shared(totalFilesProcessed, totalInvalidInputFiles, totalNumErrors, flProgress, logFileLinesPerFile, gpsFileLinesPerFile)  
#endif
                for(int i = 0; i < totalNumberOfFiles; i++)
                {
                    // Check for kill
                    QCoreApplication::processEvents();

                    if(!isWorkCancelled)
                    {
                        // Need to pass needed maps and vectors to a FBLoggerDataReader
                        LoggerDataReader localLoggerDataReader(shareDataLocal);
                        if(shareDataLocal.printRaw == true)
                        {
                            localLoggerDataReader.SetPrintRaw(true);
                        }
                        localLoggerDataReader.ProcessSingleDataFile(i);

                        // Check for kill
                        QCoreApplication::processEvents();

#ifdef _OPENMP
#pragma omp critical(updateLogAndProgress)
#endif
                        // critical section 
                        {
                                totalFilesProcessed += localLoggerDataReader.GetNumFilesProcessed();
                                totalInvalidInputFiles += localLoggerDataReader.GetNumInvalidFiles();
                                totalNumErrors += localLoggerDataReader.GetNumErrors();
                                logFileLinesPerFile[i] = localLoggerDataReader.GetLogFileLines();
                                gpsFileLinesPerFile[i] = localLoggerDataReader.GetGPSFileLines();

                                if(!isWorkCancelled)
                                {
                                    flProgress = (static_cast<float>(totalFilesProcessed) / totalNumberOfFiles) * (static_cast<float>(100.0));
                                    QApplication::postEvent(parent, new ProgressUpdateEvent((int)flProgress));
                                }
                                else
                                {
                                    QApplication::postEvent(parent, new ProgressUpdateEvent((int)0));
                                }
                        }
                       
                        // end critical section
                    }
                }
            }
        }
    }

    QApplication::postEvent(parent, new ProgressUpdateEvent(100));

    if(isWorkCancelled)
    {
        sharedData->aborted = true;
        double totalTimeInSeconds = ((double)clock() - (double)startClock) / (double)CLOCKS_PER_SEC;

        if(logFile.IsLogFileGood())
        {
            sharedData->logFilePath = sharedData->dataPath + "/" + "log_file.txt";
            globalLoggerDataReader.ReportAbort();
            logFile.AddToLogFile(globalLoggerDataReader.GetLogFileLines());
            logFile.PrintFinalReportToLogFile(totalTimeInSeconds, totalInvalidInputFiles, totalFilesProcessed, totalNumErrors);
        }
        else
        {
            sharedData->logFilePath = "";
        }
    }
    else
    {
        double totalTimeInSeconds = ((double)clock() - (double)startClock) / (double)CLOCKS_PER_SEC;
        int numFilesConverted = totalFilesProcessed - totalInvalidInputFiles;

        if(logFile.IsLogFileGood())
        {
            sharedData->logFilePath = sharedData->dataPath + "/" + "log_file.txt";
            for(int i = 0; i < logFileLinesPerFile.size(); i++)
            {
                logFile.AddToLogFile(logFileLinesPerFile[i]);
            }
            logFile.PrintFinalReportToLogFile(totalTimeInSeconds, totalInvalidInputFiles, totalFilesProcessed, totalNumErrors);
        }
        else
        {
            sharedData->logFilePath = "";
        }

        if(gpsFile.IsGPSFileGood())
        {
            sharedData->gpsFilePath = sharedData->dataPath + "/" + sharedData->burnName + "gps_file.txt";
            for(int i = 0; i < gpsFileLinesPerFile.size(); i++)
            {
                gpsFile.AddToGPSFile(gpsFileLinesPerFile[i]);
            }
            gpsFile.PrintGPSFileLinesToFile();
        }

        if(!isGoodWindTunnelTable)
        {
            //text = _T("There are errors in the wind tunnel data table file");
            //caption = ("Error: Invalid wind tunnel data table file");
        }

        if(globalLoggerDataReader.IsConfigFileValid())
        {
            sharedData->configFileGood = true;
        }

        if(totalFilesProcessed > 0)
        {
            sharedData->totalFilesProcessed = totalFilesProcessed;
        }

        if(totalInvalidInputFiles > 0)
        {
            sharedData->totalInvalidInputFiles = totalInvalidInputFiles;
        }

        if(gpsFile.IsGPSFileGood())
        {
            sharedData->gpsFileGood = true;
        }
    }

    emit resultReady();
}

void LoggerDataWorker::cancelWork()
{
    qDebug() << "Work cancelled!";
    isWorkCancelled = true;
}
