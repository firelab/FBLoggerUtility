#include "logger_data_reader.h"
#include "string_utility.h"
#include "directory_reader_utility.h"

#include <algorithm>
#include <ctype.h>
#include <limits.h>
#include <string>
#include <math.h>
#include <iomanip>
#include <fstream>

using std::ios;
using std::ofstream;

template<typename T>
std::vector<std::vector<T>> make_2d_vector(std::size_t rows, std::size_t cols)
{
    return std::vector<std::vector<T>>(rows, std::vector<T>(cols));
}

LoggerDataReader::LoggerDataReader(string dataPath, string burnName)

//kmlFilePath_(dataPath + "\\" + burnName + ".kml"),
//kmlFile_(kmlFilePath_, std::ios::out)
{
    //kmlFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    numFilesProcessed_ = 0;
    numInvalidInputFiles_ = 0;
    numInvalidOutputFiles_ = 0;
    currentFileIndex_ = 0;
    numErrors_ = 0;
    serialNumber_ = -1;
    outDataLines_ = "";
    gpsFileLines_ = "";
    burnName_ = "";
    dataPath_ = "";
    appPath_ = "";

    configFilePath_ = "";
    isConfigFileValid_ = false;
    outputsAreGood_ = false;
    lineStream_.str("");
    lineStream_.clear();

    outDataFilePath_ = "";
    rawOutDataFilePath_ = "";

    inputFilePathList_.clear();
    invalidInputFileList_.clear();
    invalidInputFileErrorType_.clear();
    configMap_.clear();

    printRaw_ = false;

    // Initialize F Config Sanity Checks
    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
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
    for(int i = 0; i < 9; i++)
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
    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        sanityChecks_.TMin[i] = sanityChecks_.temperatureMin;
        sanityChecks_.TMax[i] = sanityChecks_.temperatureMax;
    }

    SetDataPath(dataPath);

    ResetCurrentFileStats();
}

LoggerDataReader::LoggerDataReader(const SharedData& sharedData)
{
    //gpsFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    //kmlFile_.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    numFilesProcessed_ = 0;
    numInvalidInputFiles_ = 0;
    numInvalidOutputFiles_ = 0;
    currentFileIndex_ = 0;
    numErrors_ = 0;
    serialNumber_ = -1;
    outDataLines_ = "";
    burnName_ = "";
    dataPath_ = "";
    appPath_ = "";

    configFilePath_ = "";
    isConfigFileValid_ = false;
    outputsAreGood_ = false;
    lineStream_.str("");
    lineStream_.clear();

    outDataFilePath_ = "";
    rawOutDataFilePath_ = "";

    inputFilePathList_.clear();
    invalidInputFileList_.clear();
    invalidInputFileErrorType_.clear();
    configMap_.clear();

    printRaw_ = false;

    // Initialize F Config Sanity Checks
    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
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
    for(int i = 0; i < 9; i++)
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
    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        sanityChecks_.TMin[i] = sanityChecks_.temperatureMin;
        sanityChecks_.TMax[i] = sanityChecks_.temperatureMax;
    }

    ResetCurrentFileStats();
    SetSharedData(sharedData);
}

LoggerDataReader::~LoggerDataReader()
{

}

void LoggerDataReader::GetSharedData(SharedData& sharedData)
{
    sharedData.burnName = burnName_;
    sharedData.configFilePath = configFilePath_;
    sharedData.dataPath = dataPath_;
    sharedData.windTunnelDataTablePath = windTunnelDataTablePath_;
    sharedData.printRaw = printRaw_;
    sharedData.configMap = &configMap_;
    sharedData.heatFlux_X_VoltageOffsetMap = &heatFlux_X_VoltageOffsetMap_;
    sharedData.heatFlux_Y_VoltageOffsetMap = &heatFlux_Y_VoltageOffsetMap_;
    sharedData.heatFlux_Z_VoltageOffsetMap = &heatFlux_Z_VoltageOffsetMap_;
    sharedData.sensorBearingMap = &sensorBearingMap_;
    sharedData.statsFileMap = &statsFileMap_;
    sharedData.angles = &angles_;
    sharedData.ReynloldsNumbers = &ReynloldsNumbers_;
    sharedData.pressureCoefficients = &pressureCoefficients_;
    sharedData.inputFilePathList = &inputFilePathList_;
}

void LoggerDataReader::SetSharedData(const SharedData& sharedData)
{
    burnName_ = sharedData.burnName;
    configFilePath_ = sharedData.configFilePath;
    dataPath_ = sharedData.dataPath;
    windTunnelDataTablePath_ = sharedData.windTunnelDataTablePath;
    printRaw_ = sharedData.printRaw;
    if(sharedData.configMap != nullptr)
    {
        configMap_ = *sharedData.configMap;
    }
    if(sharedData.heatFlux_X_VoltageOffsetMap != nullptr &&
        sharedData.heatFlux_Y_VoltageOffsetMap != nullptr &&
        sharedData.heatFlux_Z_VoltageOffsetMap != nullptr)
    {
        heatFlux_X_VoltageOffsetMap_ = *sharedData.heatFlux_X_VoltageOffsetMap;
        heatFlux_Y_VoltageOffsetMap_ = *sharedData.heatFlux_Y_VoltageOffsetMap;
        heatFlux_Z_VoltageOffsetMap_ = *sharedData.heatFlux_Z_VoltageOffsetMap;
    }

    if(sharedData.sensorBearingMap != nullptr)
    {
        sensorBearingMap_ = *sharedData.sensorBearingMap;
    }
    if(sharedData.statsFileMap != nullptr)
    {
        statsFileMap_ = *sharedData.statsFileMap;
    }
    if(sharedData.angles != nullptr)
    {
        angles_ = *sharedData.angles;
    }
    if(sharedData.ReynloldsNumbers != nullptr)
    {
        ReynloldsNumbers_ = *sharedData.ReynloldsNumbers;
    }
    if(sharedData.pressureCoefficients != nullptr)
    {
        pressureCoefficients_ = *sharedData.pressureCoefficients;
    }
    if(sharedData.inputFilePathList != nullptr)
    {
        inputFilePathList_ = *sharedData.inputFilePathList;
    }
}

