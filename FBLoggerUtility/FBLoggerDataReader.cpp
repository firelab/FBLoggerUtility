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
#include <iomanip>
#include <ctype.h>

template<typename T>
std::vector<std::vector<T>> make_2d_vector(std::size_t rows, std::size_t cols)
{
    return std::vector<std::vector<T>>(rows, std::vector<T>(cols));
}

FBLoggerDataReader::FBLoggerDataReader(string dataPath, string burnName)
    :logFilePath_(dataPath + "\\log_file.txt"),
    logFile_(logFilePath_, std::ios::out),
    gpsFilePath_(dataPath + "\\" + burnName + "_gps.txt"),
    gpsFile_(gpsFilePath_, std::ios::out),
    kmlFilePath_(dataPath + "\\" + burnName + ".kml"),
    kmlFile_(kmlFilePath_, std::ios::out)
{
    logFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    gpsFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    kmlFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    startClock_ = clock();

    numFilesProcessed_ = 0;
    numInvalidInputFiles_ = 0;
    numInvalidOutputFiles_ = 0;
    currentFileIndex_ = 0;
    numErrors_ = 0;
    serialNumber_ = -1;
    outDataLine_ = "";
    gpsFileLine_ = "";
    dataPath_ = "";
    appPath_ = "";

    configFilePath_ = "";
    isConfigFileValid_ = false;
    outputsAreGood_ = false;
    lineStream_.str("");
    lineStream_.clear();

    outDataFilePath_ = "";
    rawOutDataFilePath_ = "";

    inputFilesNameList_.clear();
    invalidInputFileList_.clear();
    invalidInputFileErrorType_.clear();
    configMap_.clear();

    totalTimeInSeconds_ = 0.0;

    printRaw_ = false;

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

    SetDataPath(dataPath);

    string logFileNoExtension = dataPath_ + "log_file";

    // Check for output file's existence
    if (logFile_.fail())
    {
        logFile_.close();
        logFile_.clear();
        
        SYSTEMTIME systemTime;
        GetLocalTime(&systemTime);

        string dateTimeString = MakeStringWidthTwoFromInt(systemTime.wDay) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + std::to_string(systemTime.wYear) +
            "_" + MakeStringWidthTwoFromInt(systemTime.wHour) + "H" + MakeStringWidthTwoFromInt(systemTime.wMinute) + "M" +
            MakeStringWidthTwoFromInt(systemTime.wSecond) + "S";

        logFileLine_ = "ERROR: default log file at " + logFilePath_ + " already opened or otherwise unwritable\n";
        logFilePath_ = logFileNoExtension + "_" + dateTimeString + ".txt";
        logFile_.open(logFilePath_, std::ios::out);
    }
    if (logFile_.fail())
    {
        // Something really went wrong here
        logFile_.close();
        logFile_.clear();
    }
    ResetCurrentFileStats();
}

