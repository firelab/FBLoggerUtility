#pragma once

// Allows backward compatabilty for logger data from before 2022

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>

#include "common_logger_constants.h"
#include "convert_velocity.h"

using std::string;
using std::vector;
using std::ifstream;
using std::stringstream;
using std::pair;
using std::map;

static const unsigned int NUM_LEGACY_SENSOR_READINGS = 9;

struct LegacyStatsFileData
{
    double columnMin[NUM_LEGACY_SENSOR_READINGS] = { 0,0,0,0,0,0,0,0,0 };
    double columnMax[NUM_LEGACY_SENSOR_READINGS] = { 0,0,0,0,0,0,0,0,0 };
    bool columnFailedSanityCheckRaw[NUM_LEGACY_SENSOR_READINGS] = { false,false,false,false,false,false,false,false,false };
    bool columnFailedSanityCheckFinal[NUM_LEGACY_SENSOR_READINGS] = { false,false,false,false,false,false,false,false,false };
    bool isFailedSanityCheckRaw = false;
    bool isFailedSanityCheckFinal = false;
    string fileName = "";
};

struct LegacySharedData
{
    int totalFilesProcessed = 0;
    int totalInvalidInputFiles = 0;
    int totalErrors = 0;
    string burnName = "";
    string logFilePath = "";
    string gpsFilePath = "";
    string configFilePath = "";
    string dataPath = "";
    string windTunnelDataTablePath = "";
    bool aborted = false;
    bool configFileGood = false;
    bool gpsFileGood = false;
    bool printRaw = false;
    map<int, string>* configMap = NULL;
    map<int, double>* sensorBearingMap = NULL;
    map<int, double>* heatFlux_X_VoltageOffsetMap = nullptr;
    map<int, double>* heatFlux_Y_VoltageOffsetMap = nullptr;
    map<int, double>* heatFlux_Z_VoltageOffsetMap = nullptr;
    map<int, LegacyStatsFileData>* statsFileMap = nullptr;
    vector<double>* angles = nullptr;
    vector<double>* ReynloldsNumbers = nullptr;
    vector<vector<double>>* pressureCoefficients = nullptr;
    vector<string>* inputFilePathList = nullptr;

    void reset()
    {
        totalFilesProcessed = 0;
        totalInvalidInputFiles = 0;
        totalErrors = 0;

        aborted = false;
        printRaw = false;
        configFileGood = false;
        gpsFileGood = false;

        burnName = "";
        logFilePath = "";
        gpsFilePath = "";
        configFilePath = "";
        dataPath = "";
        windTunnelDataTablePath = "";

        if(configMap != nullptr)
        {
            configMap->clear();
            delete configMap;
            configMap = nullptr;
        }
        if(heatFlux_X_VoltageOffsetMap != nullptr)
        {
            heatFlux_X_VoltageOffsetMap->clear();
            delete heatFlux_X_VoltageOffsetMap;
            heatFlux_X_VoltageOffsetMap = nullptr;
        }
        if(heatFlux_Y_VoltageOffsetMap != nullptr)
        {
            heatFlux_Y_VoltageOffsetMap->clear();
            delete heatFlux_Y_VoltageOffsetMap;
            heatFlux_Y_VoltageOffsetMap = nullptr;
        }
        if(heatFlux_Z_VoltageOffsetMap != nullptr)
        {
            heatFlux_Z_VoltageOffsetMap->clear();
            delete heatFlux_Z_VoltageOffsetMap;
            heatFlux_Z_VoltageOffsetMap = nullptr;
        }
        if(sensorBearingMap != nullptr)
        {
            sensorBearingMap->clear();
            delete sensorBearingMap;
            sensorBearingMap = nullptr;
        }
        if(statsFileMap != nullptr)
        {
            statsFileMap->clear();
            delete statsFileMap;
            statsFileMap = nullptr;
        }
        if(angles != nullptr)
        {
            angles->clear();
            delete angles;
            angles = nullptr;
        }
        if(ReynloldsNumbers != nullptr)
        {
            ReynloldsNumbers->clear();
            delete ReynloldsNumbers;
            ReynloldsNumbers = nullptr;
        }
        if(pressureCoefficients != nullptr)
        {
            pressureCoefficients->clear();
            delete pressureCoefficients;
            pressureCoefficients = nullptr;
        }
        if(inputFilePathList != nullptr)
        {
            inputFilePathList->clear();
            delete inputFilePathList;
            inputFilePathList = nullptr;
        }
    }
};