void LoggerDataReader::ProcessSingleDataFile(int fileIndex)
{
    string infileName = inputFilePathList_[fileIndex];
    string extension = "";

    int pos = infileName.find_last_of('/');
    if(pos != string::npos)
    {
        infileName = infileName.substr(pos + 1, infileName.size());
    }
    pos = infileName.find_last_of('.');
    if(pos != string::npos)
    {
        extension = infileName.substr(pos, infileName.size());
    }

    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if(extension == ".dat")
    {
        inDataFilePath_ = inputFilePathList_[fileIndex];

        std::ifstream inFile(inDataFilePath_, std::ios::in | std::ios::binary);

        outDataLines_ = "";
        configurationType_ = "";
        status_.isGoodOutput = true;

        ResetCurrentFileStats();

        string placemarkName = "";
        string placemarkDescription = "";

        // Check for input file's existence
        if(inFile.fail())
        {
            inFile.close();
            // Report error
            logFileLines_ += "ERROR: Input file " + inDataFilePath_ + " does not exist\n";
            //PrintLogFileLine();
        }
        else
        {
            // Clear file content vector 
            inputFileContents_.clear();

            // Store input file in RAM
            StoreInputFileContentsToRAM(inFile);

            if(inputFileContents_.size() > 0)
            {
                ResetInFileReadingStatus();
                bool headerFound = GetFirstHeader();
                DegreesDecimalMinutesToDecimalDegrees(headerData_);
                if(configMap_.find(atoi(headerData_.serialNumberString.c_str())) != configMap_.end())
                {
                    configurationType_ = configMap_.find(atoi(headerData_.serialNumberString.c_str()))->second;
                }

                if((headerFound) && (configurationType_ != ""))
                {
                    status_.loggingSession++;
                    currentFileStats_.fileName = infileName;
                    SetOutFilePaths(infileName);

                    ofstream outFile(outDataFilePath_, std::ios::out);
                    ofstream rawOutFile;

                    if(printRaw_)
                    {
                        rawOutFile.open(rawOutDataFilePath_, std::ios::out);
                    }

                    // Check for output file's existence and is not open by another process
                    if(outFile.fail())
                    {
                        inFile.close();
                        outFile.close();
                        // Report error
                        logFileLines_ += "ERROR: Unable to open " + outDataFilePath_ + "\n";
                        //PrintLogFileLine();
                        status_.isGoodOutput = false;
                    }

                    if(printRaw_ && rawOutFile.fail())
                    {
                        rawOutFile.close();
                        // Report error
                        logFileLines_ += "ERROR: Unable to open " + rawOutDataFilePath_ + "\n";
                        status_.isGoodOutput = false;
                    }

                    // We can read the input file
                    else
                    {
                        status_.isGoodOutput = true;
                        SetConfigDependentValues();
                        PrintHeader(OutFileType::PROCESSED);
                        // If this is the first file processed, print out gps and kml file headers
                        if(numFilesProcessed_ == 0)
                        {
                            //BeginKMLFile();
                            //PrintGPSFileHeader();
                        }
                        if(printRaw_)
                        {
                            PrintHeader(OutFileType::RAW);
                        }
                        while(status_.position < status_.filePositionLimit)
                        {
                            ReadNextHeaderOrNumber();

                            if(status_.headerFound)
                            {
                                for(int i = 0; i < NUM_SENSOR_READINGS; i++)
                                {
                                    status_.isChannelPresent[i] = false;
                                }
                                // Reset record and sensor reading count
                                status_.recordNumber = 0;
                                status_.sensorReadingCounter = 0;

                                // Clear sensor readings
                                for(int i = 0; i < NUM_SENSOR_READINGS; i++)
                                {
                                    status_.sensorReadingValue[i] = 0.0;
                                }

                                DegreesDecimalMinutesToDecimalDegrees(headerData_);
                                // Print gps data from header to gps and kml files
                                //PrintGPSFileLine(); 
                                //kmlFile_ << FormatPlacemark();
                                // Get header data and print it to file
                                GetHeader();
                                status_.loggingSession++;
                                // Store new header time as start time (and possible end time) for next logging session
                                StoreSessionStartTime();
                                StoreSessionEndTime();
                                PrintHeader(OutFileType::PROCESSED);
                                if(printRaw_)
                                {
                                    PrintHeader(OutFileType::RAW);
                                }
                            }

                            UpdateSensorMaxAndMin();
                            if(status_.sensorReadingCounter == NUM_SENSOR_READINGS)
                            {
                                for(int i = 0; i < NUM_SENSOR_READINGS; i++)
                                {
                                    status_.isChannelPresent[i] = false;
                                }

                                // An entire row of sensor data has been read in
                                status_.recordNumber++;

                                PerformSanityChecksOnValues(SanityChecks::RAW);
                                PerformNeededDataConversions();
                                PerformSanityChecksOnValues(SanityChecks::FINAL);
                                UpdateStatsFileMap();

                                // Update time for next set of sensor readings        
                                UpdateTime();
                                // Get (possible) session end time
                                StoreSessionEndTime();

                                PrintSensorDataOutput(OutFileType::PROCESSED);

                                if(printRaw_)
                                {
                                    PrintSensorDataOutput(OutFileType::RAW);
                                }

                                // Reset sensor reading counter 
                                status_.sensorReadingCounter = 0;
                            }
                        }
                    }

                    // Print to gps file
                    DegreesDecimalMinutesToDecimalDegrees(headerData_);
                    PrintGPSFileLine();

                    //// Print to kml file
                    //int serial = atoi(headerData_.serialNumberString.c_str());
                    //placemarkName = to_string(serial) + "_s" + to_string(status_.loggingSession);
                    //placemarkDescription = "test";
                    //kmlFile_ << FormatPlacemark();

                    if(status_.isCarryBugEncountered_ == true)
                    {
                        PrintCarryBugToLog();
                    }

                    if(status_.corruptedRecordNumbers.size() > 0)
                    {
                        PrintCorruptionDetectionsToLog();
                    }

                    PrintOutDataLinesToFile(outFile, outDataLines_);

                    // Close the file streams
                    inFile.close();
                    outFile.close();
                    if(printRaw_)
                    {
                        PrintOutDataLinesToFile(rawOutFile, rawOutDataLines_);
                        rawOutFile.close();
                    }
                }
            }
            else
            {
                // An error occurred
                inFile.close();
            }
        }

        if(!status_.isGoodOutput)
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

    //if (logFile_.good())
    //{
    //    PrintLogFileLine();
    //}

    numFilesProcessed_++;
    currentFileIndex_++;

    //if (numFilesProcessed_ == inputFilesNameList_.size())
    //{
    //    EndKMLFile();
    //    kmlFile_.close();
    //}
}

void LoggerDataReader::CheckConfig()
{
    ReadConfig();

    if(isConfigFileValid_)
    {
        //ReadDirectoryIntoStringVector(dataPath_, inputFilesNameList_);
        CheckConfigForAllFiles();
    }
}

void LoggerDataReader::ReportAbort()
{
    numErrors_++;
    logFileLines_ += "Conversion aborted by user before completion at " + GetMyLocalDateTimeString() + "\n";
}

double LoggerDataReader::CalculateHeatFlux(double rawVoltage)
{
    //if (true)	//if this is after the test gorse burn in 2018, we need to add 1.25 volts
    //{
    //    rawVoltage = rawVoltage + 1.25;
    //}
    //return 56.60 * pow(rawVoltage, 1.129);
    return  43.789 * (rawVoltage + 1.25);
}

double LoggerDataReader::CalculateHeatFluxTemperature(double rawVoltage)
{
    double thermistorResistance = 10000.0 / ((-1.25 / rawVoltage) - 1.0);
    double heatFluxTemperature = 1.0 / (0.003356 - (std::log10(33580 / thermistorResistance) / 3962.0)) - 273.0;

    return heatFluxTemperature;
}

int LoggerDataReader::CalculateHeatFluxComponentVelocities(double temperatureOne, double temperatureTwo,
    double pressureVoltageU, double pressureVoltageV, double pressureVoltageW, double sensorBearing)
{
    bool tempOneGood = (temperatureOne <= sanityChecks_.temperatureMax) && (temperatureOne >= sanityChecks_.temperatureMin);
    bool tempTwoGood = (temperatureTwo <= sanityChecks_.temperatureMax) && (temperatureTwo >= sanityChecks_.temperatureMin);
    double temperature = 0.0;

    // Calcuate average if both temps are good, or use the only good value
    if(tempOneGood && tempTwoGood)
    {
        temperature = (temperatureOne + temperatureTwo) / 2.0;
    }
    else if(tempOneGood)
    {
        temperature = temperatureOne;
    }
    else if(tempTwoGood)
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

    if(convertVelocity_.convert(temperature, pressureVoltageU, pressureVoltageV, pressureVoltageW, sensorBearing, pressureCoefficients_, angles_, ReynloldsNumbers_) == true)
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

double LoggerDataReader::CalculateFIDPackagePressure(double rawVoltage)
{
    return 249.09 * (rawVoltage - 1.550) * 0.8065;
}

double LoggerDataReader::CalculateTCTemperature(double rawVoltage)
{
    double temperatureMillivolt;
    //--------------------------------------------------------------------------------------
    //The code below is to fix a problem with data loggers incorrectly converting voltage to
    //    temperature.  This only occurred on the first day of burns in New Zealand in
    //    February/March of 2018.  Currently the loggers now save raw voltage to the raw
    //    data file. Set to "true" if this is day 1 data, set to "false" if this is good
    //    data from a fixed logger.

    //if (false)	
    //{
    //	temperatureMillivolt = (4.6433525E-2 + 4.8477860E-2 * rawVoltage) / (1.0 + 1.1184923E-3 * rawVoltage - 2.0748220E-8 * rawVoltage * rawVoltage);
    //}
    //else
    //{
    //	double panelTemp = status_.sensorReadingValue[TCIndex::PANEL_TEMP];
    //	double panelMillivolt = 2.0 * pow(10, -5) * panelTemp * panelTemp + 0.0393 * panelTemp;
    //	temperatureMillivolt = panelMillivolt + rawVoltage * 1000.0;
    //}
    //
 //   return (24.319 * temperatureMillivolt) - (0.0621 * temperatureMillivolt * temperatureMillivolt) + 
 //       (0.00013 * temperatureMillivolt * temperatureMillivolt * temperatureMillivolt);
    return rawVoltage;
}

void LoggerDataReader::PerformNeededDataConversions()
{
    if(configurationType_ == "F")
    {
        // Convert temperature from volts to C
        double temperatureVoltage = status_.sensorReadingValue[FIDPackageIndex::TEMPERATURE];
        status_.sensorReadingValue[FIDPackageIndex::TEMPERATURE] = CalculateTCTemperature(temperatureVoltage);

        double FIDPackagePressure = CalculateFIDPackagePressure(status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE]);
        bool FIDPackagePressureGood = (FIDPackagePressure <= sanityChecks_.pressureMax) && (FIDPackagePressure >= sanityChecks_.pressureMin);
        if(FIDPackagePressureGood)
        {
            status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE] = FIDPackagePressure;
        }
        else
        {
            status_.sensorReadingValue[FIDPackageIndex::PRESSURE_VOLTAGE] = SanityChecks::INVALID_SENSOR_READING;
        }
    }
    else if(configurationType_ == "H")
    {
        double xVoltageOffset = heatFlux_X_VoltageOffsetMap_.find(serialNumber_)->second;
        double yVoltageOffset = heatFlux_Y_VoltageOffsetMap_.find(serialNumber_)->second;
        double zVoltageOffset = heatFlux_Z_VoltageOffsetMap_.find(serialNumber_)->second;

        // Convert temperature from volts to C
        double temperatureVoltageOne = status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_ONE];
        status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_ONE] = CalculateTCTemperature(temperatureVoltageOne);
        double temperatureVoltageTwo = status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_TWO];
        status_.sensorReadingValue[HeatFluxIndex::TEMPERATURE_SENSOR_TWO] = CalculateTCTemperature(temperatureVoltageTwo);

        double heatFluxVoltage = status_.sensorReadingValue[HeatFluxIndex::HEAT_FLUX_VOLTAGE];
        bool heatFluxVoltageGood = ((heatFluxVoltage <= sanityChecks_.heatFluxVoltageMax) && (heatFluxVoltage >= sanityChecks_.heatFluxVoltageMin));
        double heatFlux = SanityChecks::INVALID_SENSOR_READING;;
        if(heatFluxVoltageGood)
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
    else if(configurationType_ == "T")
    {
        // Convert temperature from volts to C
        double temperatureVoltage;
        for(int i = 0; i < 8; i++)
        {
            temperatureVoltage = status_.sensorReadingValue[i];
            status_.sensorReadingValue[i] = CalculateTCTemperature(temperatureVoltage);
        }
    }
}