FBLoggerDataReader::~FBLoggerDataReader()
{   
    if (logFile_.fail())
    {
        logFile_.close();
        logFile_.clear();
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

        outDataLine_ = "";
        configurationType_ = "";
        status_.isGoodOutput = true;

        ResetCurrentFileStats();

        string placemarkName = "";
        string placemarkDescription = "";

        // Check for input file's existence
        if (inFile.fail())
        {
            inFile.close();
            // Report error
            logFileLine_ = "ERROR: Input file " + inDataFilePath_ + " does not exist\n";
            PrintLogFileLine();
        }
        else
        {
            // Clear file content vector 
            inputFileContents_.clear();
            // Store input file in RAWM
            StoreInputFileContentsToRAM(inFile);

            if (inputFileContents_.size() > 0)
            {
                ResetInFileReadingStatus();
                bool headerFound = GetFirstHeader();
                DegreesDecimalMinutesToDecimalDegrees(headerData_);
                if (configMap_.find(atoi(headerData_.serialNumberString.c_str())) != configMap_.end())
                {
                    configurationType_ = configMap_.find(atoi(headerData_.serialNumberString.c_str()))->second;
                }

                if ((headerFound) && (configurationType_ != ""))
                {
                    status_.loggingSession++;
                    currentFileStats_.fileName = infileName;
                    SetOutFilePaths(infileName);

                    ofstream outDataFile(outDataFilePath_, std::ios::out);
                    ofstream rawOutDataFile;

                    if (printRaw_)
                    {
                        rawOutDataFile.open(rawOutDataFilePath_, std::ios::out);
                    }

                    // Check for output file's existence and is not open by another process
                    if (outDataFile.fail())
                    {
                        inFile.close();
                        outDataFile.close();
                        // Report error
                        logFileLine_ = "ERROR: Unable to open " + outDataFilePath_ + "\n";
                        PrintLogFileLine();
                        status_.isGoodOutput = false;
                    }

                    if (printRaw_ && rawOutDataFile.fail())
                    {
                        rawOutDataFile.close();
                        // Report error
                        logFileLine_ = "ERROR: Unable to open " + rawOutDataFilePath_ + "\n";
                        PrintLogFileLine();
                        status_.isGoodOutput = false;
                    }

                    // We can read the input file
                    else
                    {
                        status_.isGoodOutput = true;
                        SetConfigDependentValues();
                        PrintHeader(outDataFile, OutFileType::PROCESSED);
                        // If this is the first file processed, print out gps and kml file headers
                        if (numFilesProcessed_ == 0)
                        {
                            BeginKMLFile();
                            PrintGPSFileHeader();
                        }
                        if (printRaw_)
                        {
                            PrintHeader(rawOutDataFile, OutFileType::RAW);
                        }
                        while (status_.pos < inputFileContents_.size())
                        {
                            ReadNextHeaderOrNumber();

                            if (status_.headerFound)
                            {
                                // Reset record and sensor reading count
                                status_.recordNumber = 0;
                                status_.sensorReadingCounter = 0;
                                DegreesDecimalMinutesToDecimalDegrees(headerData_);
                                // Print gps data from header to gps and kml files
                                PrintGPSFileLine(); 
                                kmlFile_ << FormatPlacemark();
                                // Get header data and print it to file
                                GetHeader();
                                status_.loggingSession++;
                                // Store new header time as start time (and possible end time) for next logging session
                                StoreSessionStartTime();
                                StoreSessionEndTime();
                                PrintHeader(outDataFile, OutFileType::PROCESSED);
                                if (printRaw_)
                                {
                                    PrintHeader(outDataFile, OutFileType::RAW);
                                }
                            }

                            UpdateSensorMaxAndMin();
                            if (status_.sensorReadingCounter == 9)
                            {
                                // An entire row of sensor data has been read in
                                status_.recordNumber++;
                                PerformSanityChecksOnValues(SanityChecks::RAW);
                                if (printRaw_)
                                {
                                    PrintSensorDataOutput(rawOutDataFile);
                                }
                                PerformNeededDataConversions();
                                PerformSanityChecksOnValues(SanityChecks::FINAL);
                                UpdateStatsFileMap();
                                PrintSensorDataOutput(outDataFile);
                                // Reset sensor reading counter 
                                status_.sensorReadingCounter = 0;
                                // Get (possible) session end time
                                StoreSessionEndTime();
                                // Update time for next set of sensor readings
                                headerData_.milliseconds += headerData_.sampleInterval;
                                UpdateTime();
                            }
                        }
                    }

                    // Print to gps file
                    DegreesDecimalMinutesToDecimalDegrees(headerData_);
                    PrintGPSFileLine();
                    // Print to kml file
                    int serial = atoi(headerData_.serialNumberString.c_str());
                    placemarkName = to_string(serial) + "_s" + to_string(status_.loggingSession);
                    placemarkDescription = "test";
                    kmlFile_ << FormatPlacemark();

                    // Close the file streams
                    inFile.close();
                    outDataFile.close();
                    if (printRaw_)
                    {
                        rawOutDataFile.close();
                    }
                }
            }
            else
            {
                // An error occurred
                inFile.close();
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

    if(logFile_.good())
    {
        PrintLogFileLine();
        PrintCarryBugToLog();
    }

    numFilesProcessed_++;
    currentFileIndex_++;

    if (numFilesProcessed_ == inputFilesNameList_.size())
    {
        EndKMLFile();
        kmlFile_.close();
    }
}

void FBLoggerDataReader::CheckConfig()
{
    if (logFile_.good())
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

void FBLoggerDataReader::ReportAbort()
{
    numErrors_++;
    if (logFile_.good())
    {
        logFileLine_ = "Conversion aborted by user before completion at " + GetMyLocalDateTimeString() + "\n";
        logFile_ << logFileLine_;
    }
    logFileLine_ = "";

    if (gpsFile_.is_open())
    {
        gpsFile_.close();
    }
    if (kmlFile_.is_open())
    {
        EndKMLFile();
        kmlFile_.close();
    }
}

double FBLoggerDataReader::CalculateHeatFlux(double rawVoltage)
{
	if (true)	//if this is after the test gorse burn in 2018, we need to add 1.25 volts
		rawVoltage = rawVoltage + 1.25;

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
    bool tempOneGood = (temperatureOne <= sanityChecks_.temperatureMax) && (temperatureOne >= sanityChecks_.temperatureMin);
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
        temperature = temperatureTwo;
    }
    else
    {
        // Error in logger data
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_U] = SanityChecks::INVALID_SENSOR_READING;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_V] = SanityChecks::INVALID_SENSOR_READING;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_W] = SanityChecks::INVALID_SENSOR_READING;
        return -1;
    }

    if (convertVelocity_.convert(temperature, pressureVoltageU, pressureVoltageV, pressureVoltageW, sensorBearing, pressureCoefficients_, angles_, ReynloldsNumbers_) == true)
    {
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_U] = convertVelocity_.u;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_V] = convertVelocity_.v;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_W] = convertVelocity_.w;
    }
    else
    {
        // Error in velocity conversion
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_U] = SanityChecks::INVALID_VELCOCITY_CONVERSION;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_V] = SanityChecks::INVALID_VELCOCITY_CONVERSION;
        status_.sensorReadingValue[HeatFluxIndex::PRESSURE_SENSOR_W] = SanityChecks::INVALID_VELCOCITY_CONVERSION;
        return -1;
    }
    return 0; // success
}

double FBLoggerDataReader::CalculateFIDPackagePressure(double rawVoltage)
{
    return 249.09 * (rawVoltage - 1.550) * 0.8065;
}