class LegacyLoggerDataReader
{
    // Internal data structures
    struct HeaderData
    {
        string rawHeader = "0000";
        string serialNumberString = "00000";
        string latitudeDegreesString = "00";
        string latitudeDecimalMinutesString = "00000000";
        string northOrSouthHemisphere = "0";
        string longitudeDegreesString = "000";
        string longitudeDecimalMinutesString = "00000000";
        string eastOrWestHemishere = "0";
        string dayString = "00";
        string monthString = "00";
        string yearString = "2000";
        string hourString = "00";
        string minuteString = "00";
        string secondString = "00";
        string millisecondString = "000";
        string sampleIntervalString = "0000";
        unsigned int year = 0;
        unsigned int month = 0;
        unsigned int day = 0;
        unsigned int hours = 0;
        unsigned int minutes = 0;
        unsigned int seconds = 0;
        unsigned int milliseconds = 0;
        unsigned int sampleInterval = 0;
        double decimalDegreesLatitude = 0;
        double decimalDegreesLongitude = 0;
    };

    struct IconUrls
    {
        const string fid = "http://maps.google.com/mapfiles/kml/paddle/F.png";
        const string heatFlux = "http://maps.google.com/mapfiles/kml/paddle/H.png";
        const string temperature = "http://maps.google.com/mapfiles/kml/paddle/T.png";
    };

    struct ParsedNumericData
    {
        uint8_t rawHexNumber[NUM_DATA_BYTES];
        uint8_t channelNumber = 0;
        float parsedIEEENumber = 0;
        unsigned int intFromBytes = 0;
    };

    struct InFileReadingStatus
    {
        bool isChannelPresent[NUM_LEGACY_SENSOR_READINGS] = { false, false, false, false, false,
                                     false, false, false, false };
        int8_t previousChannelNumber = 0;
        uint8_t sensorReadingCounter = 0;
        uint8_t loggingSession = 0;
        uint32_t position = 0;
        uint32_t recordNumber = 0;
        size_t filePositionLimit;
        string configColumnTextLine;
        string rawConfigColumnTextLine;
        string columnRawType[NUM_LEGACY_SENSOR_READINGS];
        double sensorReadingValue[NUM_LEGACY_SENSOR_READINGS];
        bool headerFound = false;
        bool firstHeaderFound = false;
        bool isSerialNumberValid = false;
        bool isConfigurationTypeValid = false;
        bool isSensorNumberValid = false;
        bool isSensorBearingValid = false;
        bool isHeatFlux_X_VoltageOffsetValid = false;
        bool isHeatFlux_Y_VoltageOffsetValid = false;
        bool isHeatFlux_Z_VoltageOffsetValid = false;
        bool isGoodOutput = true;
        bool isCarryBugEncountered_ = false;
        bool isDataCorruptionEncountered_ = false;
        bool isRecoveredAfterDataCorruption_ = false;
        unsigned int configFileLineNumber = 0;
        vector<int> corruptedRecordNumbers;
        vector<int> corruptedSessionNumbers;
    };

    struct InvalidInputFileErrorType
    {
        enum InvalidInputErrorTypeEnum
        {
            CANNOT_OPEN_FILE = 0,
            MISSING_HEADER = 1,
            MISSING_CONFIG = 2,
            ALREADY_OPEN = 3
        };
    };

    struct FIDPackageIndex
    {
        enum FIDPackageIndexEnum
        {
            TEMPERATURE = 0,
            FID_VOLTAGE = 1,
            PRESSURE_VOLTAGE = 2,
            PRESSURE = 2,
            PANEL_TEMP = 8
        };
    };

    struct HeatFluxIndex
    {
        enum HeatFluxIndexEnum
        {
            TEMPERATURE_SENSOR_ONE = 0,
            TEMPERATURE_SENSOR_TWO = 1,
            PRESSURE_SENSOR_U = 2,
            VELOCITY_U = 2,
            PRESSURE_SENSOR_V = 3,
            VELOCITY_V = 3,
            PRESSURE_SENSOR_W = 4,
            VELOCITY_W = 4,
            HEAT_FLUX_VOLTAGE = 5,
            HEAT_FLUX = 5,
            HEAT_FLUX_TEMPERATURE_VOLTAGE = 6,
            HEAT_FLUX_TEMPERATURE = 6,
            PANEL_TEMP = 8
        };
    };

