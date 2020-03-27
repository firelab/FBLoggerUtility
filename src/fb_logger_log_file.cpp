#include "fb_logger_log_file.h"

#include <QDateTime>

#include "string_utility.h"

FBLoggerLogFile::FBLoggerLogFile(string dataPath)
    :logFilePath_(dataPath + "\\log_file.txt"),
    logFile_(logFilePath_, std::ios::out)
{
    logFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
   
    logFileLines_ = "";
    dataPath_ = dataPath;
    // Check for output file's existence
    if (logFile_.fail())
    {
        logFile_.close();
        logFile_.clear();

        QTime systemTime = QTime::currentTime();
    //    GetLocalTime(&systemTime);

    //    string dateTimeString = MakeStringWidthTwoFromInt(systemTime.wDay) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + std::to_string(systemTime.wYear) +
    //        "_" + MakeStringWidthTwoFromInt(systemTime.wHour) + "H" + MakeStringWidthTwoFromInt(systemTime.wMinute) + "M" +
    //        MakeStringWidthTwoFromInt(systemTime.wSecond) + "S";

    //    string logFileNoExtension = dataPath + "log_file";
    //    logFileLines_ += "ERROR: default log file at " + logFilePath_ + " already opened or otherwise unwritable\n";
    //    logFilePath_ = logFileNoExtension + "_" + dateTimeString + ".txt";
    //    logFile_.open(logFilePath_, std::ios::out);
    }
    if (logFile_.fail())
    {
        // Something really went wrong here
        logFile_.close();
        logFile_.clear();
    }
}

FBLoggerLogFile::~FBLoggerLogFile()
{
    if (logFile_.is_open())
    {
        logFile_.close();
    }
    logFile_.clear();
}

string FBLoggerLogFile::GetLogFilePath()
{
    return logFilePath_;
}

void FBLoggerLogFile::PrintFinalReportToLogFile(const double totalTimeInSeconds, const int numInvalidFiles, const int numFilesProcessed_, const int numErrors_)
{
    string dataPathOutput = dataPath_.substr(0, dataPath_.size());
    if (logFile_.good() && logFileLines_ == "" && numInvalidFiles == 0)
    {
        if ((numFilesProcessed_ > 0) && (numErrors_ == 0))
        {
            logFileLines_ += "Successfully processed " + std::to_string(numFilesProcessed_) + " DAT files in " + std::to_string(totalTimeInSeconds) + " seconds into \"" + dataPathOutput + "\" " + GetMyLocalDateTimeString() + "\n";
        }
        else if ((numFilesProcessed_ > 0) && (numErrors_ > 0))
        {
            string errorOrErrors = "";
            if(numErrors_ == 1)
            {
                errorOrErrors = "error";
            }
            else
            {
                errorOrErrors = "errors";
            }
            logFileLines_ += "Processed " + std::to_string(numFilesProcessed_) + " DAT files with " + std::to_string(numErrors_) + " " + errorOrErrors + " in " + std::to_string(totalTimeInSeconds) + " seconds in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        else
        {
            logFileLines_ += "No DAT files found in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        logFile_ << logFileLines_;
    }
    else
    {
        if ((numFilesProcessed_ > 0) && (numErrors_ > 0))
        {
            logFileLines_ += "\nProcessed " + std::to_string(numFilesProcessed_) + " DAT files with " + std::to_string(numErrors_) + " errors in " + std::to_string(totalTimeInSeconds) + " seconds in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        logFile_ << logFileLines_;
    }
    if (logFile_.is_open())
    {
        logFile_.close();
    }
}
