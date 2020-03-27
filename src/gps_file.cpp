#include "gps_file.h"

GPSFile::GPSFile(string dataPath)
    :gpsFilePath_(dataPath + "\\gps_file.txt"),
    gpsFile_(gpsFilePath_, std::ios::out)
{
    gpsFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    gpsFileLines_ = "";
    PrintGPSFileHeader();
}

GPSFile::~GPSFile()
{
    if (gpsFile_.is_open())
    {
        gpsFile_.close();
    }
    gpsFile_.clear();
}

void GPSFile::PrintGPSFileHeader()
{
    gpsFileLines_ += "file_name, logger_id, logging_session_number, configuration_type, longitude, latitude, start_time, end_time\n";
}

void GPSFile::PrintGPSFileLinesToFile()
{
    // print to file
    gpsFile_ << gpsFileLines_;
}
