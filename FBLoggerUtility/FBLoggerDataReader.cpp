#include "stdafx.h"
#include "FBLoggerDataReader.h"
#include "StringUtility.h"
#include "DirectoryReaderUtility.h"
#include "FileNameUtility.h"

#include "stdafx.h"
#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <math.h>
#include <sys/stat.h>
#include <errno.h>

FBLoggerDataReader::FBLoggerDataReader()
{
    numFilesProcessed_ = 0;
    numInvalidInputFiles_ = 0;
    numInvalidOutputFiles_ = 0;
    serialNumber_ = -1;
    pLogFile_ = NULL;
    pOutLoggerDataFile_ = NULL;
    pOutSensorStatsFile_ = NULL;
    pInFile_ = NULL;
    outputLine_ = "";
    dataPath_ = "";
    appPath_ = "";
    numErrors_ = 0;
    configFilePath_ = "";
    isConfigFileValid_ = false;
    outputsAreGood_ = false;
    lineStream_.str("");
    lineStream_.clear();

    currentFileIndex_ = 0;

    // Initialize F Config Sanity Checks
    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        sanityChecks_.FRawMin[i] = sanityChecks_.NAMin;
        sanityChecks_.FRawMax[i] = sanityChecks_.NAMax;
        sanityChecks_.FFinalMin[i] = sanityChecks_.NAMin;
        sanityChecks_.FFinalMax[i] = sanityChecks_.NAMax;
    }

    // Raw F Config Sanity Checks
    sanityChecks_.FRawMin[FIDPackageIndex::TEMPERATURE] = sanityChecks_.temperatureMin;
    sanityChecks_.FRawMin[FIDPackageIndex::FID_VOLTAGE] = sanityChecks_.FIDVoltageMin;
    sanityChecks_.FRawMin[FIDPackageIndex::PRESSURE_VOLTAGE] = sanityChecks_.pressureVoltageMin;
    sanityChecks_.FRawMin[FIDPackageIndex::PANEL_TEMP] = sanityChecks_.temperatureMin;

    sanityChecks_.FRawMax[FIDPackageIndex::TEMPERATURE] = sanityChecks_.temperatureMax;
    sanityChecks_.FRawMax[FIDPackageIndex::FID_VOLTAGE] = sanityChecks_.FIDVoltageMax;
    sanityChecks_.FRawMax[FIDPackageIndex::PRESSURE_VOLTAGE] = sanityChecks_.pressureVoltageMax;
    sanityChecks_.FRawMax[FIDPackageIndex::PANEL_TEMP] = sanityChecks_.temperatureMax;

    // Final F Config Sanity Checks
    sanityChecks_.FFinalMin[FIDPackageIndex::TEMPERATURE] = sanityChecks_.temperatureMin;
    sanityChecks_.FFinalMin[FIDPackageIndex::FID_VOLTAGE] = sanityChecks_.FIDVoltageMin;
    sanityChecks_.FFinalMin[FIDPackageIndex::PRESSURE] = sanityChecks_.pressureMin;
    sanityChecks_.FFinalMin[FIDPackageIndex::PANEL_TEMP] = sanityChecks_.temperatureMin;

    sanityChecks_.FFinalMax[FIDPackageIndex::TEMPERATURE] = sanityChecks_.temperatureMax;
    sanityChecks_.FFinalMax[FIDPackageIndex::FID_VOLTAGE] = sanityChecks_.FIDVoltageMax;
    sanityChecks_.FFinalMax[FIDPackageIndex::PRESSURE] = sanityChecks_.pressureMax;
    sanityChecks_.FFinalMax[FIDPackageIndex::PANEL_TEMP] = sanityChecks_.temperatureMax;

    // Initialize H Config Sanity Checks
    for (int i = 0; i < 9; i++)
    {
        sanityChecks_.HRawMin[i] = sanityChecks_.NAMin;
        sanityChecks_.HRawMax[i] = sanityChecks_.NAMax;
        sanityChecks_.HFinalMin[i] = sanityChecks_.NAMin;
        sanityChecks_.HFinalMax[i] = sanityChecks_.NAMax;
    }

    // H Config Raw Sanity Checks
    sanityChecks_.HRawMin[HeatFluxIndex::TEMPERATURE_SENSOR_ONE] = sanityChecks_.temperatureMin;
    sanityChecks_.HRawMin[HeatFluxIndex::TEMPERATURE_SENSOR_TWO] = sanityChecks_.temperatureMin;
    sanityChecks_.HRawMin[HeatFluxIndex::PRESSURE_SENSOR_U] = sanityChecks_.pressureVoltageMin;
    sanityChecks_.HRawMin[HeatFluxIndex::PRESSURE_SENSOR_V] = sanityChecks_.pressureVoltageMin;
    sanityChecks_.HRawMin[HeatFluxIndex::PRESSURE_SENSOR_W] = sanityChecks_.pressureVoltageMin;
    sanityChecks_.HRawMin[HeatFluxIndex::HEAT_FLUX_VOLTAGE] = sanityChecks_.heatFluxVoltageMin;
    sanityChecks_.HRawMin[HeatFluxIndex::HEAT_FLUX_TEMPERATURE_VOLTAGE] = sanityChecks_.heatFluxTemperatureVoltageMin;
    sanityChecks_.HRawMin[HeatFluxIndex::PANEL_TEMP] = sanityChecks_.temperatureMin;

    sanityChecks_.HRawMax[HeatFluxIndex::TEMPERATURE_SENSOR_ONE] = sanityChecks_.temperatureMax;
    sanityChecks_.HRawMax[HeatFluxIndex::TEMPERATURE_SENSOR_TWO] = sanityChecks_.temperatureMax;
    sanityChecks_.HRawMax[HeatFluxIndex::PRESSURE_SENSOR_U] = sanityChecks_.pressureVoltageMax;
    sanityChecks_.HRawMax[HeatFluxIndex::PRESSURE_SENSOR_V] = sanityChecks_.pressureVoltageMax;
    sanityChecks_.HRawMax[HeatFluxIndex::PRESSURE_SENSOR_W] = sanityChecks_.pressureVoltageMax;
    sanityChecks_.HRawMax[HeatFluxIndex::HEAT_FLUX_VOLTAGE] = sanityChecks_.heatFluxVoltageMax;
    sanityChecks_.HRawMax[HeatFluxIndex::HEAT_FLUX_TEMPERATURE_VOLTAGE] = sanityChecks_.heatFluxTemperatureVoltageMax;
    sanityChecks_.HRawMax[HeatFluxIndex::PANEL_TEMP] = sanityChecks_.temperatureMax;

    // Final H Config Sanity Checks
    sanityChecks_.HFinalMin[HeatFluxIndex::TEMPERATURE_SENSOR_ONE] = sanityChecks_.temperatureMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::TEMPERATURE_SENSOR_TWO] = sanityChecks_.temperatureMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::VELOCITY_U] = sanityChecks_.velocityMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::VELOCITY_V] = sanityChecks_.velocityMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::VELOCITY_W] = sanityChecks_.velocityMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::HEAT_FLUX] = sanityChecks_.heatFluxMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::HEAT_FLUX_TEMPERATURE] = sanityChecks_.heatFluxTemperatureMin;
    sanityChecks_.HFinalMin[HeatFluxIndex::PANEL_TEMP] = sanityChecks_.temperatureMin;

    sanityChecks_.HFinalMax[HeatFluxIndex::TEMPERATURE_SENSOR_ONE] = sanityChecks_.temperatureMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::TEMPERATURE_SENSOR_TWO] = sanityChecks_.temperatureMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::VELOCITY_U] = sanityChecks_.velocityMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::VELOCITY_V] = sanityChecks_.velocityMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::VELOCITY_W] = sanityChecks_.velocityMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::HEAT_FLUX] = sanityChecks_.heatFluxMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::HEAT_FLUX_TEMPERATURE] = sanityChecks_.heatFluxTemperatureMax;
    sanityChecks_.HFinalMax[HeatFluxIndex::PANEL_TEMP] = sanityChecks_.temperatureMax;

    // T Config Sanity Checks
    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        sanityChecks_.TMin[i] = sanityChecks_.temperatureMin;
        sanityChecks_.TMax[i] = sanityChecks_.temperatureMax;
    }

    ResetCurrentFileStats();
}