    struct TCIndex
    {
        enum TCIndexEnum
        {
            PANEL_TEMP = 8
        };
    };

    struct OutFileType
    {
        enum OutFileTypeEnum
        {
            RAW = 0,
            PROCESSED = 1
        };
    };

    struct SanityChecks
    {
        enum SanityCheckTypeEnum
        {
            RAW = 0,
            FINAL = 1,
            IGNORE_MIN = -999999,
            IGNORE_MAX = 999999,
            INVALID_SENSOR_READING = 9999,
            INVALID_VELCOCITY_CONVERSION = -9999,
            INITIAL_MIN = 9999,
            INITIAL_MAX = -9999
        };

        const double FIDVoltageMin = 0.0;
        const double FIDVoltageMax = 2.5;
        const double heatFluxMin = -5.0;
        const double heatFluxMax = 150.0;
        const double heatFluxTemperatureMin = -40.0;
        const double heatFluxTemperatureMax = 1400.0;
        const double heatFluxTemperatureVoltageMin = -1.2499999999;
        const double heatFluxTemperatureVoltageMax = -0.0000000001;
        const double heatFluxVoltageMin = 0.0;
        const double heatFluxVoltageMax = 2.5;
        const double NAMin = IGNORE_MIN;
        const double NAMax = IGNORE_MAX;
        const double pressureVoltageMin = 0.0;
        const double pressureVoltageMax = 2.5;
        const double pressureMin = -797.0;
        const double pressureMax = 797.0;
        const double temperatureMin = -40.0;
        const double temperatureMax = 1400.0;
        const double velocityMin = -36.0;
        const double velocityMax = 36.0;

        double FRawMin[NUM_LEGACY_SENSOR_READINGS];
        double FRawMax[NUM_LEGACY_SENSOR_READINGS];
        double FFinalMin[NUM_LEGACY_SENSOR_READINGS];
        double FFinalMax[NUM_LEGACY_SENSOR_READINGS];
        double HRawMin[NUM_LEGACY_SENSOR_READINGS];
        double HRawMax[NUM_LEGACY_SENSOR_READINGS];
        double HFinalMin[NUM_LEGACY_SENSOR_READINGS];
        double HFinalMax[NUM_LEGACY_SENSOR_READINGS];
        double TMin[NUM_LEGACY_SENSOR_READINGS];
        double TMax[NUM_LEGACY_SENSOR_READINGS];
    };

    struct ConfigFileLine
    {
        string serialNumberString = "";
        string conifgurationString = "";
        string sensorNumberString = "";
        string sensorBearingString = "";
        string heatFlux_X_VoltageOffsetString = "";
        string heatFlux_Y_VoltageOffsetString = "";
        string heatFlux_Z_VoltageOffsetString = "";

        double heatFlux_X_VoltageOffsetValue = 0.0;
        double heatFlux_Y_VoltageOffsetValue = 0.0;
        double heatFlux_Z_VoltageOffsetValue = 0.0;
        double sensorBearingValue = 0.0;
    };

public:
    // Public interface
    LegacyLoggerDataReader(string dataPath, string burnName);
    LegacyLoggerDataReader(const LegacySharedData& sharedData);
    ~LegacyLoggerDataReader();

    void SetSharedData(const LegacySharedData& sharedData);
    void GetSharedData(LegacySharedData& sharedData);

    void ProcessSingleDataFile(int fileIndex);
    //void PrintStatsFile();
    void CheckConfig();
    void ReportAbort();
    bool CheckConfigFileFormatIsValid(ifstream& configFile);

    string GetLogFileLines();
    string GetGPSFileLines();

    void SetPrintRaw(bool option);
    void SetDataPath(string dataPath);
    void SetWindTunnelDataTablePath(string windTunnelDataTablePath);
    void SetAppPath(string appPath);
    void SetConfigFile(string configFileFullPath);
    string GetDataPath();
    string GetStatsFilePath();
    unsigned int GetNumberOfInputFiles();
    unsigned int GetNumFilesProcessed();
    unsigned int GetNumInvalidFiles();
    unsigned int GetNumErrors();

    bool IsConfigFileValid();
    bool IsDoneReadingDataFiles();

    bool CreateWindTunnelDataVectors();

private:
    // Private methods
    static inline bool double_equals(double a, double b, double epsilon = 0.000001)
    {
        return std::abs(a - b) < epsilon;
    }