double FBLoggerDataReader::CalculateTCTemperature(double rawVoltage)
{
	double temperatureMillivolt;
    //--------------------------------------------------------------------------------------
	//The code below is to fix a problem with data loggers incorrectly converting voltage to
	//    temperature.  This only occurred on the first day of burns in New Zealand in
	//    February/March of 2018.  Currently the loggers now save raw voltage to the raw
	//    data file. Set to "true" if this is day 1 data, set to "false" if this is good
	//    data from a fixed logger.
	
	if (false)	
	{
		temperatureMillivolt = (4.6433525E-2 + 4.8477860E-2 * rawVoltage) / (1.0 + 1.1184923E-3 * rawVoltage - 2.0748220E-8 * rawVoltage * rawVoltage);
	}
	else
	{
		double panelTemp = status_.sensorReadingValue[TCIndex::PANEL_TEMP];
		double panelMillivolt = 2.0 * pow(10, -5) * panelTemp * panelTemp + 0.0393 * panelTemp;
		temperatureMillivolt = panelMillivolt + rawVoltage * 1000.0;
	}
	
    return (24.319 * temperatureMillivolt) - (0.0621 * temperatureMillivolt * temperatureMillivolt) + 
        (0.00013 * temperatureMillivolt * temperatureMillivolt * temperatureMillivolt);
}

void FBLoggerDataReader::PerformNeededDataConversions()
{
    if (configurationType_ == "F")
    {
        // Convert temperature from volts to °C
        double temperatureVoltage = status_.sensorReadingValue[FIDPackageIndex::TEMPERATURE];
        status_.sensorReadingValue[FIDPackageIndex::TEMPERATURE] = CalculateTCTemperature(temperatureVoltage);

        double FIDPackagePressure = CalculateFIDPackagePressure(status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE]);
        bool FIDPackagePressureGood = (FIDPackagePressure <= sanityChecks_.pressureMax) && (FIDPackagePressure >= sanityChecks_.pressureMin);
        if (FIDPackagePressureGood)
        {
            status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE] = FIDPackagePressure;
        }
        else
        {
            status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE] = SanityChecks::INVALID_SENSOR_READING;
        }
    }
    else if (configurationType_ == "H")
    {
        // Convert temperature from volts to °C
        double temperatureVoltageOne = status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_ONE];
        status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_ONE] = CalculateTCTemperature(temperatureVoltageOne);
        double temperatureVoltageTwo = status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_TWO];
        status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_TWO] = CalculateTCTemperature(temperatureVoltageTwo);

        double heatFluxVoltage = status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_VOLTAGE];
        bool heatFluxVoltageGood = ((heatFluxVoltage <= sanityChecks_.heatFluxVoltageMax) && (heatFluxVoltage >= sanityChecks_.heatFluxVoltageMin));
        double heatFlux = SanityChecks::INVALID_SENSOR_READING;;
        if (heatFluxVoltageGood)
        {
            heatFlux = CalculateHeatFlux(status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_VOLTAGE]);
        }

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
    else if (configurationType_ == "T")
    {
        // Convert temperature from volts to °C
        double temperatureVoltage;
        for (int i = 0; i < 8; i++)
        {
            temperatureVoltage = status_.sensorReadingValue[i];
            status_.sensorReadingValue[i] = CalculateTCTemperature(temperatureVoltage);
        }
    }
}

void FBLoggerDataReader::PerformSanityChecksOnValues(SanityChecks::SanityCheckTypeEnum sanityCheckType)
{
    double minForSanityCheck = 0.0;
    double maxForSanityCheck = 0.0;

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

void FBLoggerDataReader::StoreInputFileContentsToRAM(ifstream & inFile)
{
    // Get size of file in bytes
    inFile.seekg(0, ios::end);
    unsigned int fileSizeInBytes = (unsigned int)inFile.tellg();
    inFile.seekg(0, ios::beg);

    inputFileContents_.reserve(fileSizeInBytes);
    inputFileContents_.assign(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>());
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
                // remove any whitespace or control chars from line
                line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
                line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '\r\n'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());
                if (line != "")
                {
                    ParseTokensFromLineOfConfigFile(line);
                    ProcessErrorsInLineOfConfigFile();
                }
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
        logFileLine_ = "Error: The entry in config file at " + configFilePath_ + " has more than the required 4 columns with first invalid entry at line " + std::to_string(status_.configFileLineNumber) + " " + GetMyLocalDateTimeString() + "\n";
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
}

void FBLoggerDataReader::CheckConfigForAllFiles()
{
    int codepage = CP_UTF8;
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
            // Check for input file's existence
            if (inFile.fail())
            {
                // Cannot open file
                numInvalidInputFiles_++;
                invalidInputFileList_.push_back(inDataFilePath_);
                invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::CANNOT_OPEN_FILE);
                inFile.close();
            }
            else
            {
                inputFileContents_.clear();
                StoreInputFileContentsToRAM(inFile);
                if (inputFileContents_.size() > 0)
                {
                    ResetInFileReadingStatus();
                    bool headerFound = GetFirstHeader();
                    int serialNumber = atoi(headerData_.serialNumberString.c_str());
                    if ((headerFound) && (configMap_.find(serialNumber) == configMap_.end()) || configMap_.find(serialNumber)->second == "")
                    {
                        // Config missing for file
                        numInvalidInputFiles_++;
                        invalidInputFileList_.push_back(inDataFilePath_);
                        invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::MISSING_CONFIG);
                        inFile.close();
                    }
                    else if (!headerFound)
                    {
                        // No header found in file
                        numInvalidInputFiles_++;
                        invalidInputFileList_.push_back(inDataFilePath_);
                        invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::MISSING_HEADER);
                        inFile.close();
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
    status_.configColumnTextLine = "";
    status_.loggingSession = 0;

    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        status_.columnRawType[i] = "";
        status_.sensorReadingValue[i] = 0.0;
    }
    status_.headerFound = false;
    status_.carryBugEncountered_ = false;
}