FBLoggerDataReader::~FBLoggerDataReader()
{

}

void FBLoggerDataReader::FillSensorValuesWithTestVoltages()
{
    if (configurationType_ == "F")
    {
        status_.sensorReadingValue[0] = 18.05; // This is temperature
        status_.sensorReadingValue[1] = 1.43; // FID(V)
        status_.sensorReadingValue[2] = 2.4; // This is the voltage used for pressure
        status_.sensorReadingValue[3] = 0.0; // NA
        status_.sensorReadingValue[4] = 0.0; // NA
        status_.sensorReadingValue[5] = 0.0; // NA
        status_.sensorReadingValue[6] = 0.0; // NA
        status_.sensorReadingValue[7] = 0.0; // NA
        status_.sensorReadingValue[8] = 17.1; // Panel Temp
    }
    else if (configurationType_ == "H")
    {
        status_.sensorReadingValue[0] = 20.05; // This is temperature
        status_.sensorReadingValue[1] = 19.43; // This is temperature
        status_.sensorReadingValue[2] = 1.21; // P(V)
        status_.sensorReadingValue[3] = 1.16; // P(V)
        status_.sensorReadingValue[4] = 1.01; // P(V)
        status_.sensorReadingValue[5] = 1.23; // HF(V)
        status_.sensorReadingValue[6] = -0.50; // HFT(V)
        status_.sensorReadingValue[7] = 0.00; // NA
        status_.sensorReadingValue[8] = 16.2; // Panel Temp
    }
}

void FBLoggerDataReader::PrepareToReadDataFiles()
{
    numFilesProcessed_ = 0;
    numInvalidInputFiles_ = 0;
    numInvalidOutputFiles_ = 0;
    numErrors_ = 0;
    currentFileIndex_ = 0;
    outputsAreGood_ = false;

    inputFilesNameList_.clear();
    invalidInputFileList_.clear();
    invalidInputFileErrorType_.clear();
    configMap_.clear();

    string logFileNoExtension = dataPath_ + "log_file";
    logFilePath_ = logFileNoExtension + ".txt";
    pLogFile_ = new ofstream(logFilePath_, std::ios::out);

    // Check for output file's existence
    if (!pLogFile_ || pLogFile_->fail())
    {
        pLogFile_->close();
        pLogFile_->clear();
        SYSTEMTIME systemTime;
        GetLocalTime(&systemTime);

        string dateTimeString = MakeStringWidthTwoFromInt(systemTime.wDay) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + std::to_string(systemTime.wYear) +
            "_" + MakeStringWidthTwoFromInt(systemTime.wHour) + "H" + MakeStringWidthTwoFromInt(systemTime.wMinute) + "M" +
            MakeStringWidthTwoFromInt(systemTime.wSecond) + "S";

        logFileLine_ = "ERROR: default log file at " + logFilePath_ + " already opened or otherwise unwritable\n";
        logFilePath_ = logFileNoExtension + "_" + dateTimeString + ".txt";

        ofstream logFile(logFilePath_, std::ios::out);
    }
    if (pLogFile_->good())
    {
        PrintLogFileLine();
        ReadConfig();

        if (isConfigFileValid_)
        {
            ReadDirectoryIntoStringVector(dataPath_, inputFilesNameList_);
            CheckConfigForAllFiles();
            PrintLogFileLine();
        }
    }
}

void FBLoggerDataReader::ProcessSingleDataFile()
{
    int codepage = CP_UTF8;
    //std::wstring extensionW = PathFindExtension(Get_utf16(inputFilesNameList_[i], codepage).c_str());
    string infileName = inputFilesNameList_[currentFileIndex_];
    std::wstring extensionW = PathFindExtension(Get_utf16(infileName, codepage).c_str());
    string extension = Utf8_encode(extensionW);

    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    if (extension == ".DAT")
    {
        inDataFilePath_ = dataPath_ + infileName;

        std::ifstream inFile(inDataFilePath_, std::ios::in | std::ios::binary);
        pInFile_ = &inFile;

        outputLine_ = "";
        configurationType_ = "";
        status_.isGoodOutput = true;

        ResetCurrentFileStats();

        // Check for input file's existence
        if (pInFile_ == NULL)
        {
            pInFile_->close();
            pInFile_ = NULL;
            pOutLoggerDataFile_ = NULL;
            // Report error
            logFileLine_ = "ERROR: Input file " + inDataFilePath_ + " does not exist\n";
            PrintLogFileLine();
        }
        else
        {
            // Get the size of the data file in bytes
            const char* cStringFilePath = inDataFilePath_.c_str();
            struct stat results;
            if (stat(cStringFilePath, &results) == 0)
            {
                fileSizeInBytes_ = results.st_size;

                bool headerFound = GetFirstHeader();

                if (configMap_.find(atoi(headerData_.serialNumberString.c_str())) != configMap_.end())
                {
                    configurationType_ = configMap_.find(atoi(headerData_.serialNumberString.c_str()))->second;
                }

                if ((headerFound) && (configurationType_ != ""))
                {
                    currentFileStats_.fileName = infileName;
                    SetLoggerDataOutFilePath(infileName);

                    ofstream outDataFile(outLoggerDataFilePath_, std::ios::out);
                    pOutLoggerDataFile_ = &outDataFile;

                    // Check for output file's existence and is not open by another process
                    if (pOutLoggerDataFile_ == NULL || outDataFile.fail())
                    {
                        pInFile_->close();
                        pOutLoggerDataFile_->close();
                        // Report error
                        logFileLine_ = "ERROR: Unable to open " + outLoggerDataFilePath_ + "\n";
                        PrintLogFileLine();
                        status_.isGoodOutput = false;
                    }
                    // We can read the input file
                    else
                    {
                        status_.isGoodOutput = true;
                        SetConfigDependentValues();
                        PrintHeader();
                        while (status_.pos < (fileSizeInBytes_ - BYTES_READ_PER_ITERATION))
                        {
                            ReadNextHeaderOrNumber();
                            UpdateSensorMaxAndMin();
                            if (status_.sensorReadingCounter == 9)
                            {
                                // An entire row of sensor data has been read in
                                PerformSanityChecksOnValues(SanityChecks::RAW);
                                PerformNeededDataConversions();
                                PerformSanityChecksOnValues(SanityChecks::FINAL);
                                UpdateStatsFileMap();
                                PrintSensorDataOutput();
                                // Reset sensor reading counter 
                                status_.sensorReadingCounter = 0;
                                // Update time for next set of sensor readings
                                UpdateTime();
                            }
                        }
                    }

                    // Close the file streams
                    pInFile_->close();
                    pInFile_ = NULL;
                    pOutLoggerDataFile_->close();
                    pOutLoggerDataFile_ = NULL;
                }
            }
            else
            {
                inFile.close();
                pInFile_ = NULL;
                // An error occurred
            }
        }

        if (!status_.isGoodOutput)
        {
            numInvalidOutputFiles_++;
            numErrors_++;
        }
    }
    else
    {
        numInvalidInputFiles_++;
        numErrors_++;
    }

    if(pLogFile_->good())
    {
        PrintLogFileLine();
    }

    numFilesProcessed_++;
    currentFileIndex_++;
}

