#pragma once

#include <string>
#include <fstream>

using std::ofstream;
using std::string;

class FBLoggerLogFile
{
public:
    FBLoggerLogFile(string dataPath);
    ~FBLoggerLogFile();

    inline void AddToLogFile(const string& logFileLines)
    {
        logFileLines_ += logFileLines;
    }

    inline bool IsLogFileGood()
    {
        bool isGood = false;
        if (!logFile_.fail())
        {
            isGood = logFile_.good();
        }
        return isGood;
    }

    void PrintFinalReportToLogFile(const double totalTimeInSeconds, const int numInvalidFiles, const int numFilesProcessed_, const int numErrors_);

    string GetLogFilePath();

    string dataPath_;
    string logFilePath_;
    string logFileLines_;
    ofstream logFile_;
};