    double CalculateHeatFlux(double rawVoltage);
    double CalculateHeatFluxTemperature(double rawVoltage);
    int CalculateHeatFluxComponentVelocities(double temperatureOne, double temperatureTwo,
        double pressureVoltageU, double pressureVoltageV, double pressureVoltageW, double sensorBearing);
    double CalculateFIDPackagePressure(double rawVoltage);
    double CalculateTCTemperature(double rawVoltage);
    void PerformNeededDataConversions();
    void PerformSanityChecksOnValues(SanityChecks::SanityCheckTypeEnum sanityCheckType);
    void StoreInputFileContentsToRAM(ifstream& inFile);
	void ReadConfig();
    void ParseTokensFromLineOfConfigFile(string& line);
    void ProcessLineOfConfigFile();
    void CheckConfigForAllFiles();
    bool GetFirstHeader();
    void ReadNextHeaderOrNumber();
    uint32_t GetIntFromByteArray(uint8_t arr[NUM_DATA_BYTES]);
    void GetRawNumber();
	void CheckForHeader();
    void UpdateTime();
    void GetHeader();
    void ParseHeader();
    void SetOutFilePaths(string inFilePaths);
    void PrintOutDataLinesToFile(ofstream& outFile, const string& outFileDataLines);
    void PrintCarryBugToLog();
    void PrintCorruptionDetectionsToLog();
    void PrintConfigErrorsToLog();
    void PrintGPSFileLine();
    void PrintHeader(OutFileType::OutFileTypeEnum outFileType);

    void UpdateSensorMaxAndMin();
    void PrintSensorDataOutput(OutFileType::OutFileTypeEnum outFileType);
    float UnsignedIntToIEEEFloat(uint32_t binaryNumber);
    void ResetHeaderData();
    void ResetInFileReadingStatus();
    void ResetCurrentFileStats();
    void SetConfigDependentValues();
    void UpdateStatsFileMap();
    void StoreSessionStartTime();
    void StoreSessionEndTime();

    void DegreesDecimalMinutesToDecimalDegrees(HeaderData& headerData);

    void HandleCorruptData();

    //void BeginKMLFile();
    //string FormatPlacemark();
    //void EndKMLFile();

    // Private data members
    vector<string> inputFilePathList_;
    vector<string> invalidInputFileList_;
    vector<int> invalidInputFileErrorType_;

    map<int, string> configMap_;
    map<int, double> sensorBearingMap_;
    map<int, double> heatFlux_X_VoltageOffsetMap_;
    map<int, double> heatFlux_Y_VoltageOffsetMap_;
    map<int, double> heatFlux_Z_VoltageOffsetMap_;
    map<int, LegacyStatsFileData> statsFileMap_;

    bool isConfigFileValid_;
    bool outputsAreGood_;
    bool printRaw_;

    int serialNumber_;
    unsigned int currentFileIndex_;
    unsigned int numErrors_;
    unsigned int numFilesProcessed_;
    unsigned int numInvalidInputFiles_;
    unsigned int numInvalidOutputFiles_;

    string inDataFilePath_;
    string outDataFilePath_;
    string rawOutDataFilePath_;
    string gpsFileLines_;
    string kmlFilePath_;
    string outDataLines_;
    string rawOutDataLines_;
    string burnName_;
    string appPath_;
    string dataPath_;
    string windTunnelDataTablePath_;
    string logFileLines_;
    string statsFilePath_;
    string configFilePath_;
    string configurationType_;
    string windTunnelDataPath_;

    //ofstream kmlFile_;
    //ofstream gpsFile_;
    stringstream lineStream_;

    HeaderData headerData_;
    HeaderData startTimeForSession_;
    HeaderData endTimeForSession_;
    InFileReadingStatus status_;
    ParsedNumericData parsedNumericData_;
    ConfigFileLine configFileLine_;
    SanityChecks sanityChecks_;
    convertVelocity convertVelocity_;
    LegacyStatsFileData currentFileStats_;
    IconUrls iconsUrls_;

    vector<char> inputFileContents_;

    // The three angles_, ReynoldsNumbers_ and pressureCoefficients_ below are populated from the wind tunnel data file 
    vector<double> angles_;
    vector<double> ReynloldsNumbers_;
    vector<vector<double>> pressureCoefficients_; // 2D vector of pressure coefficients with angle and Reynolds number as indices
};