void FBLoggerDataReader::EndReadingDataFiles()
{
    statsFilePath_ = dataPath_ + "sensor_stats.csv";
    ofstream outStatsFile(statsFilePath_, std::ios::out);

    // Check for sensor stats file's existence
    if (!outStatsFile || outStatsFile.fail())
    {
        outStatsFile.close();
        outStatsFile.clear();
        SYSTEMTIME systemTime;
        GetLocalTime(&systemTime);

        string dateTimeString = MakeStringWidthTwoFromInt(systemTime.wDay) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + std::to_string(systemTime.wYear) +
            "_" + MakeStringWidthTwoFromInt(systemTime.wHour) + "H" + MakeStringWidthTwoFromInt(systemTime.wMinute) + "M" +
            MakeStringWidthTwoFromInt(systemTime.wSecond) + "S";
        string statsFileNoExtension = dataPath_ + "sensor_stats";
        logFileLine_ = "ERROR: default log file at " + logFilePath_ + " already opened or otherwise unwritable\n";
        statsFilePath_ = statsFileNoExtension + "_" + dateTimeString + ".csv";

        ofstream outStatsFile(statsFilePath_, std::ios::out);
    }

    outputsAreGood_ = (numInvalidInputFiles_ == 0) && outStatsFile.good() && pLogFile_->good();

    if (outStatsFile.good())
    {
        pOutSensorStatsFile_ = &outStatsFile;
    }
 
    if (outStatsFile.good())
    {
        PrintStatsFile();

        pOutSensorStatsFile_->close();
        pOutSensorStatsFile_ = NULL;
    }

    if (numErrors_ == 0)
    {
        ReportSuccessToLog();
    }

    pLogFile_->close();
    // Deallocate dynamic memory
    delete pLogFile_;
    pLogFile_ = NULL;
}

double FBLoggerDataReader::CalculateHeatFlux(double rawVoltage)
{
    return 56.60 * pow(rawVoltage, 1.129);
}

double FBLoggerDataReader::CalculateHeatFluxTemperature(double rawVoltage)
{
	double thermistorResistance = 10000.0 / ((-1.25 / rawVoltage) - 1.0);
    double heatFluxTemperature = (1.0 / (0.003356 - (log(30000.0 / thermistorResistance) / 3962.0))) - 273.15;
    return heatFluxTemperature;
}

int FBLoggerDataReader::CalculateHeatFluxComponentVelocities(double temperatureOne, double temperatureTwo,
    double pressureVoltageU, double pressureVoltageV, double pressureVoltageW, double sensorBearing)
{
    bool tempOneGood = (temperatureOne <= sanityChecks_.temperatureMax) && (temperatureTwo >= sanityChecks_.temperatureMin);
    bool tempTwoGood = (temperatureTwo <= sanityChecks_.temperatureMax) && (temperatureTwo >= sanityChecks_.temperatureMin);
    double temperature = 0.0;

    // Calcuate average if both temps are good, or use the only good value
    if (tempOneGood && tempTwoGood)
    {
        temperature = (temperatureOne + temperatureTwo) / 2.0;
    }
    else if (tempOneGood)
    {
        temperature = temperatureOne;
    }
    else if (tempTwoGood)
    {
        temperature = temperatureOne;
    }
    else
    {
        // Error
        return -1;
    }

    if (convertVelocity_.convert(temperature, pressureVoltageU, pressureVoltageV, pressureVoltageW, sensorBearing) == true)
    {
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_U] = convertVelocity_.u;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_V] = convertVelocity_.v;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_W] = convertVelocity_.w;
    }
    else
    {
        return -1;
    }
    return 0; // success
}

double FBLoggerDataReader::CalculateFIDPackagePressure(double rawVoltage)
{
    return 249.09 * (rawVoltage - 1.550) * 0.8065;
}

void FBLoggerDataReader::PerformNeededDataConversions()
{
    if (configurationType_ == "F")
    {
        double FIDPackagePressure = CalculateFIDPackagePressure(status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE]);
        bool FIDPackagePressureGood = (FIDPackagePressure <= sanityChecks_.pressureMax) && (FIDPackagePressure >= sanityChecks_.pressureMin);
        if (FIDPackagePressureGood)
        {
            status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE] = FIDPackagePressure;
        }
        else
        {
            status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE] = -99999999999.0;
        }
    }
    else if (configurationType_ == "H")
    {
        double heatFlux = CalculateHeatFlux(status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_VOLTAGE]);
        double heatFluxTemperature = CalculateHeatFluxTemperature(status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_TEMPERATURE_VOLTAGE]);

        double bearing = sensorBearingMap_.find(serialNumber_)->second;
        status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_VOLTAGE] = heatFlux;
        status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_TEMPERATURE_VOLTAGE] = heatFluxTemperature;
        CalculateHeatFluxComponentVelocities(status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_ONE],
            status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_TWO],
            status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_U],
            status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_V],
            status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_W],
            bearing);
    }
}

