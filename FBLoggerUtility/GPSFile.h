#pragma once

#include <string>
#include <fstream>

using std::ofstream;
using std::string;

class GPSFile
{
public:
    GPSFile(string dataPath);
    ~GPSFile();
   
    void PrintGPSFileLinesToFile();
    inline void AddToGPSFile(const string& gpsFileLines)
    {
        gpsFileLines_ += gpsFileLines;
    }

    inline bool IsGPSFileGood()
    {
        bool isGood = false;
        if(!gpsFile_.fail())
        {
            isGood = gpsFile_.good();
        }
        return isGood;
    }

    inline string GetGPSFilePath()
    {
        return gpsFilePath_;
    }

private:
    void PrintGPSFileHeader();

    string dataPath_;
    string gpsFilePath_;
    string gpsFileLines_;
    ofstream gpsFile_;
};