void FBLoggerDataReader::ResetCurrentFileStats()
{
    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        currentFileStats_.columnFailedSanityCheckRaw[i] = false;
        currentFileStats_.columnFailedSanityCheckFinal[i] = false;
        currentFileStats_.columnMin[i] = SanityChecks::INITIAL_MIN;
        currentFileStats_.columnMax[i] = SanityChecks::INITIAL_MAX;
    }
    currentFileStats_.isFailedSanityCheckFinal = false;
    currentFileStats_.isFailedSanityCheckRaw = false;
}

void FBLoggerDataReader::SetConfigDependentValues()
{
    if (configurationType_ == "F")
    {
        status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"FID(V)\",\"P(Pa)\",\"NA\",\"NA\",\"NA\",\"NA\",\"NA\",\"Panel Temp (°C)\"\n";
        status_.rawConfigColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"Temp(V)\",\"FID(V)\",\"P(V)\",\"NA\",\"NA\",\"NA\",\"NA\",\"NA\",\"Panel Temp (V)\"\n";
        status_.columnRawType[0] = "V";
        status_.columnRawType[1] = "FID(V)";
        status_.columnRawType[2] = "P(V)";
        for (int i = 3; i < 8; i++)
        {
            status_.columnRawType[i] = "NA";
        }
    }
    else if (configurationType_ == "H")
    {
        status_.configColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"°C\",\"°C\",\"u(m/s)\",\"v(m/s)\",\"w(m/s)\",\"HF(kW/m^2)\",\"HFT(C)\",\"NA\",\"Panel Temp (°C)\"\n";
        status_.rawConfigColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"Temp(V)\",\"Temp(V)\",\"u(V)\",\"v(V)\",\"w(V)\",\"HF(V)\",\"HFT(V)\",\"NA\",\"Panel Temp (V)\"\n";
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
        status_.rawConfigColumnTextLine = "\"TIMESTAMP\",\"RECORD\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Panel Temp (Temp(V))\"\n";
        for (int i = 0; i < 8; i++)
        {
            status_.columnRawType[i] = "V";
        }
    }
    // Common among all configurations
    status_.columnRawType[8] = "Panel Temp (V)";
}

void FBLoggerDataReader::PrintStatsFile()
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

        outStatsFile.open(statsFilePath_, std::ios::out);
    }

    outputsAreGood_ = (numInvalidInputFiles_ == 0) && outStatsFile.good() && logFile_.good();

    if (outStatsFile.good())
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
            outStatsFile << outputLine;
        }

        outputLine = "";
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
        outStatsFile << outputLine;

        outputLine = "";
        for (auto it = statsFileMap_.begin(); it != statsFileMap_.end(); it++)
        {
            outputLine = it->second.fileName + ",";

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

            outStatsFile << outputLine;
        }
    }
    outStatsFile.close();
}