void FBLoggerDataReader::PerformSanityChecksOnValues(SanityChecks::SanityCheckTypeEnum sanityCheckType)
{
    double minForSanityCheck = 0.0;
    double maxForSanityCheck = 0.0;
    //unsigned int currentIndex = status_.sensorReadingCounter;

    for (unsigned int currentIndex = 0; currentIndex < NUM_SENSOR_READINGS; currentIndex++)
    {
        double currentValue = status_.sensorReadingValue[currentIndex];
        if (configurationType_ == "F")
        {
            if (sanityCheckType == SanityChecks::RAW)
            {
                minForSanityCheck = sanityChecks_.FRawMin[currentIndex];
                maxForSanityCheck = sanityChecks_.FRawMax[currentIndex];
            }
            else if (sanityCheckType == SanityChecks::FINAL)
            {
                minForSanityCheck = sanityChecks_.FFinalMin[currentIndex];
                maxForSanityCheck = sanityChecks_.FFinalMax[currentIndex];
            }
        }
        else if (configurationType_ == "H")
        {
            if (sanityCheckType == SanityChecks::RAW)
            {
                minForSanityCheck = sanityChecks_.HRawMin[currentIndex];
                maxForSanityCheck = sanityChecks_.HRawMax[currentIndex];
            }
            else if (sanityCheckType == SanityChecks::FINAL)
            {
                minForSanityCheck = sanityChecks_.HFinalMin[currentIndex];
                maxForSanityCheck = sanityChecks_.HFinalMax[currentIndex];
            }
        }
        else if (configurationType_ == "T")
        {
            if (sanityCheckType == SanityChecks::RAW)
            {
                minForSanityCheck = sanityChecks_.TMin[currentIndex];
                maxForSanityCheck = sanityChecks_.TMax[currentIndex];
            }
            else if (sanityCheckType == SanityChecks::FINAL)
            {
                minForSanityCheck = sanityChecks_.TMin[currentIndex];
                maxForSanityCheck = sanityChecks_.TMax[currentIndex];
            }
        }

        if (sanityCheckType == SanityChecks::RAW)
        {
            if (isnan(currentValue))
            {
                currentFileStats_.columnFailedSanityCheckRaw[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckRaw = true;
            }
            else if (!(double_equals(minForSanityCheck, sanityChecks_.IGNORE_MIN)) && (currentValue < minForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckRaw[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckRaw = true;
            }
            else if (!(double_equals(maxForSanityCheck, sanityChecks_.IGNORE_MAX)) && (currentValue > maxForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckRaw[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckRaw = true;
            }
        }
        else if (sanityCheckType == SanityChecks::FINAL)
        {
            if (isnan(currentValue))
            {
                currentFileStats_.columnFailedSanityCheckFinal[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckFinal = true;
            }
            else if (!(double_equals(minForSanityCheck, sanityChecks_.IGNORE_MIN)) && (currentValue < minForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckFinal[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckFinal = true;
            }
            else if (!(double_equals(maxForSanityCheck, sanityChecks_.IGNORE_MAX)) && (currentValue > maxForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckFinal[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckFinal = true;
            }
        }
    }
}

void FBLoggerDataReader::ReadConfig()
{
    // Check for config file's existence
    std::ifstream configFile(configFilePath_, std::ios::in | std::ios::binary);

    string line = "";
    numErrors_ = 0;

    status_.configFileLineNumber = 0;

    if (!configFile)
    {
        configFile.close();
        logFileLine_ = "Error: config file not found or cannot be opened at path " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
    }
    else
    {
        if (CheckConfigFileFormatIsValid(configFile))
        {
            while (getline(configFile, line))
            {
                ParseTokensFromLineOfConfigFile(line);
                ProcessErrorsInLineOfConfigFile();
            }
        }
    }

    PrintLogFileLine();

    if (numErrors_ == 0)
    {
        isConfigFileValid_ = true;
    }
}

void FBLoggerDataReader::ParseTokensFromLineOfConfigFile(string& line)
{
    enum
    {
        SERIAL_NUMBER = 0,
        CONFIGURATION = 1,
        SENSOR_NUMBER = 2,
        SENSOR_BEARING = 3
    };

    int tokenCounter = 0;

    string token;

    status_.configFileLineNumber++;

    lineStream_.str("");
    lineStream_.clear();

    token = "";
    configFileLine_.serialNumberString = "";
    configFileLine_.conifgurationString = "";
    configFileLine_.sensorNumberString = "";
    configFileLine_.sensorBearingString = "";

    status_.isSerialNumberValid = false;
    status_.isConfigurationTypeValid = false;
    status_.isSensorNumberValid = false;
    status_.isSensorBearingValid = false;

    lineStream_ << line;

    while (getline(lineStream_, token, ','))
    {
        // remove any whitespace or control chars from token
        token.erase(std::remove(token.begin(), token.end(), ' '), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\r'), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\r\n'), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\t'), token.end());

        int testSerial = atoi(token.c_str());

        switch (tokenCounter)
        {
            case SERIAL_NUMBER:
            {
                configFileLine_.serialNumberString = token;
                if (IsOnlyDigits(configFileLine_.serialNumberString))
                {
                    status_.isSerialNumberValid = true;
                }
                else
                {
                    status_.isSerialNumberValid = false;
                    isConfigFileValid_ = false;
                }
                break;
            }
            case CONFIGURATION:
            {
                configFileLine_.conifgurationString = token;
                if (configFileLine_.conifgurationString == "F" ||
                    configFileLine_.conifgurationString == "H" ||
                    configFileLine_.conifgurationString == "T" ||
                    configFileLine_.conifgurationString == "")
                {
                    status_.isConfigurationTypeValid = true;
                }
                else
                {
                    status_.isConfigurationTypeValid = false;
                    isConfigFileValid_ = false;
                }
                break;
            }
            case SENSOR_NUMBER:
            {
                configFileLine_.sensorNumberString = token;
                if (IsOnlyDigits(configFileLine_.sensorNumberString))
                {
                    status_.isSensorNumberValid = true;
                }
                else if (configFileLine_.conifgurationString == "T" &&
                    configFileLine_.sensorNumberString == "")
                {
                    status_.isSensorNumberValid = true;
                }
                else if (configFileLine_.conifgurationString == "" &&
                    configFileLine_.sensorNumberString == "")
                {
                    status_.isSensorNumberValid = true;
                }
                else
                {
                    status_.isSensorNumberValid = false;
                    isConfigFileValid_ = false;
                }
                break;
            }
            case SENSOR_BEARING:
            {
                configFileLine_.sensorBearingString = token;

                char* end;
                errno = 0;
                configFileLine_.sensorBearingValue = std::strtod(token.c_str(), &end);
                if (configFileLine_.conifgurationString == "H")
                {
                    if (*end != '\0' ||  // error, we didn't consume the entire string
                        errno != 0)   // error, overflow or underflow
                    {
                        status_.isSensorBearingValid = false;
                    }
                    else if (configFileLine_.sensorBearingValue < 0 || configFileLine_.sensorBearingValue > 360)
                    {
                        status_.isSensorBearingValid = false;
                    }
                    else
                    {
                        status_.isSensorBearingValid = true;
                    }
                }
                else
                {
                    status_.isSensorBearingValid = true;
                }
                break;
            }
        }
        tokenCounter++;
    }

    if (tokenCounter < 4)
    {
        logFileLine_ = "Error: The entry in config file at " + configFilePath_ + " has less than the required 4 columns with first invalid entry at line " + std::to_string(status_.configFileLineNumber) + " " + GetMyLocalDateTimeString() + "\n";
        numErrors_++;
        isConfigFileValid_ = false;
    }
    if (tokenCounter > 4)
    {
        logFileLine_ = "Error: The entry in in config file at " + configFilePath_ + " has more than the required 4 columns with first invalid entry at line " + std::to_string(status_.configFileLineNumber) + " " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
}

bool FBLoggerDataReader::CheckConfigFileFormatIsValid(ifstream& configFile)
{
    // Check if file is valid
    string token = "";
    string line;
    getline(configFile, line);
    lineStream_.str("");
    lineStream_.clear();
    lineStream_ << line;
    getline(lineStream_, token, ' ');
    status_.configFileLineNumber++;
    if (token != "First")
    {
        logFileLine_ = "Error: Config file at " + configFilePath_ + " is improperly formatted " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
    else
    {
        isConfigFileValid_ = true;
    }

    return isConfigFileValid_;
}

void FBLoggerDataReader::ProcessErrorsInLineOfConfigFile()
{
    if (status_.isSerialNumberValid && status_.isConfigurationTypeValid)
    {
        int serialNumber = atoi(configFileLine_.serialNumberString.c_str());
        int testCount = (int)configMap_.count(serialNumber);
        if (configMap_.count(serialNumber) > 0)
        {
            logFileLine_ += "Error: More than one entry exists for logger id " + configFileLine_.serialNumberString +
                " with first repeated entry in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
            isConfigFileValid_ = false;
            numErrors_++;
        }
        else
        {
            if (configFileLine_.conifgurationString == "")
            {
                if (configFileLine_.sensorNumberString != "")
                {
                    if (status_.isSerialNumberValid)
                    {
                        logFileLine_ += "Error: " + configFileLine_.conifgurationString + " is an invalid configuration for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
                            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                    else
                    {
                        logFileLine_ += "Error: Invalid configuration (second column) in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                }
            }
            if (status_.isSerialNumberValid && status_.isConfigurationTypeValid && status_.isSensorNumberValid && status_.isSensorBearingValid)
            {
                configMap_.insert(pair<int, string>(serialNumber, configFileLine_.conifgurationString));
                if (configFileLine_.conifgurationString == "H")
                {
                    sensorBearingMap_.insert(pair<int, double>(serialNumber, configFileLine_.sensorBearingValue));
                }
            }
            else if (!status_.isSensorNumberValid)
            {
                logFileLine_ += "Error: Entry " + configFileLine_.sensorNumberString + " is invalid for sensor number (third column) for logger id " + configFileLine_.serialNumberString +
                    " in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                isConfigFileValid_ = false;
                numErrors_++;
            }
            else if (configFileLine_.conifgurationString == "H" && !status_.isSensorBearingValid)
            {
                logFileLine_ += "Error: " + configFileLine_.sensorBearingString + " is an invalid entry for bearing (fourth column) for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
                    " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                isConfigFileValid_ = false;
                numErrors_++;
            }
            else
            {
                logFileLine_ += "Error: Entry for logger id " + configFileLine_.serialNumberString +
                    " is missing sensor number data (third column) " + " in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                isConfigFileValid_ = false;
                numErrors_++;
            }
        }
    }
    else if (!status_.isSerialNumberValid)
    {
        if (configFileLine_.serialNumberString != "")
        {
            logFileLine_ += "Error: " + configFileLine_.serialNumberString + " is an invalid serial number (first column) entry in line " + std::to_string(status_.configFileLineNumber) + " in config file at " +
                configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        }
        else
        {
            logFileLine_ += "Error: No entry for serial number (first column) in line number " + std::to_string(status_.configFileLineNumber) + " in config file at " +
                configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        }
        isConfigFileValid_ = false;
        numErrors_++;
    }
    else if (!status_.isConfigurationTypeValid)
    {
        logFileLine_ += "Error: " + configFileLine_.conifgurationString + " is an invalid configuration(second column) for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
    else if (configFileLine_.conifgurationString == "H" && !status_.isSensorBearingValid)
    {
        logFileLine_ += "Error: " + configFileLine_.sensorBearingString + " is an invalid entry for bearing (fourth column) for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }

    int a = 1;
}

void FBLoggerDataReader::CheckConfigForAllFiles()
{
    int codepage = CP_UTF8;
    struct stat results;
    std::wstring extensionW;
    string extension;

    numInvalidInputFiles_ = 0;

    for (unsigned int i = 0; i < inputFilesNameList_.size(); i++)
    {
        inDataFilePath_ = dataPath_ + inputFilesNameList_[i];
        extensionW = PathFindExtension(Get_utf16(inputFilesNameList_[i], codepage).c_str());
        extension = Utf8_encode(extensionW);
        if (extension == ".DAT" || extension == ".dat")
        {
            std::ifstream inFile(inDataFilePath_, std::ios::in | std::ios::binary);
            pInFile_ = &inFile;

            // Check for input file's existence
            if (pInFile_ == NULL)
            {
                // Cannot open file
                numInvalidInputFiles_++;
                invalidInputFileList_.push_back(inDataFilePath_);
                invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::CANNOT_OPEN_FILE);
                pInFile_->close();
                pInFile_ = NULL;
            }
            else
            {
                // Get the size of the data file in bytes             
                const char* cStringFileName = inDataFilePath_.c_str();
                if (stat(cStringFileName, &results) == 0)
                {
                    // The size of the file in bytes
                    fileSizeInBytes_ = results.st_size;

                    bool headerFound = GetFirstHeader();
                    int serialNumber = atoi(headerData_.serialNumberString.c_str());
                    if ((headerFound) && (configMap_.find(serialNumber) == configMap_.end()) || configMap_.find(serialNumber)->second == "")
                    {
                        // Config missing for file
                        numInvalidInputFiles_++;
                        invalidInputFileList_.push_back(inDataFilePath_);
                        invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::MISSING_CONFIG);
                        pInFile_->close();
                        pInFile_ = NULL;
                    }
                    else if (!headerFound)
                    {
                        // No header found in file
                        numInvalidInputFiles_++;
                        invalidInputFileList_.push_back(inDataFilePath_);
                        invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::MISSING_HEADER);
                        pInFile_->close();
                        pInFile_ = NULL;
                    }
                }
            }
        }
    }

    if (numInvalidInputFiles_)
    {
        PrintConfigErrorsToLog();
    }
}

void FBLoggerDataReader::ResetHeaderData()
{
    headerData_.rawHeader = "0000";
    headerData_.serialNumberString = "00000";
    headerData_.latitudeDegreesString = "00";
    headerData_.latitudeDecimalMinutesString = "00000000";
    headerData_.longitudeDegreesString = "000";
    headerData_.longitudeDecimalMinutesString = "00000000";
    headerData_.dayString = "00";
    headerData_.monthString = "00";
    headerData_.yearString = "2000";
    headerData_.hourString = "00";
    headerData_.minuteString = "00";
    headerData_.secondString = "00";
    headerData_.millisecondString = "000";
    headerData_.sampleIntervalString = "0000";
    headerData_.year = 0;
    headerData_.month = 0;
    headerData_.day = 0;
    headerData_.hours = 0;
    headerData_.minutes = 0;
    headerData_.seconds = 0;
    headerData_.milliseconds = 0;
    headerData_.sampleInterval = 0;
}

void FBLoggerDataReader::ResetInFileReadingStatus()
{
    status_.sensorReadingCounter = 0;
    status_.pos = 0;
    status_.recordNumber = 0;
    status_.configColumnTextLine;

    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        status_.columnRawType[i] = "";
        status_.sensorReadingValue[i] = 0.0;
    }
    status_.headerFound = false;
}

void FBLoggerDataReader::ResetCurrentFileStats()
{
    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        currentFileStats_.columnFailedSanityCheckRaw[i] = false;
        currentFileStats_.columnFailedSanityCheckFinal[i] = false;
        currentFileStats_.columnMin[i] = 99999.0;
        currentFileStats_.columnMax[i] = -99999.0;
    }
    currentFileStats_.isFailedSanityCheckFinal = false;
    currentFileStats_.isFailedSanityCheckRaw = false;
}

void FBLoggerDataReader::SetConfigDependentValues()
{
    if (configurationType_ == "F")
    {
        //status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"FID(V)\",\"P(V)\",\"NA\",\"NA\",\"NA\",\"NA\",\"NA\",\"Panel Temp (°C)\"\n";
        status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"FID(V)\",\"P(Pa)\",\"NA\",\"NA\",\"NA\",\"NA\",\"NA\",\"Panel Temp (°C)\"\n";
        status_.columnRawType[0] = "°C";
        status_.columnRawType[1] = "FID(V)";
        status_.columnRawType[2] = "P(V)";
        for (int i = 3; i < 8; i++)
        {
            status_.columnRawType[i] = "NA";
        }
    }
    else if (configurationType_ == "H")
    {
        //status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"°C\",\"P(V)\",\"P(V)\",\"P(V)\",\"HF(V)\",\"HFT(V)\",\"NA\",\"Panel Temp (°C)\"\n";
        status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"°C\",\"u(m/s)\",\"v(m/s)\",\"w(m/s)\",\"HF(kW/m^2)\",\"HFT(C)\",\"NA\",\"Panel Temp (°C)\"\n";
        for (int i = 0; i < 2; i++)
        {
            status_.columnRawType[i] = "°C";
        }
        for (int i = 2; i < 5; i++)
        {
            status_.columnRawType[i] = "P(V)";
        }
        status_.columnRawType[5] = "HF(V)";
        status_.columnRawType[6] = "HFT(V)";
        status_.columnRawType[7] = "NA";
    }
    else if (configurationType_ == "T")
    {
        status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"°C\",\"°C\",\"°C\",\"°C\",\"°C\",\"°C\",\"°C\",\"Panel Temp (°C)\"\n";
        for (int i = 0; i < 8; i++)
        {
            status_.columnRawType[i] = "°C";
        }
    }
    // Common among all configurations
    status_.columnRawType[8] = "Panel Temp (°C)";
}

void FBLoggerDataReader::PrintStatsFileHeader()
{
    string outputLine = "";
    outputLine += "Full Sensor Min/Max Stats For All Files:\n";
    outputLine += "File Name,Col 1 Min,Col 1 Max,Col 1 Type,";
    outputLine += "Col 2 Min,Col 2 Max,Col 2 Type,";
    outputLine += "Col 3 Min,Col 3 Max,Col 3 Type,";
    outputLine += "Col 3 Min,Col 4 Max,Col 4 Type,";
    outputLine += "Col 3 Min,Col 5 Max,Col 5 Type,";
    outputLine += "Col 6 Min,Col 6 Max,Col 6 Type,";
    outputLine += "Col 7 Min,Col 7 Max,Col 7 Type,";
    outputLine += "Col 8 Min,Col 8 Max,Col 8 Type,";
    outputLine += "Panel Temp Min,Panel Temp Max,Panel Temp Type\n";

    *pOutSensorStatsFile_ << outputLine;
}

void FBLoggerDataReader::PrintStatsFile()
{
    string outputLine = "";
    string fileName = "";
    for (auto it = statsFileMap_.begin(); it != statsFileMap_.end(); it++)
    {
        if (it->second.isFailedSanityCheckRaw)
        {
            int numFailures = 0;
            int lastFailure = 0;
            fileName = it->second.fileName;
            outputLine += fileName + ": Raw data has failed sanity check in column(s) ";
            for (int i = 0; i < NUM_SENSOR_READINGS; i++)
            {
                if (it->second.columnFailedSanityCheckRaw[i])
                {
                    numFailures++;
                    lastFailure = i;
                }
            }
            for (int i = 0; i < NUM_SENSOR_READINGS; i++)
            {
                if (it->second.columnFailedSanityCheckRaw[i])
                {
                    if ((numFailures > 1) && (i < lastFailure))
                    {
                        outputLine += to_string(i + 1) + " ";
                    }
                    else if ((numFailures > 1) && (i == lastFailure))
                    {
                        outputLine += "and " + to_string(i + 1) + "\n";
                    }
                    else
                    {
                        outputLine += to_string(i + 1) + "\n";
                    }
                }
            }
        }
        if (it->second.isFailedSanityCheckFinal)
        {
            int numFailures = 0;
            int lastFailure = 0;
            outputLine += "Converted data in file " + it->second.fileName + " has failed sanity check in column(s) ";
            for (int i = 0; i < NUM_SENSOR_READINGS; i++)
            {
                if (it->second.columnFailedSanityCheckFinal[i])
                {
                    numFailures++;
                    lastFailure = i;
                }
            }
            for (int i = 0; i < NUM_SENSOR_READINGS; i++)
            {
                if (it->second.columnFailedSanityCheckFinal[i])
                {
                    if ((numFailures > 1) && (i < lastFailure))
                    {
                        outputLine += to_string(i + 1) + " ";
                    }
                    else if ((numFailures > 1) && (i == lastFailure))
                    {
                        outputLine += "and " + to_string(i + 1) + "\n";
                    }
                    else
                    {
                        outputLine += to_string(i + 1) + "\n";
                    }
                }
            }
        }
    }

    if (outputLine != "")
    {
        outputLine += "\n";
        *pOutSensorStatsFile_ << outputLine;
    }

    PrintStatsFileHeader();

    outputLine = "";
    for (auto it = statsFileMap_.begin(); it != statsFileMap_.end(); it++)
    {
        outputLine += it->second.fileName + ",";

        for (int i = 0; i < NUM_SENSOR_READINGS; i++)
        {
            outputLine += std::to_string(it->second.columnMin[i]) + "," + std::to_string(it->second.columnMax[i]) + "," + status_.columnRawType[i];
            if (i < 8)
            {
                outputLine += ",";
            }
            else
            {
                outputLine += "\n";
            }
        }

        *pOutSensorStatsFile_ << outputLine;
    }
}

void FBLoggerDataReader::ReportSuccessToLog()
{
    string dataPathOutput = dataPath_.substr(0, dataPath_.size() - 1);
    if (pLogFile_->good() && logFileLine_ == "" && invalidInputFileList_.empty() && numErrors_ == 0)
    {
        if (numFilesProcessed_)
        {
            logFileLine_ = "Successfully processed " + std::to_string(numFilesProcessed_) + " DAT files in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        else
        {
            logFileLine_ = "No DAT files found in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        *pLogFile_ << logFileLine_;
    }
}

void FBLoggerDataReader::UpdateStatsFileMap()
{
    statsFileMap_.insert(pair<int, StatsFileData>(serialNumber_, currentFileStats_));
}

void FBLoggerDataReader::SetDataPath(string dataPath)
{
    if (dataPath.length() > 0)
    {
        char lastChar = dataPath[dataPath.length() - 1];
        char testChar('\\');

        if (lastChar != testChar)
        {
            dataPath += '\\';
        }

        dataPath_ = dataPath;
    }
    else
    {
        dataPath_ = "";
    }
}

void FBLoggerDataReader::SetAppPath(string appPath)
{
    appPath_ = appPath;
}

void FBLoggerDataReader::SetConfigFile(string configFileFullPath)
{
    configFilePath_ = configFileFullPath;
}

string FBLoggerDataReader::GetDataPath()
{
    return dataPath_;
}

string FBLoggerDataReader::GetLogFilePath()
{
    return logFilePath_;
}

string FBLoggerDataReader::GetStatsFilePath()
{
    return statsFilePath_;
}

unsigned int FBLoggerDataReader::GetNumFilesProcessed()
{
    return numFilesProcessed_;
}

unsigned int FBLoggerDataReader::GetNumInvalidFiles()
{
    return numInvalidInputFiles_ + numInvalidOutputFiles_;
}

bool FBLoggerDataReader::IsConfigFileValid()
{
    return isConfigFileValid_;
}

bool FBLoggerDataReader::IsLogFileGood()
{
    bool isGood = false;
    if (!pLogFile_ == NULL)
    {
        isGood = pLogFile_->good();
    }
    return isGood;
}

bool FBLoggerDataReader::IsDoneReadingDataFiles()
{
    return(currentFileIndex_ >= inputFilesNameList_.size());
}

bool FBLoggerDataReader::GetFirstHeader()
{
    ResetInFileReadingStatus();
    ResetHeaderData();

    status_.pos = (int)pInFile_->tellg();
    uint32_t filePositionLimit = fileSizeInBytes_ - BYTES_READ_PER_ITERATION;

    // Keep grabbing bytes until header is found or end of file is reached
    while (!status_.headerFound && (status_.pos < filePositionLimit))
    {
        GetRawNumber();

        // Update file reading position
        status_.pos = (int)pInFile_->tellg();
    }
    if (status_.headerFound)
    {
        // Reset record and sensor reading count
        status_.recordNumber = 0;
        status_.sensorReadingCounter = 0;
        // Get header data and print it to file
        GetHeader();
        ParseHeader();
        // Update file reading position
        status_.pos = (int)pInFile_->tellg();
    }

    return status_.headerFound;
}

void FBLoggerDataReader::ReadNextHeaderOrNumber()
{
    status_.headerFound = false;
    status_.pos = (int)pInFile_->tellg();

    GetRawNumber();
    CheckForHeader();
    if (!status_.headerFound)
    {
        if (parsedNumericData_.channelNumber != 0) // Prevents array out of bounds while reading a header 
        {
            parsedNumericData_.intFromBytes = GetIntFromByteArray(parsedNumericData_.rawHexNumber);
            parsedNumericData_.parsedIEEENumber = UnsignedIntToIEEEFloat(parsedNumericData_.intFromBytes);
            status_.sensorReadingValue[parsedNumericData_.channelNumber - 1] = parsedNumericData_.parsedIEEENumber;
            // increment sensor reading for next iteration
            status_.sensorReadingCounter++;
        }
    }
    else if (status_.headerFound)
    {
        // Reset record and sensor reading count
        status_.recordNumber = 0;
        status_.sensorReadingCounter = 0;
        // Get header data and print it to file
        GetHeader();
        ParseHeader();
        PrintHeader();
    }
}

uint32_t FBLoggerDataReader::GetIntFromByteArray(uint8_t byteArray[4])
{
    uint32_t integerValue = int(
        byteArray[3] << 24 | // Set high byte (bits 24 to 31)
        byteArray[2] << 16 | // Set bits 16 to 23
        byteArray[1] << 8 | // Set bits 8 to 15
        byteArray[0]); // Set low byte (bits 0 to 7)

    return integerValue;
}

void FBLoggerDataReader::GetRawNumber()
{
    uint8_t byte;
    char rawByte[2];
    rawByte[1] = '\0';
    uint8_t *pRawByte;
    int index = 0;
    status_.headerFound = false;

    // Always read in 4 bytes, then check if those 4 bytes contain the header signature
    for (int i = 0; i < 4; i++)
    {
        pInFile_->read(rawByte, 1);
        pRawByte = (uint8_t *)(&rawByte);
        byte = *pRawByte;
        parsedNumericData_.rawHexNumber[i] = byte;
    }
    CheckForHeader();
    // If it is not a header, read in 1 more byte for channel number
    if (!status_.headerFound)
    {
        pInFile_->read(rawByte, 1);
        pRawByte = (uint8_t *)(&rawByte);
        parsedNumericData_.channelNumber = *pRawByte;
        if (parsedNumericData_.channelNumber == 0) 
        {
            // Make panel temp's channel number 9, as it is printed last
            parsedNumericData_.channelNumber = 9;
        }
    }
}

void FBLoggerDataReader::CheckForHeader()
{
    // Check for header signature "SNXX" where X is 0-9
    if ((parsedNumericData_.rawHexNumber[0] == 'S') && (parsedNumericData_.rawHexNumber[1] == 'N') && (parsedNumericData_.rawHexNumber[2] == '0')
        && (parsedNumericData_.rawHexNumber[3] == '0' || parsedNumericData_.rawHexNumber[3] == '1' || parsedNumericData_.rawHexNumber[3] == '2' ||
            parsedNumericData_.rawHexNumber[3] == '3' || parsedNumericData_.rawHexNumber[3] == '4' || parsedNumericData_.rawHexNumber[3] == '5' ||
            parsedNumericData_.rawHexNumber[3] == '6' || parsedNumericData_.rawHexNumber[3] == '7' || parsedNumericData_.rawHexNumber[3] == '8' ||
            parsedNumericData_.rawHexNumber[3] == '9'))
    {
        status_.headerFound = true;
    }
}

void FBLoggerDataReader::UpdateTime()
{
    // Zero is in first entry so index is calendar year month
    static const unsigned int monthLength[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    headerData_.milliseconds += headerData_.sampleInterval;
    if (headerData_.milliseconds >= 1000)
    {
        headerData_.milliseconds -= 1000;
        headerData_.seconds++;
        if (headerData_.seconds >= 60)
        {
            headerData_.seconds -= 60;
            headerData_.minutes++;
            if (headerData_.minutes >= 60)
            {
                headerData_.minutes -= 60;
                headerData_.hours++;
                if (headerData_.hours >= 24)
                {
                    headerData_.hours -= 24;
                    headerData_.day++;
                    // Handling leap years
                    bool leap = (headerData_.year % 4 == 0) && (headerData_.year % 100 != 0 || headerData_.year % 400 == 0);
                    if (headerData_.day > monthLength[headerData_.month])
                    {
                        if (!(leap && (headerData_.month == 2)))
                        {
                            // If it's not both february and a leap year
                            // Then move on to the next month
                            headerData_.day = 1;
                            headerData_.month++;
                        }
                        else if (leap && (headerData_.month == 2) && (headerData_.day > 29))
                        {
                            // Else if it's a leap year and yesterday was February 29
                            // Then move on to the next month
                            headerData_.day = 1;
                            headerData_.month++;
                        }
                    }
                    if (headerData_.month > 12)
                    {
                        headerData_.year++;
                        headerData_.month = 1;
                    }
                }
            }
        }
    }
}

string FBLoggerDataReader::GetMyLocalDateTimeString()
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);

    return MakeStringWidthTwoFromInt(systemTime.wDay) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + std::to_string(systemTime.wYear) +
        " " + MakeStringWidthTwoFromInt(systemTime.wHour) + ":" + MakeStringWidthTwoFromInt(systemTime.wMinute) + ":" +
        MakeStringWidthTwoFromInt(systemTime.wSecond) + ":" + MakeStringWidthThreeFromInt(systemTime.wMilliseconds);
}

void FBLoggerDataReader::GetHeader()
{
    uint8_t byte = 0;
    ResetHeaderData();

    for (int i = 0; i < 4; i++)
    {
        headerData_.rawHeader[i] = parsedNumericData_.rawHexNumber[i];
    }
    for (unsigned int i = 4; i < 48; i++)
    {
        if (pInFile_)
        {
            *pInFile_ >> byte;
            headerData_.rawHeader += byte;
        }
    }
}

void FBLoggerDataReader::ParseHeader()
{
    string temp;
    // Get the serial number
    for (int i = 2; i < 7; i++)
    {
        headerData_.serialNumberString[i - 2] = headerData_.rawHeader[i];
    }
    // Get the latitude
    for (int i = 7; i < 9; i++)
    {
        headerData_.latitudeDegreesString[i - 7] = headerData_.rawHeader[i];
    }
    headerData_.latitudeDegreesString += "°";
    for (int i = 9; i < 17; i++)
    {
        headerData_.latitudeDecimalMinutesString[i - 9] = headerData_.rawHeader[i];
    }
    temp = headerData_.latitudeDecimalMinutesString[7];
    headerData_.latitudeDecimalMinutesString[7] = '\'';
    headerData_.latitudeDecimalMinutesString += " " + temp;
    //Get the longitude
    for (int i = 17; i < 20; i++)
    {
        headerData_.longitudeDegreesString[i - 17] = headerData_.rawHeader[i];
    }
    headerData_.longitudeDegreesString += "°";
    for (int i = 20; i < 28; i++)
    {
        headerData_.longitudeDecimalMinutesString[i - 20] = headerData_.rawHeader[i];
    }
    temp = headerData_.longitudeDecimalMinutesString[7];
    headerData_.longitudeDecimalMinutesString[7] = '\'';
    headerData_.longitudeDecimalMinutesString += " " + temp;
    for (int i = 28; i < 30; i++)
    {
        headerData_.yearString[i - 26] = headerData_.rawHeader[i];
    }
    headerData_.year = std::stoi(headerData_.yearString);
    for (int i = 30; i < 32; i++)
    {
        headerData_.monthString[i - 30] = headerData_.rawHeader[i];
    }
    headerData_.month = std::stoi(headerData_.monthString);
    for (int i = 32; i < 34; i++)
    {
        headerData_.dayString[i - 32] = headerData_.rawHeader[i];
    }
    headerData_.day = std::stoi(headerData_.dayString);
    for (int i = 34; i < 36; i++)
    {
        headerData_.hourString[i - 34] = headerData_.rawHeader[i];
    }
    headerData_.hours = std::stoi(headerData_.hourString);
    for (int i = 36; i < 38; i++)
    {
        headerData_.minuteString[i - 36] = headerData_.rawHeader[i];
    }
    headerData_.minutes = std::stoi(headerData_.minuteString);
    for (int i = 38; i < 40; i++)
    {
        headerData_.secondString[i - 38] = headerData_.rawHeader[i];
    }
    headerData_.seconds = std::stoi(headerData_.secondString);
    for (int i = 41; i < 44; i++)
    {
        headerData_.millisecondString[i - 41] = headerData_.rawHeader[i];
    }
    headerData_.milliseconds = std::stoi(headerData_.millisecondString);
    for (int i = 44; i < 48; i++)
    {
        headerData_.sampleIntervalString[i - 44] = headerData_.rawHeader[i];
    }
    headerData_.sampleInterval = std::stoi(headerData_.sampleIntervalString);
}

void FBLoggerDataReader::SetLoggerDataOutFilePath(string infileName)
{
    string firstDateString;
    string firstTimeString;

    firstDateString = headerData_.dayString + "-" + headerData_.monthString + "-" + headerData_.yearString.at(2) + headerData_.yearString.at(3);
    firstTimeString = headerData_.hourString + headerData_.minuteString;

    infileName = RemoveFileExtension(infileName);

    int infileIntValue = atoi(infileName.c_str());
    serialNumber_ = atoi(headerData_.serialNumberString.c_str());

    if (infileIntValue == serialNumber_ && IsOnlyDigits(infileName))
    {
        outLoggerDataFilePath_ = dataPath_ + +"SN" + headerData_.serialNumberString.c_str();
    }
    else
    {
        outLoggerDataFilePath_ = dataPath_ + infileName + "_" + +"SN" + headerData_.serialNumberString.c_str();
    }

    outLoggerDataFilePath_ += "_" + configurationType_ + "_" + firstDateString + "_" + firstTimeString + ".csv";
}

void FBLoggerDataReader::PrintConfigErrorsToLog()
{
    for (unsigned int i = 0; i < invalidInputFileList_.size(); i++)
    {
        logFileLine_ += "Error: File " + invalidInputFileList_[i];
        if (invalidInputFileErrorType_[i] == InvalidInputFileErrorType::CANNOT_OPEN_FILE)
        {
            logFileLine_ += " cannot be opened " + GetMyLocalDateTimeString() + "\n";
        }
        else if (invalidInputFileErrorType_[i] == InvalidInputFileErrorType::MISSING_HEADER)
        {
            logFileLine_ += " has no valid header " + GetMyLocalDateTimeString() + "\n";
        }
        else if (invalidInputFileErrorType_[i] == InvalidInputFileErrorType::MISSING_CONFIG)
        {
            logFileLine_ += " has no valid configuration " + GetMyLocalDateTimeString() + "\n";
        }
        else if (invalidInputFileErrorType_[i] == InvalidInputFileErrorType::ALREADY_OPEN)
        {
            logFileLine_ += "ERROR: The file " + outLoggerDataFilePath_ + " may be already opened by another process\n";
        }
        *pLogFile_ << logFileLine_;
    }
    logFileLine_ = "";
}

void FBLoggerDataReader::PrintLogFileLine()
{
    if (pLogFile_->good() && logFileLine_ != "")
    {
        *pLogFile_ << logFileLine_;
    }
    logFileLine_ = "";
}

void FBLoggerDataReader::PrintHeader()
{
    outputLine_ += "\"SN" + headerData_.serialNumberString + "\"," + "\"" + headerData_.latitudeDegreesString + " " +
        headerData_.latitudeDecimalMinutesString + "\",\"" + headerData_.longitudeDegreesString + " " +
        headerData_.longitudeDecimalMinutesString + "\", " + headerData_.sampleIntervalString + "\n";

    // Print header data to file
    if (pOutLoggerDataFile_)
    {
        *pOutLoggerDataFile_ << outputLine_;
        // Print column header data to file

        outputLine_ = status_.configColumnTextLine;
        *pOutLoggerDataFile_ << outputLine_;

        // Clear out output buffer for next function call
        outputLine_ = "";
    }
}

string FBLoggerDataReader::MakeStringWidthTwoFromInt(int headerData)
{
    string formattedString;
    if (headerData > 9)
    {
        formattedString = std::to_string(headerData);
    }
    else
    {
        formattedString = "0" + std::to_string(headerData);
    }
    return formattedString;
}

string FBLoggerDataReader::MakeStringWidthThreeFromInt(int headerData)
{
    string formattedString;
    if (headerData > 99)
    {
        formattedString = std::to_string(headerData);
    }
    else if (headerData > 9)
    {
        formattedString = "0" + std::to_string(headerData);
    }
    else
    {
        formattedString = "00" + std::to_string(headerData);
    }
    return formattedString;
}

void FBLoggerDataReader::UpdateSensorMaxAndMin()
{
    // Update max and min
    int currentReadingIndex = parsedNumericData_.channelNumber - 1;
    if (parsedNumericData_.parsedIEEENumber < currentFileStats_.columnMin[currentReadingIndex])
    {
        currentFileStats_.columnMin[currentReadingIndex] = parsedNumericData_.parsedIEEENumber;
    }
    if (parsedNumericData_.parsedIEEENumber > currentFileStats_.columnMax[currentReadingIndex])
    {
        currentFileStats_.columnMax[currentReadingIndex] = parsedNumericData_.parsedIEEENumber;
    }
}

void FBLoggerDataReader::PrintSensorDataOutput()
{
    const unsigned int miliSecondWidth = 3;

    string dayString;
    string monthString;
    string yearString;
    string hourString;
    string minuteString;
    string secondString;
    string millisecondsString;
    string recordString;

    outputLine_ = "";

    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        if (i == 0)
        {
            status_.recordNumber++;
            yearString = MakeStringWidthTwoFromInt(headerData_.year);
            monthString = MakeStringWidthTwoFromInt(headerData_.month);
            dayString = MakeStringWidthTwoFromInt(headerData_.day);
            hourString = MakeStringWidthTwoFromInt(headerData_.hours);
            minuteString = MakeStringWidthTwoFromInt(headerData_.minutes);
            secondString = MakeStringWidthTwoFromInt(headerData_.seconds);
            millisecondsString = MakeStringWidthThreeFromInt(headerData_.milliseconds);
            recordString = MakeStringWidthThreeFromInt(status_.recordNumber);
            // Print out time information parsed from the header
            outputLine_ += "\"" + yearString + "-" + monthString + "-" + dayString + " " +
                hourString + ":" + minuteString + ":" + secondString + "." + millisecondsString +
                "\"," + recordString + ",";
        }

        if (configurationType_ == "F")
        {
            if ((sanityChecks_.FFinalMin[i] != sanityChecks_.NAMin) && (sanityChecks_.FFinalMax[i] != sanityChecks_.NAMax))
            {
                outputLine_ += std::to_string(status_.sensorReadingValue[i]);
            }
            else
            {
                outputLine_ += std::to_string(0.0);
            }
        }
        else if (configurationType_ == "H")
        {
            if ((sanityChecks_.HFinalMin[i] != sanityChecks_.NAMin) && (sanityChecks_.HFinalMax[i] != sanityChecks_.NAMax))
            {
                outputLine_ += std::to_string(status_.sensorReadingValue[i]);
            }
            else
            {
                outputLine_ += std::to_string(0.0);
            }
        }
        else if(configurationType_ == "T")
        {
            outputLine_ += std::to_string(status_.sensorReadingValue[i]);
        }

        if (i < 8)
        {
            outputLine_ += ",";
        }
        else
        {
            outputLine_ += "\n";

            // Print to file
            if (pOutLoggerDataFile_)
            {
                *pOutLoggerDataFile_ << outputLine_;
                // Clear out output buffer for next function call
                outputLine_ = "";
            }
        }
    }
}

float FBLoggerDataReader::UnsignedIntToIEEEFloat(uint32_t binaryNumber)
{
    // Cast the unsigned 32 bit int as a float
    float *pFloat;
    pFloat = (float *)(&binaryNumber);
    float value = *pFloat;

    return value;
}