void LoggerDataReader::PerformSanityChecksOnValues(SanityChecks::SanityCheckTypeEnum sanityCheckType)
{
    double minForSanityCheck = 0.0;
    double maxForSanityCheck = 0.0;

    for(unsigned int currentIndex = 0; currentIndex < NUM_SENSOR_READINGS; currentIndex++)
    {
        double currentValue = status_.sensorReadingValue[currentIndex];
        if(configurationType_ == "F")
        {
            if(sanityCheckType == SanityChecks::RAW)
            {
                minForSanityCheck = sanityChecks_.FRawMin[currentIndex];
                maxForSanityCheck = sanityChecks_.FRawMax[currentIndex];
            }
            else if(sanityCheckType == SanityChecks::FINAL)
            {
                minForSanityCheck = sanityChecks_.FFinalMin[currentIndex];
                maxForSanityCheck = sanityChecks_.FFinalMax[currentIndex];
            }
        }
        else if(configurationType_ == "H")
        {
            if(sanityCheckType == SanityChecks::RAW)
            {
                minForSanityCheck = sanityChecks_.HRawMin[currentIndex];
                maxForSanityCheck = sanityChecks_.HRawMax[currentIndex];
            }
            else if(sanityCheckType == SanityChecks::FINAL)
            {
                minForSanityCheck = sanityChecks_.HFinalMin[currentIndex];
                maxForSanityCheck = sanityChecks_.HFinalMax[currentIndex];
            }
        }
        else if(configurationType_ == "T")
        {
            if(sanityCheckType == SanityChecks::RAW)
            {
                minForSanityCheck = sanityChecks_.TMin[currentIndex];
                maxForSanityCheck = sanityChecks_.TMax[currentIndex];
            }
            else if(sanityCheckType == SanityChecks::FINAL)
            {
                minForSanityCheck = sanityChecks_.TMin[currentIndex];
                maxForSanityCheck = sanityChecks_.TMax[currentIndex];
            }
        }

        if(sanityCheckType == SanityChecks::RAW)
        {
            if(isnan(currentValue))
            {
                currentFileStats_.columnFailedSanityCheckRaw[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckRaw = true;
            }
            else if(!(double_equals(minForSanityCheck, sanityChecks_.IGNORE_MIN)) && (currentValue < minForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckRaw[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckRaw = true;
            }
            else if(!(double_equals(maxForSanityCheck, sanityChecks_.IGNORE_MAX)) && (currentValue > maxForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckRaw[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckRaw = true;
            }
        }
        else if(sanityCheckType == SanityChecks::FINAL)
        {
            if(isnan(currentValue))
            {
                currentFileStats_.columnFailedSanityCheckFinal[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckFinal = true;
            }
            else if(!(double_equals(minForSanityCheck, sanityChecks_.IGNORE_MIN)) && (currentValue < minForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckFinal[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckFinal = true;
            }
            else if(!(double_equals(maxForSanityCheck, sanityChecks_.IGNORE_MAX)) && (currentValue > maxForSanityCheck))
            {
                currentFileStats_.columnFailedSanityCheckFinal[currentIndex] = true;
                currentFileStats_.isFailedSanityCheckFinal = true;
            }
        }
    }
}

void LoggerDataReader::StoreInputFileContentsToRAM(ifstream& inFile)
{
    // Get size of file in bytes
    inFile.seekg(0, ios::end);
    unsigned int fileSizeInBytes = (unsigned int)inFile.tellg();
    inFile.seekg(0, ios::beg);

    inputFileContents_.reserve(fileSizeInBytes);

    outDataLines_.reserve(fileSizeInBytes * 2);
    if(printRaw_)
    {
        rawOutDataLines_.reserve(fileSizeInBytes * 2);
    }
    inputFileContents_.assign(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>());
}

void LoggerDataReader::ReadConfig()
{
    // Check for config file's existence
    std::ifstream configFile(configFilePath_, std::ios::in | std::ios::binary);

    string line = "";
    numErrors_ = 0;

    status_.configFileLineNumber = 0;

    if(!configFile)
    {
        configFile.close();
        logFileLines_ += "Error: config file not found or cannot be opened at path " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
    }
    else
    {
        if(CheckConfigFileFormatIsValid(configFile))
        {
            while(getline(configFile, line))
            {
                ParseTokensFromLineOfConfigFile(line);
                ProcessLineOfConfigFile();
            }
        }
    }

    if(numErrors_ == 0)
    {
        isConfigFileValid_ = true;
    }
}

void LoggerDataReader::ParseTokensFromLineOfConfigFile(string& line)
{
    enum
    {
        SERIAL_NUMBER = 0,
        CONFIGURATION = 1,
        SENSOR_NUMBER = 2,
        SENSOR_BEARING = 3,
        HEAT_FLUX_X_VOLTAGE_OFFSET = 4,
        HEAT_FLUX_Y_VOLTAGE_OFFSET = 5,
        HEAT_FLUX_Z_VOLTAGE_OFFSET = 6
    };

    int tokenCounter = 0;

    const int numRequiredColumns = 7;

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

    while(getline(lineStream_, token, ','))
    {
        int testSerial = atoi(token.c_str());

        // remove any whitespace or control chars from line
        token.erase(std::remove(token.begin(), token.end(), ' '), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\r'), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\r\n'), token.end());
        token.erase(std::remove(token.begin(), token.end(), '\t'), token.end());

        switch(tokenCounter)
        {
            case SERIAL_NUMBER:
            {
                configFileLine_.serialNumberString = token;
                if(IsOnlyDigits(configFileLine_.serialNumberString))
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
                if(configFileLine_.conifgurationString == "F" ||
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
                if(IsOnlyDigits(configFileLine_.sensorNumberString))
                {
                    status_.isSensorNumberValid = true;
                }
                else if(configFileLine_.conifgurationString == "T" &&
                    configFileLine_.sensorNumberString == "")
                {
                    status_.isSensorNumberValid = true;
                }
                else if(configFileLine_.conifgurationString == "H" &&
                    configFileLine_.sensorNumberString == "")
                {
                    status_.isSensorNumberValid = true;
                }
                else if(configFileLine_.conifgurationString == "")
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
                if(configFileLine_.conifgurationString == "H")
                {
                    if(*end != '\0' ||  // error, we didn't consume the entire string
                        errno != 0)   // error, overflow or underflow
                    {
                        status_.isSensorBearingValid = false;
                    }
                    else if(configFileLine_.sensorBearingValue < 0 || configFileLine_.sensorBearingValue > 360)
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
            case HEAT_FLUX_X_VOLTAGE_OFFSET:
            {
                configFileLine_.heatFlux_X_VoltageOffsetString = token;

                if(configFileLine_.conifgurationString == "H")
                {
                    char* end;
                    errno = 0;
                    long double ld;
                    if((std::istringstream(configFileLine_.heatFlux_X_VoltageOffsetString) >> ld >> std::ws).eof()) // Check if string is valide double
                    {
                        configFileLine_.heatFlux_X_VoltageOffsetValue = std::strtod(token.c_str(), &end);
                        if(configFileLine_.conifgurationString == "H")
                        {
                            if(*end != '\0' ||  // error, we didn't consume the entire string
                                errno != 0)   // error, overflow or underflow
                            {
                                status_.isHeatFlux_X_VoltageOffsetValid = false;
                            }
                            else if(configFileLine_.heatFlux_X_VoltageOffsetValue < -10.0 || configFileLine_.heatFlux_X_VoltageOffsetValue > 10.0) // Need to find real acceptable voltage bounds
                            {
                                status_.isHeatFlux_X_VoltageOffsetValid = false;
                            }
                            else
                            {
                                status_.isHeatFlux_X_VoltageOffsetValid = true;
                            }
                        }
                        else
                        {
                            status_.isHeatFlux_X_VoltageOffsetValid = true;
                        }
                    }
                }
                else
                {
                    status_.isHeatFlux_X_VoltageOffsetValid = true;
                }
                break;
            }
            case HEAT_FLUX_Y_VOLTAGE_OFFSET:
            {
                configFileLine_.heatFlux_Y_VoltageOffsetString = token;

                if(configFileLine_.conifgurationString == "H")
                {
                    char* end;
                    errno = 0;
                    long double ld;
                    if((std::istringstream(configFileLine_.heatFlux_Y_VoltageOffsetString) >> ld >> std::ws).eof()) // Check if string is valide double
                    {
                        configFileLine_.heatFlux_Y_VoltageOffsetValue = std::strtod(token.c_str(), &end);
                        if(configFileLine_.conifgurationString == "H")
                        {
                            if(*end != '\0' ||  // error, we didn't consume the entire string
                                errno != 0)   // error, overflow or underflow
                            {
                                status_.isHeatFlux_Y_VoltageOffsetValid = false;
                            }
                            else if(configFileLine_.heatFlux_Y_VoltageOffsetValue < -10.0 || configFileLine_.heatFlux_Y_VoltageOffsetValue > 10.0) // Need to find real acceptable voltage bounds
                            {
                                status_.isHeatFlux_Y_VoltageOffsetValid = false;
                            }
                            else
                            {
                                status_.isHeatFlux_Y_VoltageOffsetValid = true;
                            }
                        }
                        else
                        {
                            status_.isHeatFlux_Y_VoltageOffsetValid = true;
                        }
                    }
                }
                else
                {
                    status_.isHeatFlux_Y_VoltageOffsetValid = true;
                }
                break;
            }
            case HEAT_FLUX_Z_VOLTAGE_OFFSET:
            {
                configFileLine_.heatFlux_Z_VoltageOffsetString = token;

                if(configFileLine_.conifgurationString == "H")
                {
                    char* end;
                    errno = 0;
                    long double ld;
                    if((std::istringstream(configFileLine_.heatFlux_Z_VoltageOffsetString) >> ld >> std::ws).eof()) // Check if string is valide double
                    {
                        configFileLine_.heatFlux_Z_VoltageOffsetValue = std::strtod(token.c_str(), &end);
                        if(configFileLine_.conifgurationString == "H")
                        {
                            if(*end != '\0' ||  // error, we didn't consume the entire string
                                errno != 0)   // error, overflow or underflow
                            {
                                status_.isHeatFlux_Z_VoltageOffsetValid = false;
                            }
                            else if(configFileLine_.heatFlux_Z_VoltageOffsetValue < -10.0 || configFileLine_.heatFlux_Z_VoltageOffsetValue > 10.0) // Need to find real acceptable voltage bounds
                            {
                                status_.isHeatFlux_Z_VoltageOffsetValid = false;
                            }
                            else
                            {
                                status_.isHeatFlux_Z_VoltageOffsetValid = true;
                            }
                        }
                        else
                        {
                            status_.isHeatFlux_Z_VoltageOffsetValid = true;
                        }
                    }
                    else if(configFileLine_.conifgurationString != "H")
                    {
                        status_.isHeatFlux_Z_VoltageOffsetValid = false;
                    }
                }
                else
                {
                    status_.isHeatFlux_Z_VoltageOffsetValid = true;
                }
                break;
            }
        }
        tokenCounter++;
    }

    if(tokenCounter < numRequiredColumns)
    {
        logFileLines_ += "Error: The entry in config file at " + configFilePath_ + " has less than the required " + std::to_string(numRequiredColumns) + " columns at line " + std::to_string(status_.configFileLineNumber) + " " + GetMyLocalDateTimeString() + "\n";
        numErrors_++;
        isConfigFileValid_ = false;
    }
    if(tokenCounter > numRequiredColumns)
    {
        logFileLines_ += "Error: The entry in config file at " + configFilePath_ + " has more than the required " + std::to_string(numRequiredColumns) + " columns at line " + std::to_string(status_.configFileLineNumber) + " " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
}

bool LoggerDataReader::CheckConfigFileFormatIsValid(ifstream& configFile)
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
    if((token != "First") && (token != "1st"))
    {
        logFileLines_ += "Error: Config file at " + configFilePath_ + " is improperly formatted " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
    else
    {
        isConfigFileValid_ = true;
    }

    return isConfigFileValid_;
}

void LoggerDataReader::ProcessLineOfConfigFile()
{
    if(status_.isSerialNumberValid && status_.isConfigurationTypeValid)
    {
        int serialNumber = atoi(configFileLine_.serialNumberString.c_str());
        int testCount = (int)configMap_.count(serialNumber);
        if(configMap_.count(serialNumber) > 0)
        {
            logFileLines_ += "Error: More than one entry exists for logger id " + configFileLine_.serialNumberString +
                " with repeated entry in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
            isConfigFileValid_ = false;
            numErrors_++;
        }
        else
        {
            // Check Configuration
            if(configFileLine_.conifgurationString == "")
            {
                if(configFileLine_.sensorNumberString != "")
                {
                    if(status_.isSerialNumberValid)
                    {
                        logFileLines_ += "Error: " + configFileLine_.conifgurationString + " is invalid configuration for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
                            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                    else
                    {
                        logFileLines_ += "Error: Invalid configuration (second column) in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                }
            }
            // Check Sensor Number (Package ID)
            if(!status_.isSensorNumberValid)
            {
                logFileLines_ += "Error: Entry " + configFileLine_.sensorNumberString + " is invalid for sensor number (third column) for logger id " + configFileLine_.serialNumberString +
                    " in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                isConfigFileValid_ = false;
                numErrors_++;
            }
            if(status_.isSerialNumberValid && status_.isConfigurationTypeValid)
            {
                configMap_.insert(pair<int, string>(serialNumber, configFileLine_.conifgurationString));

                // Check Heat Flux Package Configuration
                if(configFileLine_.conifgurationString == "H")
                {
                    // Check Sensor Bearing
                    if(status_.isSensorBearingValid)
                    {
                        sensorBearingMap_.insert(pair<int, double>(serialNumber, configFileLine_.sensorBearingValue));
                    }
                    else
                    {
                        logFileLines_ += "Error: " + configFileLine_.sensorBearingString + " is invalid for sensor bearing (fourth column) for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
                            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                    // Check X Voltage Offset
                    if(status_.isHeatFlux_X_VoltageOffsetValid)
                    {
                        heatFlux_X_VoltageOffsetMap_.insert(pair<int, double>(serialNumber, configFileLine_.heatFlux_X_VoltageOffsetValue));

                    }
                    else
                    {
                        logFileLines_ += "Error: Entry " + configFileLine_.heatFlux_X_VoltageOffsetString + " is invalid for heat flux X voltage offset (fifth column) for logger id " + configFileLine_.serialNumberString +
                            " in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                    // Check Y Voltage Offset
                    if(status_.isHeatFlux_Y_VoltageOffsetValid)
                    {
                        heatFlux_Y_VoltageOffsetMap_.insert(pair<int, double>(serialNumber, configFileLine_.heatFlux_Y_VoltageOffsetValue));
                        sensorBearingMap_.insert(pair<int, double>(serialNumber, configFileLine_.sensorBearingValue));
                    }
                    else if(configFileLine_.conifgurationString == "H" && !status_.isHeatFlux_Y_VoltageOffsetValid)
                    {
                        logFileLines_ += "Error: Entry " + configFileLine_.heatFlux_Y_VoltageOffsetString + " is invalid for heat flux Y voltage offset (sixth column) for logger id " + configFileLine_.serialNumberString +
                            " in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                    // Check Z Voltage Offset
                    if(status_.isHeatFlux_Z_VoltageOffsetValid)
                    {
                        heatFlux_Z_VoltageOffsetMap_.insert(pair<int, double>(serialNumber, configFileLine_.heatFlux_Z_VoltageOffsetValue));
                        sensorBearingMap_.insert(pair<int, double>(serialNumber, configFileLine_.sensorBearingValue));
                    }
                    else if(configFileLine_.conifgurationString == "H" && !status_.isHeatFlux_Z_VoltageOffsetValid)
                    {
                        logFileLines_ += "Error: Entry " + configFileLine_.heatFlux_Z_VoltageOffsetString + " is invalid for heat flux Z voltage offset (seventh column) for logger id " + configFileLine_.serialNumberString +
                            " in line " + std::to_string(status_.configFileLineNumber) + " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
                        isConfigFileValid_ = false;
                        numErrors_++;
                    }
                }
            }

        }
    }
    else if(!status_.isSerialNumberValid)
    {
        if(configFileLine_.serialNumberString != "")
        {
            logFileLines_ += "Error: " + configFileLine_.serialNumberString + " is an invalid serial number(first column) entry in line " + std::to_string(status_.configFileLineNumber) + " in config file at " +
                configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        }
        else
        {
            logFileLines_ += "Error: No entry for serial number (first column) in line number " + std::to_string(status_.configFileLineNumber) + " in config file at " +
                configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        }
        isConfigFileValid_ = false;
        numErrors_++;
    }
    else if(!status_.isConfigurationTypeValid)
    {
        logFileLines_ += "Error: " + configFileLine_.conifgurationString + " is an invalid configuration(second column) for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
    else if(configFileLine_.conifgurationString == "H" && !status_.isSensorBearingValid)
    {
        logFileLines_ += "Error: " + configFileLine_.sensorBearingString + " is invalid for sensor bearing (fourth column) for logger id " + configFileLine_.serialNumberString + " in line number " + std::to_string(status_.configFileLineNumber) +
            " in config file at " + configFilePath_ + " " + GetMyLocalDateTimeString() + "\n";
        isConfigFileValid_ = false;
        numErrors_++;
    }
}

void LoggerDataReader::CheckConfigForAllFiles()
{
    int codepage = 0;
    numInvalidInputFiles_ = 0;

    for(unsigned int i = 0; i < inputFilePathList_.size(); i++)
    {
        inDataFilePath_ = inputFilePathList_[i];

        std::ifstream inFile(inDataFilePath_, std::ios::in | std::ios::binary);
        // Check for input file's existence
        if(inFile.fail())
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
            if(inputFileContents_.size() > 0)
            {
                ResetInFileReadingStatus();
                currentFileStats_.fileName = inputFilePathList_[i];
                bool headerFound = GetFirstHeader();

                if(headerFound)
                {
                    int serialNumber = atoi(headerData_.serialNumberString.c_str());
                    if((configMap_.find(serialNumber) == configMap_.end()) || (configMap_.find(serialNumber)->second == ""))
                    {
                        // Config missing for file
                        numInvalidInputFiles_++;
                        invalidInputFileList_.push_back(inDataFilePath_);
                        invalidInputFileErrorType_.push_back(InvalidInputFileErrorType::MISSING_CONFIG);
                        inFile.close();
                    }
                }
                else if(!headerFound)
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

    if(numInvalidInputFiles_)
    {
        PrintConfigErrorsToLog();
    }
}

void LoggerDataReader::ResetHeaderData()
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

void LoggerDataReader::ResetInFileReadingStatus()
{
    status_.sensorReadingCounter = 0;
    status_.position = 0;
    status_.recordNumber = 0;
    status_.configColumnTextLine = "";
    status_.loggingSession = 0;

    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        status_.columnRawType[i] = "";
        status_.sensorReadingValue[i] = 0.0;
    }
    status_.headerFound = false;
    status_.isCarryBugEncountered_ = false;
    status_.isDataCorruptionEncountered_ = false;
    status_.isRecoveredAfterDataCorruption_ = true;
}

void LoggerDataReader::ResetCurrentFileStats()
{
    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        currentFileStats_.columnFailedSanityCheckRaw[i] = false;
        currentFileStats_.columnFailedSanityCheckFinal[i] = false;
        currentFileStats_.columnMin[i] = SanityChecks::INITIAL_MIN;
        currentFileStats_.columnMax[i] = SanityChecks::INITIAL_MAX;
    }
    currentFileStats_.isFailedSanityCheckFinal = false;
    currentFileStats_.isFailedSanityCheckRaw = false;
}

void LoggerDataReader::SetConfigDependentValues()
{
    if(configurationType_ == "F")
    {
        status_.configColumnTextLine = "\"DATETIME\",\"TIMESTAMP\",\"RECORD\",\"C\",\"FID(V)\",\"P(Pa)\",\"NA\",\"NA\",\"NA\",\"NA\",\"NA\",\"Panel Temp (C)\"\n";
        status_.rawConfigColumnTextLine = "\"DATETIME\",\"TIMESTAMP\",\"RECORD\",\"Temp(V)\",\"FID(V)\",\"P(V)\",\"NA\",\"NA\",\"NA\",\"NA\",\"NA\",\"Panel Temp (V)\"\n";
        status_.columnRawType[0] = "V";
        status_.columnRawType[1] = "FID(V)";
        status_.columnRawType[2] = "P(V)";
        for(int i = 3; i < 8; i++)
        {
            status_.columnRawType[i] = "NA";
        }
    }
    else if(configurationType_ == "H")
    {
        status_.configColumnTextLine = "\"DATETIME\",\"TIMESTAMP\",\"RECORD\",\"C\",\"C\",\"u(m/s)\",\"v(m/s)\",\"w(m/s)\",\"HF(kW/m^2)\",\"HFT(C)\",\"NA\",\"Panel Temp (C)\"\n";
        status_.rawConfigColumnTextLine = "\"DATETIME\",\"TIMESTAMP\",\"RECORD\",\"Temp(V)\",\"Temp(V)\",\"u(V)\",\"v(V)\",\"w(V)\",\"HF(V)\",\"HFT(V)\",\"NA\",\"Panel Temp (V)\"\n";
        for(int i = 0; i < 2; i++)
        {
            status_.columnRawType[i] = "C";
        }
        for(int i = 2; i < 5; i++)
        {
            status_.columnRawType[i] = "P(V)";
        }
        status_.columnRawType[5] = "HF(V)";
        status_.columnRawType[6] = "HFT(V)";
        status_.columnRawType[7] = "NA";
    }
    else if(configurationType_ == "T")
    {
        status_.configColumnTextLine = "\"DATETIME\",\"TIMESTAMP\",\"RECORD\",\"C\",\"C\",\"C\",\"C\",\"C\",\"C\",\"C\",\"C\",\"Panel Temp (C)\"\n";
        status_.rawConfigColumnTextLine = "\"DATETIME\",\"TIMESTAMP\",\"RECORD\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Temp(V)\",\"Panel Temp (Temp(V))\"\n";
        for(int i = 0; i < 8; i++)
        {
            status_.columnRawType[i] = "V";
        }
    }
    // Common among all configurations
    status_.columnRawType[8] = "Panel Temp (V)";
}

//void LoggerDataReader::PrintStatsFile()
//{
//    statsFilePath_ = dataPath_ + "sensor_stats.csv";
//    ofstream outStatsFile(statsFilePath_, std::ios::out);
//    // Check for sensor stats file's existence
//    if (!outStatsFile || outStatsFile.fail())
//    {
//        outStatsFile.close();
//        outStatsFile.clear();
//        SYSTEMTIME systemTime;
//        GetLocalTime(&systemTime);
//
//        string dateTimeString = MakeStringWidthTwoFromInt(systemTime.wDay) + "-" + MakeStringWidthTwoFromInt(systemTime.wMonth) + "-" + std::to_string(systemTime.wYear) +
//            "_" + MakeStringWidthTwoFromInt(systemTime.wHour) + "H" + MakeStringWidthTwoFromInt(systemTime.wMinute) + "M" +
//            MakeStringWidthTwoFromInt(systemTime.wSecond) + "S";
//        string statsFileNoExtension = dataPath_ + "sensor_stats";
//        logFileLine_ += "ERROR: default log file at " + logFilePath_ + " already opened or otherwise unwritable\n";
//        statsFilePath_ = statsFileNoExtension + "_" + dateTimeString + ".csv";
//
//        outStatsFile.open(statsFilePath_, std::ios::out);
//    }
//
//    outputsAreGood_ = (numInvalidInputFiles_ == 0) && outStatsFile.good(); // && logFile_.good();
//
//    if (outStatsFile.good())
//    {
//        string outputLine = "";
//        string fileName = "";
//        for (auto it = statsFileMap_.begin(); it != statsFileMap_.end(); it++)
//        {
//            if (it->second.isFailedSanityCheckRaw)
//            {
//                int numFailures = 0;
//                int lastFailure = 0;
//                fileName = it->second.fileName;
//                outputLine += fileName + ": Raw data has failed sanity check in column(s) ";
//                for (int i = 0; i < NUM_SENSOR_READINGS; i++)
//                {
//                    if (it->second.columnFailedSanityCheckRaw[i])
//                    {
//                        numFailures++;
//                        lastFailure = i;
//                    }
//                }
//                for (int i = 0; i < NUM_SENSOR_READINGS; i++)
//                {
//                    if (it->second.columnFailedSanityCheckRaw[i])
//                    {
//                        if ((numFailures > 1) && (i < lastFailure))
//                        {
//                            outputLine += to_string(i + 1) + " ";
//                        }
//                        else if ((numFailures > 1) && (i == lastFailure))
//                        {
//                            outputLine += "and " + to_string(i + 1) + "\n";
//                        }
//                        else
//                        {
//                            outputLine += to_string(i + 1) + "\n";
//                        }
//                    }
//                }
//            }
//            if (it->second.isFailedSanityCheckFinal)
//            {
//                int numFailures = 0;
//                int lastFailure = 0;
//                outputLine += "Converted data in file " + it->second.fileName + " has failed sanity check in column(s) ";
//                for (int i = 0; i < NUM_SENSOR_READINGS; i++)
//                {
//                    if (it->second.columnFailedSanityCheckFinal[i])
//                    {
//                        numFailures++;
//                        lastFailure = i;
//                    }
//                }
//                for (int i = 0; i < NUM_SENSOR_READINGS; i++)
//                {
//                    if (it->second.columnFailedSanityCheckFinal[i])
//                    {
//                        if ((numFailures > 1) && (i < lastFailure))
//                        {
//                            outputLine += to_string(i + 1) + " ";
//                        }
//                        else if ((numFailures > 1) && (i == lastFailure))
//                        {
//                            outputLine += "and " + to_string(i + 1) + "\n";
//                        }
//                        else
//                        {
//                            outputLine += to_string(i + 1) + "\n";
//                        }
//                    }
//                }
//            }
//        }
//
//        if (outputLine != "")
//        {
//            outputLine += "\n";
//            outStatsFile << outputLine;
//        }
//
//        outputLine = "";
//        outputLine += "Full Sensor Min/Max Stats For All Files:\n";
//        outputLine += "File Name,Col 1 Min,Col 1 Max,Col 1 Type,";
//        outputLine += "Col 2 Min,Col 2 Max,Col 2 Type,";
//        outputLine += "Col 3 Min,Col 3 Max,Col 3 Type,";
//        outputLine += "Col 3 Min,Col 4 Max,Col 4 Type,";
//        outputLine += "Col 3 Min,Col 5 Max,Col 5 Type,";
//        outputLine += "Col 6 Min,Col 6 Max,Col 6 Type,";
//        outputLine += "Col 7 Min,Col 7 Max,Col 7 Type,";
//        outputLine += "Col 8 Min,Col 8 Max,Col 8 Type,";
//        outputLine += "Panel Temp Min,Panel Temp Max,Panel Temp Type\n";
//        outStatsFile << outputLine;
//
//        outputLine = "";
//        for (auto it = statsFileMap_.begin(); it != statsFileMap_.end(); it++)
//        {
//            outputLine = it->second.fileName + ",";
//
//            for (int i = 0; i < NUM_SENSOR_READINGS; i++)
//            {
//                outputLine += std::to_string(it->second.columnMin[i]) + "," + std::to_string(it->second.columnMax[i]) + "," + status_.columnRawType[i];
//                if (i < 8)
//                {
//                    outputLine += ",";
//                }
//                else
//                {
//                    outputLine += "\n";
//                }
//            }
//
//            outStatsFile << outputLine;
//        }
//    }
//    outStatsFile.close();
//}

void LoggerDataReader::UpdateStatsFileMap()
{
    statsFileMap_.insert(pair<int, StatsFileData>(serialNumber_, currentFileStats_));
}

void LoggerDataReader::StoreSessionStartTime()
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

void LoggerDataReader::StoreSessionEndTime()
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

bool LoggerDataReader::CreateWindTunnelDataVectors()
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
    if(windTunnelDataFile.good())
    {
        pressureCoefficients_ = make_2d_vector<double>(NUM_ANGLES, NUM_REYNOLDS_NUMBERS);
        for(int ReynoldsNumberIndex = 0; ReynoldsNumberIndex < NUM_REYNOLDS_NUMBERS; ReynoldsNumberIndex++)
        {
            for(int angleIndex = 0; angleIndex < NUM_ANGLES; angleIndex++)
            {
                getline(windTunnelDataFile, line);
                lineCount++;
                std::istringstream iss(line);
                iss >> angle >> ReynoldsNumber >> pressureCoefficient;

                if(angles_.size() < NUM_ANGLES)
                {
                    angles_.push_back(angle);
                }

                if(angleIndex == 0)
                {
                    ReynloldsNumbers_.push_back(ReynoldsNumber);
                }
                pressureCoefficients_[angleIndex][ReynoldsNumberIndex] = pressureCoefficient;
            }
        }
        if(lineCount == NUM_DATA_LINES)
        {
            isSuccessful = true;
        }
    }

    return isSuccessful;
}

void LoggerDataReader::DegreesDecimalMinutesToDecimalDegrees(HeaderData& headerData)
{
    // latitude
    double inputLatitudeDegrees = atof(headerData.latitudeDegreesString.c_str());
    double inputLatitudeDecimalMinutes = atof(headerData.latitudeDecimalMinutesString.c_str());
    headerData.decimalDegreesLatitude = inputLatitudeDegrees + (inputLatitudeDecimalMinutes / 60.0);
    if(headerData.northOrSouthHemisphere == "S")
    {
        headerData.decimalDegreesLatitude *= -1.0;
    }

    // longitude
    double inputLongitudeDegrees = atof(headerData.longitudeDegreesString.c_str());
    double inputLongitudeDecimalMinutes = atof(headerData.longitudeDecimalMinutesString.c_str());
    headerData.decimalDegreesLongitude = inputLongitudeDegrees + (inputLongitudeDecimalMinutes / 60.0);
    if(headerData.eastOrWestHemishere == "W")
    {
        headerData.decimalDegreesLongitude *= -1.0;
    }
}

void LoggerDataReader::HandleCorruptData()
{
    int numCorruptReadings = 0;
    int numCorruptRecords = 0;
    //int fistCorruptByte = fistCorruptByte = status_.position;;
    status_.isDataCorruptionEncountered_ = true;

    bool isGoodChannelRead = false;
    int posRecoveredReading = -1;
    int previousChannelNumber = -1;

    int headerPosition;

    status_.position -= BYTES_READ_PER_ITERATION;
    while(!isGoodChannelRead && (status_.position < (status_.filePositionLimit - BYTES_READ_PER_ITERATION)))
    {
        // Read in 4 bytes, then check if those 4 bytes contain the header signature
        for(int i = 0; i < 4; i++)
        {
            parsedNumericData_.rawHexNumber[i] = (uint8_t)inputFileContents_[status_.position];
            status_.position++;
        }

        CheckForHeader();

        for(uint8_t i = 0; i < NUM_SENSOR_READINGS; i++)
        {
            status_.isChannelPresent[i] = false;
        }

        if(status_.firstHeaderFound && (!status_.headerFound))
        {
            status_.position -= 4; // move back reading frame by four bytes, check for channel number
            parsedNumericData_.channelNumber = (uint8_t)inputFileContents_[status_.position];

            if((parsedNumericData_.channelNumber >= 0) && (parsedNumericData_.channelNumber < NUM_SENSOR_READINGS))
            {
                // We have a valid channel number, check that next channel 8 numbers are also valid
                posRecoveredReading = status_.position;

                status_.isChannelPresent[parsedNumericData_.channelNumber] = true;

                for(int i = 0; i < NUM_SENSOR_READINGS; i++)
                {
                    if(status_.position < (status_.filePositionLimit - BYTES_READ_PER_ITERATION))
                    {
                        status_.position += BYTES_READ_PER_ITERATION; // move reading frame to next channel number
                        parsedNumericData_.channelNumber = (uint8_t)inputFileContents_[status_.position];
                    }
                    else
                    {
                        status_.position = status_.filePositionLimit;
                        break;
                    }

                    // Check that channel numbers appear exactly once and in the correct order
                    if((parsedNumericData_.channelNumber >= 0) &&
                        (parsedNumericData_.channelNumber < NUM_SENSOR_READINGS) &&
                        (status_.isChannelPresent[parsedNumericData_.channelNumber] == false))
                    {
                        if((parsedNumericData_.channelNumber == 0) && (status_.isChannelPresent[9] == true))
                        {
                            status_.isChannelPresent[parsedNumericData_.channelNumber] = true;
                        }
                        else if((status_.isChannelPresent[parsedNumericData_.channelNumber - 1] == true))
                        {
                            status_.isChannelPresent[parsedNumericData_.channelNumber] = true;
                        }
                        else
                        {
                            for(uint8_t i = 0; i < NUM_SENSOR_READINGS; i++)
                            {
                                status_.isChannelPresent[i] = false;
                            }
                            break;
                        }
                    }
                    else
                    {
                        for(uint8_t i = 0; i < NUM_SENSOR_READINGS; i++)
                        {
                            status_.isChannelPresent[i] = false;
                        }
                        break;
                    }
                }
            }

            if(status_.isChannelPresent[0] &&
                status_.isChannelPresent[1] &&
                status_.isChannelPresent[2] &&
                status_.isChannelPresent[3] &&
                status_.isChannelPresent[4] &&
                status_.isChannelPresent[5] &&
                status_.isChannelPresent[6] &&
                status_.isChannelPresent[7] &&
                status_.isChannelPresent[8] &&
                status_.isChannelPresent[9])
            {
                isGoodChannelRead = true;
                status_.position = posRecoveredReading;
                parsedNumericData_.channelNumber = (uint8_t)inputFileContents_[status_.position];

                status_.position -= NUM_DATA_BYTES;

                for(int i = 0; i < NUM_DATA_BYTES; i++)
                {
                    parsedNumericData_.rawHexNumber[i] = (uint8_t)inputFileContents_[status_.position];
                    status_.position++;
                }

                for(int i = 0; i < NUM_SENSOR_READINGS; i++)
                {
                    status_.isChannelPresent[i] = false;
                }

                status_.isChannelPresent[parsedNumericData_.channelNumber] = true;

                status_.sensorReadingCounter = 0;
                break;
            }
            else
            {
                for(uint8_t i = 0; i < NUM_SENSOR_READINGS; i++)
                {
                    status_.isChannelPresent[i] = false;
                }
            }
        }
        else
        {
            // Found the header
            break;
        }

        if(status_.position < status_.filePositionLimit)
        {
            status_.position++; // advance reading frame one byte forward
        }
        else
        {
            status_.position = status_.filePositionLimit;
            break;
        }

        numCorruptReadings++;
    }

    numCorruptRecords = ceil(numCorruptReadings / (NUM_SENSOR_READINGS * 1.0));

    // Ensure at least one corrupt record if this method is called
    if(numCorruptRecords < 1)
    {
        numCorruptRecords = 1;
    }

    for(int i = 0; i < numCorruptRecords; i++)
    {
        status_.corruptedRecordNumbers.push_back(status_.recordNumber + 1); // record number is zero indexed in program
        status_.corruptedSessionNumbers.push_back(status_.loggingSession);

        // An entire row of sensor data has been read in
        status_.recordNumber++;

        PerformSanityChecksOnValues(SanityChecks::RAW);
        if(printRaw_)
        {
            PrintSensorDataOutput(OutFileType::RAW);
        }

        for(int i = 0; i < NUM_SENSOR_READINGS; i++)
        {
            status_.sensorReadingValue[i] = -8888.888888; // indicates corrupted row data
        }

        PerformNeededDataConversions();
        PerformSanityChecksOnValues(SanityChecks::FINAL);
        UpdateStatsFileMap();
        PrintSensorDataOutput(OutFileType::PROCESSED);
        // Reset sensor reading counter 
        status_.sensorReadingCounter = 0;
        // Get (possible) session end time
        StoreSessionEndTime();
        // Update time for next set of sensor readings
        headerData_.milliseconds += headerData_.sampleInterval;
        UpdateTime();
    }

    if(status_.firstHeaderFound && (!status_.headerFound) && (status_.position < status_.filePositionLimit))
    {
        status_.position++;
    }

    if(status_.position > status_.filePositionLimit)
    {
        status_.position = status_.filePositionLimit;
    }
}

//void LoggerDataReader::BeginKMLFile()
//{
//    kmlFile_ << "<?xml version='1.0' encoding='utf-8'?>\n";
//    kmlFile_ << "<kml xmlns='http://www.opengis.net/kml/2.2'>\n";
//    kmlFile_ << "<Document>\n";
//}

//string LoggerDataReader::FormatPlacemark()
//{
//    std::ostringstream ss;
//
//    string iconUrl = "";
//    string iconColor = "";
//
//    int serial = atoi(headerData_.serialNumberString.c_str());
//    string serialString = to_string(serial);
//    string sessionNumberString = to_string(status_.loggingSession);
//    string placemarkName = serialString + "_s" + sessionNumberString;
//    string description = "test";
//
//    if (configurationType_ == "F")
//    {
//        iconUrl = iconsUrls_.fid;
//        iconColor = "ff00ff00"; // Green Icon
//    }
//    else if (configurationType_ == "H")
//    {
//        iconUrl = iconsUrls_.heatFlux;
//        iconColor = "ff0000ff"; // Red Icon
//    }
//    else if (configurationType_ == "T")
//    {
//        iconUrl = iconsUrls_.temperature;
//        iconColor = "ff00ffff"; // Code for Yellow, produces Orange as icon is Red 
//    }
//
//    ss << setprecision(10) << fixed;
//    ss << "<Placemark>\n"
//        << "<name>" << placemarkName << "</name>\n"
//        << "<description>" 
//        << "Logger Serial: SN" << headerData_.serialNumberString << "\n"
//        << "Longitude (DD): " << headerData_.decimalDegreesLongitude << "\n"
//        << "Latitude (DD):  " << headerData_.decimalDegreesLatitude << "\n"
//        << "Logging Session Number: " << sessionNumberString << "\n"
//        << "Start Time (GMT): " << startTimeForSession_.yearString << "-" << startTimeForSession_.monthString 
//        << "-" << startTimeForSession_.dayString <<" " << startTimeForSession_.hourString << ":" << startTimeForSession_.minuteString 
//        << ":" << startTimeForSession_.secondString << "." << startTimeForSession_.millisecondString << "\n"
//        << "End Time (GMT):   " << endTimeForSession_.yearString << "-" + endTimeForSession_.monthString << "-" << endTimeForSession_.dayString << " "
//        << endTimeForSession_.hourString << ":" << endTimeForSession_.minuteString << ":" << endTimeForSession_.secondString << "."
//        << endTimeForSession_.millisecondString << "\n"
//        << "</description>\n"
//        << "<Point>\n"
//        << "<coordinates>"
//        << headerData_.decimalDegreesLongitude << "," << headerData_.decimalDegreesLatitude << ",0"
//        << "</coordinates>\n"
//        << "</Point>\n"
//        << "<Style>"
//        << "<IconStyle>"
//        << "<color>" << iconColor << "</color>"
//        << "<Icon>"
//        << "<href>" << iconUrl << "</href>"
//        << "</Icon> "
//        << "</IconStyle>"
//        << "</Style>"
//        << "</Placemark>\n";
//
//    return ss.str();
//}

//void LoggerDataReader::EndKMLFile()
//{
//    kmlFile_ << "</Document>\n";
//    kmlFile_ << "</kml>\n";
//}

void LoggerDataReader::SetPrintRaw(bool option)
{
    printRaw_ = option;
}

void LoggerDataReader::SetDataPath(string dataPath)
{
    if(dataPath.length() > 0)
    {
        char lastChar = dataPath[dataPath.length() - 1];
        char testChar('\\');

        if(lastChar != testChar)
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

void LoggerDataReader::SetWindTunnelDataTablePath(string windTunnelDataTablePath)
{
    windTunnelDataPath_ = windTunnelDataTablePath;
}

void LoggerDataReader::SetAppPath(string appPath)
{
    appPath_ = appPath;
}

void LoggerDataReader::SetConfigFile(string configFileFullPath)
{
    configFilePath_ = configFileFullPath;
}

string LoggerDataReader::GetDataPath()
{
    return dataPath_;
}

string LoggerDataReader::GetStatsFilePath()
{
    return statsFilePath_;
}

unsigned int LoggerDataReader::GetNumberOfInputFiles()
{
    return (unsigned int)inputFilePathList_.size();
}

unsigned int LoggerDataReader::GetNumFilesProcessed()
{
    return numFilesProcessed_;
}

unsigned int LoggerDataReader::GetNumInvalidFiles()
{
    return numInvalidInputFiles_ + numInvalidOutputFiles_;
}

unsigned int LoggerDataReader::GetNumErrors()
{
    return numErrors_;
}

bool LoggerDataReader::IsConfigFileValid()
{
    return isConfigFileValid_;
}

bool LoggerDataReader::IsDoneReadingDataFiles()
{
    return(currentFileIndex_ >= inputFilePathList_.size());
}

bool LoggerDataReader::GetFirstHeader()
{
    ResetHeaderData();

    status_.position = 0;
    status_.filePositionLimit = inputFileContents_.size();

    // Keep grabbing bytes until header is found or end of file is reached
    while(!status_.headerFound && (status_.position < status_.filePositionLimit))
    {
        GetRawNumber();
    }

    if(status_.headerFound)
    {
        status_.firstHeaderFound = true;
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

void LoggerDataReader::ReadNextHeaderOrNumber()
{
    status_.headerFound = false;

    GetRawNumber();
    CheckForHeader();
    if(!status_.headerFound)
    {
        //if (parsedNumericData_.channelNumber == 0)
        //{
        //    // Make panel temp's channel number 9, as it is printed last
        //    parsedNumericData_.channelNumber = 10;
        //}

        if((parsedNumericData_.channelNumber >= 0) && (parsedNumericData_.channelNumber <= 9)) // Prevents array out of bounds while reading a header 
        {
            parsedNumericData_.intFromBytes = GetIntFromByteArray(parsedNumericData_.rawHexNumber);
            parsedNumericData_.parsedIEEENumber = UnsignedIntToIEEEFloat(parsedNumericData_.intFromBytes);
            if(parsedNumericData_.channelNumber == 0)
            {
                status_.sensorReadingValue[0] = parsedNumericData_.intFromBytes;
            }
            else
            {
                status_.sensorReadingValue[parsedNumericData_.channelNumber] = parsedNumericData_.parsedIEEENumber;
            }
            // increment sensor reading for next iteration
            status_.sensorReadingCounter++;
        }
    }
}

uint32_t LoggerDataReader::GetIntFromByteArray(uint8_t byteArray[NUM_DATA_BYTES])
{
    uint32_t integerValue = int(
        byteArray[3] << 24 | // Set high byte (bits 24 to 31)
        byteArray[2] << 16 | // Set bits 16 to 23
        byteArray[1] << 8 | // Set bits 8 to 15
        byteArray[0]); // Set low byte (bits 0 to 7)

    return integerValue;
}

void LoggerDataReader::GetRawNumber()
{
    int index = 0;
    int numCorruptReadings = 0;
    int numCorruptRecords = 0;
    status_.headerFound = false;

    // Always read in 4 bytes, then check if those 4 bytes contain the header signature
    for(int i = 0; i < 4; i++)
    {
        if(status_.position < status_.filePositionLimit)
        {
            parsedNumericData_.rawHexNumber[i] = (uint8_t)inputFileContents_[status_.position];
            status_.position++;
        }
        else
        {
            status_.position = status_.filePositionLimit;
        }
    }

    CheckForHeader();

    // If it is not a header, read in 1 more byte for channel number
    if(!status_.headerFound && (status_.position < status_.filePositionLimit))
    {
        parsedNumericData_.channelNumber = inputFileContents_[status_.position];
        status_.position++;

        if(status_.firstHeaderFound)
        {
            if((parsedNumericData_.channelNumber >= 0) || (parsedNumericData_.channelNumber <= 8))
            {
                if(status_.isChannelPresent[parsedNumericData_.channelNumber] == true)
                {
                    // Indicates data is corrupted
                    HandleCorruptData();
                }
                else
                {
                    status_.isChannelPresent[parsedNumericData_.channelNumber] = true;
                }
            }
        }
    }
}

void LoggerDataReader::CheckForHeader()
{
    // Check for header signature "SNXX" where X is 0-9
    if((parsedNumericData_.rawHexNumber[0] == 'S') && (parsedNumericData_.rawHexNumber[1] == 'N') &&
        isdigit(parsedNumericData_.rawHexNumber[2]) &&
        isdigit(parsedNumericData_.rawHexNumber[3]))
    {
        status_.headerFound = true;
        // Clear previous session's millisecond  data 
        status_.previousMillisecondsValue = 0;
        status_.previousMillisecondsSinceSessionStart = 0;
    }
}

void LoggerDataReader::UpdateTime()
{
    uint32_t currentMillisecondsSinceSessionStart = status_.sensorReadingValue[0];
    headerData_.milliseconds = currentMillisecondsSinceSessionStart % 1000;

    uint32_t millisecondsSinceSessionStartDifference = status_.sensorReadingValue[0] - status_.previousMillisecondsSinceSessionStart;
    status_.previousMillisecondsSinceSessionStart = status_.sensorReadingValue[0];

    const int recordWriteDuration = 20; // time it takes to write a single complete recond in ms

    if(millisecondsSinceSessionStartDifference > recordWriteDuration)
    {
        headerData_.milliseconds = status_.previousMillisecondsValue;
        while(millisecondsSinceSessionStartDifference > 0)
        {
            headerData_.milliseconds++;
            if(headerData_.milliseconds >= 1000)
            {
                headerData_.milliseconds -= 1000;
                AdvanceTimeByOneSecond();
            }

            millisecondsSinceSessionStartDifference--;
        }
    }
    else if(headerData_.milliseconds == 0)
    {
        AdvanceTimeByOneSecond();
    }

    status_.previousMillisecondsValue = headerData_.milliseconds;
}

void LoggerDataReader::AdvanceTimeByOneSecond()
{
    // Zero is in first entry so index is calendar year month
    static const unsigned int monthLength[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    headerData_.seconds++;

    if(headerData_.seconds >= 60)
    {
        headerData_.seconds -= 60;
        headerData_.minutes++;
    }
    if(headerData_.minutes >= 60)
    {
        headerData_.minutes -= 60;
        headerData_.hours++;
    }
    if(headerData_.hours >= 24)
    {
        headerData_.hours -= 24;
        headerData_.day++;
    }
    // Handling leap years
    bool leap = (headerData_.year % 4 == 0) && (headerData_.year % 100 != 0 || headerData_.year % 400 == 0);
    if(headerData_.day > monthLength[headerData_.month])
    {
        if(!(leap && (headerData_.month == 2)))
        {
            // If it's not both february and a leap year
            // Then move on to the next month
            headerData_.day = 1;
            headerData_.month++;
        }
        else if(leap && (headerData_.month == 2) && (headerData_.day > 29))
        {
            // Else if it's a leap year and yesterday was February 29
            // Then move on to the next month
            headerData_.day = 1;
            headerData_.month++;
        }
    }
    if(headerData_.month > 12)
    {
        headerData_.year++;
        headerData_.month = 1;
    }
}

void LoggerDataReader::GetHeader()
{
    uint8_t byte = 0;
    ResetHeaderData();

    for(int i = 0; i < 4; i++)
    {
        headerData_.rawHeader[i] = parsedNumericData_.rawHexNumber[i];
    }
    unsigned int currentPos = status_.position;
    for(unsigned int i = currentPos; (i < currentPos + 44) && (i < status_.filePositionLimit); i++)
    {
        byte = inputFileContents_[i];
        headerData_.rawHeader += byte;
        // update reading position
        status_.position++;
    }

    if(status_.position < status_.filePositionLimit)
    {
        ParseHeader();
    }
}

void LoggerDataReader::ParseHeader()
{
    string temp;
    // Get the serial number
    for(int i = 2; i < 7; i++)
    {
        headerData_.serialNumberString[i - 2] = headerData_.rawHeader[i];
    }
    // Get the latitude
    headerData_.latitudeDegreesString = "00";
    headerData_.latitudeDecimalMinutesString = "00000000";
    for(int i = 7; i < 9; i++)
    {
        headerData_.latitudeDegreesString[i - 7] = headerData_.rawHeader[i];
    }
    headerData_.latitudeDegreesString += "";
    for(int i = 9; i < 17; i++)
    {
        headerData_.latitudeDecimalMinutesString[i - 9] = headerData_.rawHeader[i];
    }
    headerData_.northOrSouthHemisphere = headerData_.latitudeDecimalMinutesString[7];
    headerData_.latitudeDecimalMinutesString[7] = '\'';
    headerData_.latitudeDecimalMinutesString += " " + headerData_.northOrSouthHemisphere;
    //Get the longitude
    headerData_.longitudeDegreesString = "000";
    headerData_.longitudeDecimalMinutesString = "00000000";
    for(int i = 17; i < 20; i++)
    {
        headerData_.longitudeDegreesString[i - 17] = headerData_.rawHeader[i];
    }
    headerData_.longitudeDegreesString += "";
    for(int i = 20; i < 28; i++)
    {
        headerData_.longitudeDecimalMinutesString[i - 20] = headerData_.rawHeader[i];
    }
    headerData_.eastOrWestHemishere = headerData_.longitudeDecimalMinutesString[7];
    headerData_.longitudeDecimalMinutesString[7] = '\'';
    headerData_.longitudeDecimalMinutesString += " " + headerData_.eastOrWestHemishere;

    for(int i = 28; i < 30; i++)
    {
        headerData_.yearString[i - 26] = headerData_.rawHeader[i];
    }
    headerData_.year = std::stoi(headerData_.yearString);
    for(int i = 30; i < 32; i++)
    {
        headerData_.monthString[i - 30] = headerData_.rawHeader[i];
    }
    headerData_.month = std::stoi(headerData_.monthString);
    for(int i = 32; i < 34; i++)
    {
        headerData_.dayString[i - 32] = headerData_.rawHeader[i];
    }
    headerData_.day = std::stoi(headerData_.dayString);
    for(int i = 34; i < 36; i++)
    {
        headerData_.hourString[i - 34] = headerData_.rawHeader[i];
    }
    headerData_.hours = std::stoi(headerData_.hourString);
    for(int i = 36; i < 38; i++)
    {
        headerData_.minuteString[i - 36] = headerData_.rawHeader[i];
    }
    headerData_.minutes = std::stoi(headerData_.minuteString);
    for(int i = 38; i < 40; i++)
    {
        headerData_.secondString[i - 38] = headerData_.rawHeader[i];
    }
    headerData_.seconds = std::stoi(headerData_.secondString);
    for(int i = 41; i < 44; i++)
    {
        headerData_.millisecondString[i - 41] = headerData_.rawHeader[i];
    }
    headerData_.milliseconds = std::stoi(headerData_.millisecondString);
    for(int i = 44; i < 48; i++)
    {
        headerData_.sampleIntervalString[i - 44] = headerData_.rawHeader[i];
    }
    headerData_.sampleInterval = std::stoi(headerData_.sampleIntervalString);
    if(headerData_.secondString.back() == ':')
    {
        // The character ':' appears in the seconds portion of some headers
        // due to an arithmetic error in which 1 was added to the ones place
        // but was not carried over to the tens
        status_.isCarryBugEncountered_ = true;

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

void LoggerDataReader::SetOutFilePaths(string inputfileName)
{
    string firstDateString = "";
    string firstTimeString = "";
    string extension = "";
    firstDateString = headerData_.yearString + "-" + headerData_.monthString + "-" + headerData_.dayString;
    firstTimeString = headerData_.hourString + "H" + headerData_.minuteString + "M";

    string fileNameNoExtension = inputfileName;
    int pos = inputfileName.find_last_of('.');
    if(pos != string::npos)
    {
        extension = fileNameNoExtension.substr(pos, fileNameNoExtension.size());
    }

    // Strip away extension from file name
    if((fileNameNoExtension != extension) && (fileNameNoExtension.size()) > (extension.size()))
    {
        // if so then strip them off
        fileNameNoExtension = fileNameNoExtension.substr(0, fileNameNoExtension.size() - extension.size());
    }

    int infileIntValue = atoi(fileNameNoExtension.c_str());
    serialNumber_ = atoi(headerData_.serialNumberString.c_str());

    if(infileIntValue == serialNumber_ && IsOnlyDigits(fileNameNoExtension))
    {
        outDataFilePath_ = dataPath_ + "/" + burnName_ + "SN" + headerData_.serialNumberString.c_str();
        if(printRaw_)
        {
            rawOutDataFilePath_ = outDataFilePath_;
        }
    }
    else
    {
        outDataFilePath_ = dataPath_ + "/" + burnName_ + fileNameNoExtension + "_" + "SN" + headerData_.serialNumberString.c_str();
        if(printRaw_)
        {
            rawOutDataFilePath_ = outDataFilePath_;
        }
    }

    outDataFilePath_ += "_" + configurationType_ + "_" + firstDateString + "-" + firstTimeString;
    if(printRaw_)
    {
        rawOutDataFilePath_ = outDataFilePath_;
        rawOutDataFilePath_ += "_RAW.csv";
    }
    outDataFilePath_ += ".csv";
}

void LoggerDataReader::PrintOutDataLinesToFile(ofstream& outFile, const string& outFileDataLines)
{
    // Print to file
    if(outFile.good())
    {
        outFile << outFileDataLines;
    }
}

void LoggerDataReader::PrintCarryBugToLog()
{
    numErrors_++;
    logFileLines_ += "Error: File " + currentFileStats_.fileName + " has failure to carry the one in header\n";
}

void LoggerDataReader::PrintCorruptionDetectionsToLog()
{
    int currentRecord = -1;
    int previousRecord = -1;
    int startRecordForRange = -1;
    int endRecordForRange = -1;
    int sessionForRange = -1;
    int currentSession = -1;
    int previousSession = -1;

    if(status_.corruptedRecordNumbers.size() == 1)
    {
        logFileLines_ += "Error: File " + currentFileStats_.fileName + " data is corrupt in record " + std::to_string(status_.corruptedRecordNumbers[0]) + " in session " + std::to_string(status_.corruptedSessionNumbers[0]) + "\n";
    }
    else
    {
        for(int i = 0; i < status_.corruptedRecordNumbers.size(); i++)
        {
            numErrors_++;
            currentRecord = status_.corruptedRecordNumbers[i];
            currentSession = status_.corruptedSessionNumbers[i];
            if(previousRecord > 0)
            {
                if(previousSession < 0)
                {
                    previousSession = currentSession;
                }

                if((currentRecord - previousRecord) == 1 && (currentSession == previousSession))
                {
                    if(startRecordForRange == -1)
                    {
                        startRecordForRange = previousRecord;
                    }
                    endRecordForRange = currentRecord;
                    sessionForRange = status_.corruptedSessionNumbers[i];
                }
                else
                {
                    if(startRecordForRange == -1)
                    {
                        endRecordForRange = currentRecord;
                        sessionForRange = status_.corruptedSessionNumbers[i];
                        logFileLines_ += "Error: File " + currentFileStats_.fileName + " data is corrupt in record " + std::to_string(previousRecord) + " in session " + std::to_string(previousSession) + "\n";
                        startRecordForRange = -1;
                        endRecordForRange = -1;
                        sessionForRange = -1;
                    }
                    else
                    {
                        sessionForRange = status_.corruptedSessionNumbers[i];
                        logFileLines_ += "Error: File " + currentFileStats_.fileName + " data is corrupt from record " + std::to_string(startRecordForRange) + " to " + std::to_string(endRecordForRange) + " in session " + std::to_string(sessionForRange) + "\n";
                        startRecordForRange = -1;
                        endRecordForRange = -1;
                        sessionForRange = -1;
                    }
                }
                if(i == status_.corruptedRecordNumbers.size() - 1 && endRecordForRange != -1)
                {
                    sessionForRange = status_.corruptedSessionNumbers[i];
                    logFileLines_ += "Error: File " + currentFileStats_.fileName + " data is corrupt from record " + std::to_string(startRecordForRange) + " to " + std::to_string(currentRecord) + " in session " + std::to_string(sessionForRange) + "\n";
                    startRecordForRange = -1;
                    endRecordForRange = -1;
                    sessionForRange = -1;
                }
            }

            previousSession = currentSession;
            previousRecord = status_.corruptedRecordNumbers[i];
        }
    }
}

void LoggerDataReader::PrintConfigErrorsToLog()
{
    for(unsigned int i = 0; i < invalidInputFileList_.size(); i++)
    {
        logFileLines_ += "Error: File " + invalidInputFileList_[i];
        if(invalidInputFileErrorType_[i] == InvalidInputFileErrorType::CANNOT_OPEN_FILE)
        {
            logFileLines_ += " cannot be opened " + GetMyLocalDateTimeString() + "\n";
        }
        else if(invalidInputFileErrorType_[i] == InvalidInputFileErrorType::MISSING_HEADER)
        {
            logFileLines_ += " has no valid header " + GetMyLocalDateTimeString() + "\n";
        }
        else if(invalidInputFileErrorType_[i] == InvalidInputFileErrorType::MISSING_CONFIG)
        {
            logFileLines_ += " has no valid configuration " + GetMyLocalDateTimeString() + "\n";
        }
        else if(invalidInputFileErrorType_[i] == InvalidInputFileErrorType::ALREADY_OPEN)
        {
            logFileLines_ += "ERROR: The file " + outDataFilePath_ + " may be already opened by another process\n";
        }
    }
}

string LoggerDataReader::GetLogFileLines()
{
    return logFileLines_;
}

string LoggerDataReader::GetGPSFileLines()
{
    return gpsFileLines_;
}

void LoggerDataReader::PrintHeader(OutFileType::OutFileTypeEnum outFileType)
{
    const string columnHeader = "\"SN" + headerData_.serialNumberString + "\"," + "\"" + headerData_.latitudeDegreesString + " " +
        headerData_.latitudeDecimalMinutesString + "\",\"" + headerData_.longitudeDegreesString + " " +
        headerData_.longitudeDecimalMinutesString + "\", " + headerData_.sampleIntervalString + "\n";

    // Print column header data to file
    if(outFileType == OutFileType::RAW)
    {
        rawOutDataLines_ += columnHeader;

        rawOutDataLines_ += status_.rawConfigColumnTextLine;
    }
    else if(outFileType == OutFileType::PROCESSED)
    {
        outDataLines_ += columnHeader;

        outDataLines_ += status_.configColumnTextLine;
    }
}

void LoggerDataReader::UpdateSensorMaxAndMin()
{
    // Update max and min
    int currentReadingIndex = parsedNumericData_.channelNumber - 1;
    if(currentReadingIndex >= 0 && currentReadingIndex <= 8) // Data is probably not corrupted
    {
        if(parsedNumericData_.parsedIEEENumber < currentFileStats_.columnMin[currentReadingIndex])
        {
            currentFileStats_.columnMin[currentReadingIndex] = parsedNumericData_.parsedIEEENumber;
        }
        if(parsedNumericData_.parsedIEEENumber > currentFileStats_.columnMax[currentReadingIndex])
        {
            currentFileStats_.columnMax[currentReadingIndex] = parsedNumericData_.parsedIEEENumber;
        }
    }
}

void LoggerDataReader::PrintSensorDataOutput(OutFileType::OutFileTypeEnum outFileType)
{
    string dayString;
    string monthString;
    string yearString;
    string hourString;
    string minuteString;
    string secondString;
    string millisecondsString;
    string recordString;

    for(int i = 0; i < NUM_SENSOR_READINGS; i++)
    {
        if(i == 0)
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
            const string timeStamp = "\"" + yearString + "-" + monthString + "-" + dayString + " " +
                hourString + ":" + minuteString + ":" + secondString + "." + millisecondsString +
                "\"";
            if(outFileType == OutFileType::PROCESSED)
            {
                //outDataLines_ += timeStamp + "," + recordString + ",";
                outDataLines_ += timeStamp + "," + std::to_string((int)status_.sensorReadingValue[i]) + "," + recordString + ","; // for debugging
            }
            if(outFileType == OutFileType::RAW)
            {
                //rawOutDataLines_ += timeStamp + "," + recordString + ",";
                rawOutDataLines_ += timeStamp + "," + std::to_string((int)status_.sensorReadingValue[i]) + "," + recordString + ","; // for debugging
            }
        }
        else
        {
            if(configurationType_ == "F")
            {
                if(outFileType == OutFileType::RAW)
                {
                    rawOutDataLines_ += std::to_string(status_.sensorReadingValue[i]);
                }
                else if(outFileType == OutFileType::PROCESSED)
                {
                    if((sanityChecks_.FFinalMin[i] != sanityChecks_.NAMin) && (sanityChecks_.FFinalMax[i] != sanityChecks_.NAMax))
                    {
                        outDataLines_ += std::to_string(status_.sensorReadingValue[i]);
                    }
                    else
                    {
                        outDataLines_ += std::to_string(0.0);
                    }
                }
            }
            else if(configurationType_ == "H")
            {
                if(outFileType == OutFileType::RAW)
                {
                    rawOutDataLines_ += std::to_string(status_.sensorReadingValue[i]);
                }
                else if(outFileType == OutFileType::PROCESSED)
                {
                    if((sanityChecks_.HFinalMin[i] != sanityChecks_.NAMin) && (sanityChecks_.HFinalMax[i] != sanityChecks_.NAMax))
                    {
                        outDataLines_ += std::to_string(status_.sensorReadingValue[i]);
                    }
                    else
                    {
                        outDataLines_ += std::to_string(0.0);
                    }
                }
            }
            else if(configurationType_ == "T")
            {
                if(outFileType == OutFileType::RAW)
                {
                    rawOutDataLines_ += std::to_string(status_.sensorReadingValue[i]);
                }
                else if(outFileType == OutFileType::PROCESSED)
                {
                    outDataLines_ += std::to_string(status_.sensorReadingValue[i]);
                }
            }

            if(i < (NUM_SENSOR_READINGS - 1))
            {
                if(outFileType == OutFileType::PROCESSED)
                {
                    outDataLines_ += ",";
                }
                else if(outFileType == OutFileType::RAW)
                {
                    rawOutDataLines_ += ",";
                }
            }
            else
            {
                if(outFileType == OutFileType::PROCESSED)
                {
                    outDataLines_ += "\n";
                }
                if(outFileType == OutFileType::RAW)
                {
                    rawOutDataLines_ += "\n";
                }
            }
        }
    }
}

void LoggerDataReader::PrintGPSFileLine()
{
    // file, logger unit, and gps data
    gpsFileLines_ = currentFileStats_.fileName + ",SN" + headerData_.serialNumberString + "," + std::to_string(status_.loggingSession) +
        "," + configurationType_ + "," + std::to_string(headerData_.decimalDegreesLongitude) + "," +
        std::to_string(headerData_.decimalDegreesLatitude) + ",";

    // start time information for session
    gpsFileLines_ += startTimeForSession_.yearString + "-" + startTimeForSession_.monthString + "-" + startTimeForSession_.dayString + " " +
        startTimeForSession_.hourString + ":" + startTimeForSession_.minuteString + ":" + startTimeForSession_.secondString + "." + startTimeForSession_.millisecondString +
        ",";

    // end time information for session
    gpsFileLines_ += endTimeForSession_.yearString + "-" + endTimeForSession_.monthString + "-" + endTimeForSession_.dayString + " " +
        endTimeForSession_.hourString + ":" + endTimeForSession_.minuteString + ":" + endTimeForSession_.secondString + "." + endTimeForSession_.millisecondString + "\n";
}

float LoggerDataReader::UnsignedIntToIEEEFloat(uint32_t binaryNumber)
{
    // Cast the unsigned 32 bit int as a float
    float* pFloat;
    pFloat = (float*)(&binaryNumber);
    float value = *pFloat;

    return value;
}