void FBLoggerDataReader::PrintFinalReportToLog()
{
    totalTimeInSeconds_ = ((double)clock() - (double)startClock_) / (double)CLOCKS_PER_SEC;
    string dataPathOutput = dataPath_.substr(0, dataPath_.size() - 1);
    if (logFile_.good() && logFileLine_ == "" && invalidInputFileList_.empty())
    {
        if ((numFilesProcessed_ > 0) && (numErrors_ == 0))
        {
            logFileLine_ = "Successfully processed " + std::to_string(numFilesProcessed_) + " DAT files in " + std::to_string(totalTimeInSeconds_) + " seconds into \"" + dataPathOutput + "\" " + GetMyLocalDateTimeString() + "\n";
        }
        else if ((numFilesProcessed_ > 0) && (numErrors_ > 0))
        {
            logFileLine_ = "Processed " + std::to_string(numFilesProcessed_) + " DAT files with " + std::to_string(numErrors_) + " errors in " + std::to_string(totalTimeInSeconds_) + " seconds in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        else
        {
            logFileLine_ = "No DAT files found in " + dataPathOutput + " " + GetMyLocalDateTimeString() + "\n";
        }
        logFile_ << logFileLine_;
    }
}

void FBLoggerDataReader::UpdateStatsFileMap()
{
    statsFileMap_.insert(pair<int, StatsFileData>(serialNumber_, currentFileStats_));
}

void FBLoggerDataReader::StoreSessionStartTime()
{
    startTimeForSession_.year = headerData_.year;
    startTimeForSession_.yearString = headerData_.yearString;
    startTimeForSession_.month = headerData_.month;
    startTimeForSession_.monthString = headerData_.monthString;
    startTimeForSession_.day = headerData_.day;
    startTimeForSession_.dayString = headerData_.dayString;
    startTimeForSession_.hours = headerData_.hours;
    startTimeForSession_.hourString = headerData_.hourString;
    startTimeForSession_.minutes = headerData_.minutes;
    startTimeForSession_.minuteString = headerData_.minuteString;
    startTimeForSession_.seconds = headerData_.seconds;
    startTimeForSession_.secondString = headerData_.secondString;
    startTimeForSession_.milliseconds = headerData_.milliseconds;
    startTimeForSession_.millisecondString = headerData_.millisecondString;

    startTimeForSession_.yearString = MakeStringWidthTwoFromInt(startTimeForSession_.year);
    startTimeForSession_.monthString = MakeStringWidthTwoFromInt(startTimeForSession_.month);
    startTimeForSession_.dayString = MakeStringWidthTwoFromInt(startTimeForSession_.day);
    startTimeForSession_.hourString = MakeStringWidthTwoFromInt(startTimeForSession_.hours);
    startTimeForSession_.minuteString = MakeStringWidthTwoFromInt(startTimeForSession_.minutes);
    startTimeForSession_.secondString = MakeStringWidthTwoFromInt(startTimeForSession_.seconds);
    startTimeForSession_.millisecondString = MakeStringWidthThreeFromInt(startTimeForSession_.milliseconds);

}

void FBLoggerDataReader::StoreSessionEndTime()
{
    endTimeForSession_.year = headerData_.year;
    endTimeForSession_.yearString = headerData_.yearString;
    endTimeForSession_.month = headerData_.month;
    endTimeForSession_.monthString = headerData_.monthString;
    endTimeForSession_.day = headerData_.day;
    endTimeForSession_.dayString = headerData_.dayString;
    endTimeForSession_.hours = headerData_.hours;
    endTimeForSession_.hourString = headerData_.hourString;
    endTimeForSession_.minutes = headerData_.minutes;
    endTimeForSession_.minuteString = headerData_.minuteString;
    endTimeForSession_.seconds = headerData_.seconds;
    endTimeForSession_.secondString = headerData_.secondString;
    endTimeForSession_.milliseconds = headerData_.milliseconds;
    endTimeForSession_.millisecondString = headerData_.millisecondString;

    endTimeForSession_.yearString = MakeStringWidthTwoFromInt(endTimeForSession_.year);
    endTimeForSession_.monthString = MakeStringWidthTwoFromInt(endTimeForSession_.month);
    endTimeForSession_.dayString = MakeStringWidthTwoFromInt(endTimeForSession_.day);
    endTimeForSession_.hourString = MakeStringWidthTwoFromInt(endTimeForSession_.hours);
    endTimeForSession_.minuteString = MakeStringWidthTwoFromInt(endTimeForSession_.minutes);
    endTimeForSession_.secondString = MakeStringWidthTwoFromInt(endTimeForSession_.seconds);
    endTimeForSession_.millisecondString = MakeStringWidthThreeFromInt(endTimeForSession_.milliseconds);
}

bool FBLoggerDataReader::CreateWindTunnelDataVectors()
{
    std::ifstream windTunnelDataFile(windTunnelDataPath_, std::ios::in);
    string line;

    double angle, ReynoldsNumber, pressureCoefficient;
    
    const int NUM_ANGLES = 100;
    const int NUM_REYNOLDS_NUMBERS = 10;
    const int NUM_DATA_LINES = NUM_ANGLES * NUM_REYNOLDS_NUMBERS;

    bool isSuccessful = false;
    angles_.clear();
    angles_.reserve(NUM_ANGLES);
    ReynloldsNumbers_.clear();
    ReynloldsNumbers_.reserve(NUM_REYNOLDS_NUMBERS);
    int lineCount = 0;
    if (windTunnelDataFile.good())
    {
        pressureCoefficients_ = make_2d_vector<double>(NUM_ANGLES, NUM_REYNOLDS_NUMBERS);
        for (int ReynoldsNumberIndex = 0; ReynoldsNumberIndex < NUM_REYNOLDS_NUMBERS; ReynoldsNumberIndex++)
        {
            for (int angleIndex = 0; angleIndex < NUM_ANGLES; angleIndex++)
            {
                getline(windTunnelDataFile, line);
                lineCount++;
                std::istringstream iss(line);
                iss >> angle >> ReynoldsNumber >> pressureCoefficient;

                if (angles_.size() < NUM_ANGLES)
                {
                    angles_.push_back(angle);
                }

                if (angleIndex == 0)
                {
                    ReynloldsNumbers_.push_back(ReynoldsNumber);
                }
                pressureCoefficients_[angleIndex][ReynoldsNumberIndex] = pressureCoefficient;
            }
        }
        if (lineCount == NUM_DATA_LINES)
        {
            isSuccessful = true;
        }
    }
    
    return isSuccessful;
}

void FBLoggerDataReader::DegreesDecimalMinutesToDecimalDegrees(HeaderData& headerData)
{
    // latitude
    double inputLatitudeDegrees = atof(headerData.latitudeDegreesString.c_str());
    double inputLatitudeDecimalMinutes = atof(headerData.latitudeDecimalMinutesString.c_str());
    headerData.decimalDegreesLatitude = inputLatitudeDegrees + (inputLatitudeDecimalMinutes / 60.0);
    if (headerData.northOrSouthHemisphere == "S")
    {
        headerData.decimalDegreesLatitude *= -1.0;
    }

    // longitude
    double inputLongitudeDegrees = atof(headerData.longitudeDegreesString.c_str());
    double inputLongitudeDecimalMinutes = atof(headerData.longitudeDecimalMinutesString.c_str());
    headerData.decimalDegreesLongitude = inputLongitudeDegrees + (inputLongitudeDecimalMinutes / 60.0);
    if (headerData.eastOrWestHemishere == "W")
    {
        headerData.decimalDegreesLongitude *= -1.0;
    }
}

void FBLoggerDataReader::BeginKMLFile()
{
    kmlFile_ << "<?xml version='1.0' encoding='utf-8'?>\n";
    kmlFile_ << "<kml xmlns='http://www.opengis.net/kml/2.2'>\n";
    kmlFile_ << "<Document>\n";
}

string FBLoggerDataReader::FormatPlacemark()
{
    std::ostringstream ss;

    string iconUrl = "";
    string iconColor = "";

    int serial = atoi(headerData_.serialNumberString.c_str());
    string serialString = to_string(serial);
    string sessionNumberString = to_string(status_.loggingSession);
    string placemarkName = serialString + "_s" + sessionNumberString;
    string description = "test";

    if (configurationType_ == "F")
    {
        iconUrl = iconsUrls_.fid;
        iconColor = "ff00ff00"; // Green Icon
    }
    else if (configurationType_ == "H")
    {
        iconUrl = iconsUrls_.heatFlux;
        iconColor = "ff0000ff"; // Red Icon
    }
    else if (configurationType_ == "T")
    {
        iconUrl = iconsUrls_.temperature;
        iconColor = "ff00ffff"; // Code for Yellow, produces Orange as icon is Red 
    }

    ss << setprecision(10) << fixed;
    ss << "<Placemark>\n"
        << "<name>" << placemarkName << "</name>\n"
        << "<description>" 
        << "Logger Serial: SN" << headerData_.serialNumberString << "\n"
        << "Longitude (DD): " << headerData_.decimalDegreesLongitude << "\n"
        << "Latitude (DD):  " << headerData_.decimalDegreesLatitude << "\n"
        << "Logging Session Number: " << sessionNumberString << "\n"
        << "Start Time (GMT): " << startTimeForSession_.yearString << "-" << startTimeForSession_.monthString 
        << "-" << startTimeForSession_.dayString <<" " << startTimeForSession_.hourString << ":" << startTimeForSession_.minuteString 
        << ":" << startTimeForSession_.secondString << "." << startTimeForSession_.millisecondString << "\n"
        << "End Time (GMT):   " << endTimeForSession_.yearString << "-" + endTimeForSession_.monthString << "-" << endTimeForSession_.dayString << " "
        << endTimeForSession_.hourString << ":" << endTimeForSession_.minuteString << ":" << endTimeForSession_.secondString << "."
        << endTimeForSession_.millisecondString << "\n"
        << "</description>\n"
        << "<Point>\n"
        << "<coordinates>"
        << headerData_.decimalDegreesLongitude << "," << headerData_.decimalDegreesLatitude << ",0"
        << "</coordinates>\n"
        << "</Point>\n"
        << "<Style>"
        << "<IconStyle>"
        << "<color>" << iconColor << "</color>"
        << "<Icon>"
        << "<href>" << iconUrl << "</href>"
        << "</Icon> "
        << "</IconStyle>"
        << "</Style>"
        << "</Placemark>\n";

    return ss.str();
}

void FBLoggerDataReader::EndKMLFile()
{
    kmlFile_ << "</Document>\n";
    kmlFile_ << "</kml>\n";
}

void FBLoggerDataReader::SetPrintRaw(bool option)
{
    printRaw_ = option;
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

void FBLoggerDataReader::SetWindTunnelDataTablePath(string windTunnelDataTablePath)
{
    windTunnelDataPath_ = windTunnelDataTablePath;
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

unsigned int FBLoggerDataReader::GetNumberOfInputFiles()
{
    return (unsigned int)inputFilesNameList_.size();
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
    if (!logFile_.fail())
    {
        isGood = logFile_.good();
    }
    return isGood;
}

bool FBLoggerDataReader::IsDoneReadingDataFiles()
{
    return(currentFileIndex_ >= inputFilesNameList_.size());
}

bool FBLoggerDataReader::GetFirstHeader()
{
    ResetHeaderData();

    status_.pos = 0;
    size_t filePositionLimit = inputFileContents_.size() - BYTES_READ_PER_ITERATION;

    // Keep grabbing bytes until header is found or end of file is reached
    while (!status_.headerFound && (status_.pos < filePositionLimit))
    {
        GetRawNumber();
    }
    if (status_.headerFound)
    {
        // Reset record and sensor reading count
        status_.recordNumber = 0;
        status_.sensorReadingCounter = 0;
        // Get header data and print it to file
        GetHeader();
        ParseHeader();

        // Store new header time as start time for next logging session
        StoreSessionStartTime();
    }

    return status_.headerFound;
}

void FBLoggerDataReader::ReadNextHeaderOrNumber()
{
    status_.headerFound = false;
   
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
    int index = 0;
    status_.headerFound = false;

    // Always read in 4 bytes, then check if those 4 bytes contain the header signature
    for (int i = 0; i < 4; i++)
    {
        parsedNumericData_.rawHexNumber[i] = (uint8_t)inputFileContents_[status_.pos];
        status_.pos++;
    }

    CheckForHeader();
    // If it is not a header, read in 1 more byte for channel number
    if (!status_.headerFound)
    {
        parsedNumericData_.channelNumber = inputFileContents_[status_.pos];
        status_.pos++;
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

    if (headerData_.milliseconds >= 1000)
    {
        headerData_.milliseconds -= 1000;
        headerData_.seconds++;
    }
    if (headerData_.seconds >= 60)
    {
        headerData_.seconds -= 60;
        headerData_.minutes++;
    }
    if (headerData_.minutes >= 60)
    {
        headerData_.minutes -= 60;
        headerData_.hours++;
    }
    if (headerData_.hours >= 24)
    {
        headerData_.hours -= 24;
        headerData_.day++;
    }
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
     
string FBLoggerDataReader::GetMyLocalDateTimeString()
{
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);

    return std::to_string(systemTime.wYear) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + MakeStringWidthTwoFromInt(systemTime.wDay) +
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
    unsigned int currentPos = status_.pos;
    for (unsigned int i = currentPos; i < currentPos + 44; i++)
    {
        byte = inputFileContents_[i];
        headerData_.rawHeader += byte;
        // update reading position
        status_.pos++;
    }

    ParseHeader();
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
    headerData_.latitudeDegreesString = "00";
    headerData_.latitudeDecimalMinutesString = "00000000";
    for (int i = 7; i < 9; i++)
    {
        headerData_.latitudeDegreesString[i - 7] = headerData_.rawHeader[i];
    }
    headerData_.latitudeDegreesString += "°";
    for (int i = 9; i < 17; i++)
    {
        headerData_.latitudeDecimalMinutesString[i - 9] = headerData_.rawHeader[i];
    }
    headerData_.northOrSouthHemisphere = headerData_.latitudeDecimalMinutesString[7];
    headerData_.latitudeDecimalMinutesString[7] = '\'';
    headerData_.latitudeDecimalMinutesString += " " + headerData_.northOrSouthHemisphere;
    //Get the longitude
    headerData_.longitudeDegreesString = "000";
    headerData_.longitudeDecimalMinutesString = "00000000";
    for (int i = 17; i < 20; i++)
    {
        headerData_.longitudeDegreesString[i - 17] = headerData_.rawHeader[i];
    }
    headerData_.longitudeDegreesString += "°";
    for (int i = 20; i < 28; i++)
    {
        headerData_.longitudeDecimalMinutesString[i - 20] = headerData_.rawHeader[i];
    }
    headerData_.eastOrWestHemishere = headerData_.longitudeDecimalMinutesString[7];
    headerData_.longitudeDecimalMinutesString[7] = '\'';
    headerData_.longitudeDecimalMinutesString += " " + headerData_.eastOrWestHemishere;
    
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
    if (headerData_.secondString.back() == ':')
    {
        // The character ':' appears in the seconds portion of some headers
        // due to an arithmetic error in which 1 was added to the ones place
        // but was not carried over to the tens
        status_.carryBugEncountered_ = true;
  
        // Correct the seconds string
        char tensSecondsChar = headerData_.secondString[0];
        string  tensSecondsString(1, tensSecondsChar);
        int tensSecondsValue = std::stoi(tensSecondsString);
        tensSecondsValue++;
        tensSecondsString = std::to_string(tensSecondsValue);
        tensSecondsChar = tensSecondsString[0];
        char onesSecondsChar = '0';
        headerData_.secondString[0] = tensSecondsChar;
        headerData_.secondString[1] = onesSecondsChar;
        // Correct the seconds value
        headerData_.seconds = std::stoi(headerData_.secondString);

        // Now update the time to propogate the correct values
        UpdateTime();

        // Update the time string values
        headerData_.yearString = std::to_string(headerData_.year);
        headerData_.monthString = MakeStringWidthTwoFromInt(headerData_.month);
        headerData_.dayString = MakeStringWidthTwoFromInt(headerData_.day);
        headerData_.hourString = MakeStringWidthTwoFromInt(headerData_.hours);
        headerData_.minuteString = MakeStringWidthTwoFromInt(headerData_.minutes);
        headerData_.secondString = MakeStringWidthTwoFromInt(headerData_.seconds);
        headerData_.millisecondString = MakeStringWidthThreeFromInt(headerData_.milliseconds);
    }
}

void FBLoggerDataReader::SetOutFilePaths(string infileName)
{
    string firstDateString;
    string firstTimeString;

    firstDateString = headerData_.yearString + "-" + headerData_.monthString  + "-" + headerData_.dayString;
    firstTimeString = headerData_.hourString + "H" + headerData_.minuteString + "M";

    infileName = RemoveFileExtension(infileName);

    int infileIntValue = atoi(infileName.c_str());
    serialNumber_ = atoi(headerData_.serialNumberString.c_str());

    if (infileIntValue == serialNumber_ && IsOnlyDigits(infileName))
    {
        outDataFilePath_ = dataPath_ + "SN" + headerData_.serialNumberString.c_str();
        if (printRaw_)
        {
            rawOutDataFilePath_ = outDataFilePath_;
        }
    }
    else
    {
        outDataFilePath_ = dataPath_ + infileName + "_" + "SN" + headerData_.serialNumberString.c_str();
        if (printRaw_)
        {
            rawOutDataFilePath_ = outDataFilePath_;
        }
    }

    outDataFilePath_ += "_" + configurationType_ + "_" + firstDateString + "-" + firstTimeString;
    if (printRaw_)
    {
        rawOutDataFilePath_ = outDataFilePath_;
        rawOutDataFilePath_ += "_RAW.csv";
    }
    outDataFilePath_ += ".csv";
}

void FBLoggerDataReader::PrintCarryBugToLog()
{
    if (logFile_.good() && status_.carryBugEncountered_)
    {
        numErrors_++;
        logFileLine_ = "Error: File " + currentFileStats_.fileName + " has failure to carry the one in header\n";
        logFile_ << logFileLine_;
    }
    logFileLine_ = "";
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
            logFileLine_ += "ERROR: The file " + outDataFilePath_ + " may be already opened by another process\n";
        }
        logFile_ << logFileLine_;
    }
    logFileLine_ = "";
}

void FBLoggerDataReader::PrintGPSFileHeader()
{
    gpsFileLine_ = "file_name, logger_id, logging_session_number, configuration_type, longitude, latitude, start_time, end_time\n";
    gpsFile_ << gpsFileLine_;
}

void FBLoggerDataReader::PrintGPSFileLine()
{
    // file, logger unit, and gps data
    gpsFileLine_ = currentFileStats_.fileName + ",SN" + headerData_.serialNumberString + "," + to_string(status_.loggingSession) + 
        "," + configurationType_ + "," + to_string(headerData_.decimalDegreesLongitude) + "," +
        to_string(headerData_.decimalDegreesLatitude) + ",";

    // start time information for session
    gpsFileLine_ += startTimeForSession_.yearString + "-" + startTimeForSession_.monthString + "-" + startTimeForSession_.dayString + " " +
        startTimeForSession_.hourString + ":" + startTimeForSession_.minuteString + ":" + startTimeForSession_.secondString + "." + startTimeForSession_.millisecondString +
        ",";

    // end time information for session
    gpsFileLine_ += endTimeForSession_.yearString + "-" + endTimeForSession_.monthString + "-" + endTimeForSession_.dayString + " " +
        endTimeForSession_.hourString + ":" + endTimeForSession_.minuteString + ":" + endTimeForSession_.secondString + "." + endTimeForSession_.millisecondString + "\n";

    // print to file
    gpsFile_ << gpsFileLine_;
}

void FBLoggerDataReader::PrintLogFileLine()
{
    if (logFile_.good() && logFileLine_ != "")
    {
        logFile_ << logFileLine_;
    }
    logFileLine_ = "";
}

void FBLoggerDataReader::PrintHeader(ofstream& outFile, OutFileType::OutFileTypeEnum outFileType)
{
    outDataLine_ += "\"SN" + headerData_.serialNumberString + "\"," + "\"" + headerData_.latitudeDegreesString + " " +
        headerData_.latitudeDecimalMinutesString + "\",\"" + headerData_.longitudeDegreesString + " " +
        headerData_.longitudeDecimalMinutesString + "\", " + headerData_.sampleIntervalString + "\n";

    // Print header data to file
    if (outFile.good())
    {
        outFile << outDataLine_;

        // Print column header data to file
        if (outFileType == OutFileType::RAW)
        {
            outDataLine_ = status_.rawConfigColumnTextLine;
        }
        else
        {
            outDataLine_ = status_.configColumnTextLine;
        }
        outFile << outDataLine_;

        // Clear out output buffer for next function call
        outDataLine_ = "";
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

void FBLoggerDataReader::PrintSensorDataOutput(ofstream& outFile)
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

    outDataLine_ = "";

    for (int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        if (i == 0)
        {
            yearString = MakeStringWidthTwoFromInt(headerData_.year);
            monthString = MakeStringWidthTwoFromInt(headerData_.month);
            dayString = MakeStringWidthTwoFromInt(headerData_.day);
            hourString = MakeStringWidthTwoFromInt(headerData_.hours);
            minuteString = MakeStringWidthTwoFromInt(headerData_.minutes);
            secondString = MakeStringWidthTwoFromInt(headerData_.seconds);
            millisecondsString = MakeStringWidthThreeFromInt(headerData_.milliseconds);
            recordString = MakeStringWidthThreeFromInt(status_.recordNumber);
            // Print out time information parsed from the header
            outDataLine_ += "\"" + yearString + "-" + monthString + "-" + dayString + " " +
                hourString + ":" + minuteString + ":" + secondString + "." + millisecondsString +
                "\"," + recordString + ",";
        }

        if (configurationType_ == "F")
        {
            if ((sanityChecks_.FFinalMin[i] != sanityChecks_.NAMin) && (sanityChecks_.FFinalMax[i] != sanityChecks_.NAMax))
            {
                outDataLine_ += std::to_string(status_.sensorReadingValue[i]);
            }
            else
            {
                outDataLine_ += std::to_string(0.0);
            }
        }
        else if (configurationType_ == "H")
        {
            if ((sanityChecks_.HFinalMin[i] != sanityChecks_.NAMin) && (sanityChecks_.HFinalMax[i] != sanityChecks_.NAMax))
            {
                outDataLine_ += std::to_string(status_.sensorReadingValue[i]);
            }
            else
            {
                outDataLine_ += std::to_string(0.0);
            }
        }
        else if(configurationType_ == "T")
        {
            outDataLine_ += std::to_string(status_.sensorReadingValue[i]);
        }

        if (i < 8)
        {
            outDataLine_ += ",";
        }
        else
        {
            outDataLine_ += "\n";

            // Print to file
            if (outFile.good())
            {
                outFile << outDataLine_;
                // Clear out output buffer for next function call
                outDataLine_ = "";
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
