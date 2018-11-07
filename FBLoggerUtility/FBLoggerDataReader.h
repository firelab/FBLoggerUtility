#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>

#include "convertVelocity.h"

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::pair;
using std::map;

class FBLoggerDataReader
{
public:
    // Public interface
    FBLoggerDataReader(string dataPath, string burnName);
    ~FBLoggerDataReader();

    //void PrepareToReadDataFiles();
    void ProcessSingleDataFile();
    void PrintStatsFile();
    void CheckConfig();
    void ReportAbort();
    bool CheckConfigFileFormatIsValid(ifstream& configFile);
    void PrintFinalReportToLog();

    void SetPrintRaw(bool option);
    void SetDataPath(string dataPath);
    void SetWindTunnelDataTablePath(string windTunnelDataTablePath);
    void SetAppPath(string appPath);
    void SetConfigFile(string configFileFullPath);
    string GetDataPath();
    string GetLogFilePath();
    string GetStatsFilePath();
    unsigned int GetNumberOfInputFiles();
    unsigned int GetNumFilesProcessed();
    unsigned int GetNumInvalidFiles();
  
    bool IsConfigFileValid();
    bool IsLogFileGood();
    bool IsDoneReadingDataFiles();

    bool CreateWindTunnelDataVectors();

private:
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
        uint8_t rawHexNumber[4];
        uint8_t channelNumber = 0;
        float parsedIEEENumber = 0;
        unsigned int intFromBytes = 0;
    };

    struct InFileReadingStatus
    {
        uint8_t sensorReadingCounter = 0;
        uint8_t loggingSession = 0;
        uint32_t pos = 0;
        uint32_t recordNumber = 0;
        string configColumnTextLine;
        string rawConfigColumnTextLine;
        string columnRawType[9];
        double sensorReadingValue[9];
        bool headerFound = false;
        bool isSerialNumberValid = false;
        bool isConfigurationTypeValid = false;
        bool isSensorNumberValid = false;
        bool isSensorBearingValid = false;
        bool isGoodOutput = true;
        bool carryBugEncountered_ = false;
        unsigned int configFileLineNumber = 0;
    };

    struct StatsFileData
    {
        double columnMin[9];
        double columnMax[9];
        bool columnFailedSanityCheckRaw[9];
        bool columnFailedSanityCheckFinal[9];
        bool isFailedSanityCheckRaw;
        bool isFailedSanityCheckFinal;
        string fileName;
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

        double FRawMin[9];
        double FRawMax[9];
        double FFinalMin[9];
        double FFinalMax[9];
        double HRawMin[9];
        double HRawMax[9];
        double HFinalMin[9];
        double HFinalMax[9];
        double TMin[9];
        double TMax[9];
    };

    struct ConfigFileLine
    {
        string serialNumberString = "";
        string conifgurationString = "";
        string sensorNumberString = "";
        string sensorBearingString = "";
        double sensorBearingValue = 0.0;
    };

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
    void ProcessErrorsInLineOfConfigFile();
    void CheckConfigForAllFiles();
    bool GetFirstHeader();
    void ReadNextHeaderOrNumber();
    uint32_t GetIntFromByteArray(uint8_t arr[4]);
    void GetRawNumber();
	void CheckForHeader();
    void UpdateTime();
    string GetMyLocalDateTimeString();
    void GetHeader();
    void ParseHeader();
    void SetOutFilePaths(string inFileName);
    void PrintCarryBugToLog();
    void PrintConfigErrorsToLog();
    void PrintGPSFileHeader();
    void PrintGPSFileLine();
    void PrintLogFileLine();
    void PrintHeader(ofstream& pOutFile, OutFileType::OutFileTypeEnum outFileType);
    string MakeStringWidthTwoFromInt(int headerData);
    string MakeStringWidthThreeFromInt(int headerData);
    void UpdateSensorMaxAndMin();
    void PrintSensorDataOutput(ofstream& pOutFile);
    float UnsignedIntToIEEEFloat(uint32_t binaryNumber);
    void ResetHeaderData();
    void ResetInFileReadingStatus();
    void ResetCurrentFileStats();
    void SetConfigDependentValues();
    void UpdateStatsFileMap();
    void StoreSessionStartTime();
    void StoreSessionEndTime();

    void DegreesDecimalMinutesToDecimalDegrees(HeaderData& headerData);

    void BeginKMLFile();
    string FormatPlacemark();
    void EndKMLFile();

    // Private data members
    static const unsigned int NUM_SENSOR_READINGS = 9;
    static const uint8_t BYTES_READ_PER_ITERATION = 5; // 5 total: 4 byte sensor reading, 1 byte channel number

    vector<string> inputFilesNameList_;
    vector<string> invalidInputFileList_;
    vector<int> invalidInputFileErrorType_;

    map<int, string> configMap_;
    map<int, double> sensorBearingMap_;
    map<int, StatsFileData> statsFileMap_;

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
    string gpsFilePath_;
    string gpsFileLine_;
    string kmlFilePath_;
    string outDataLine_;
    string appPath_;
    string dataPath_;
    string logFilePath_;
    string logFileLine_;
    string statsFilePath_;
    string configFilePath_;
    string configurationType_;
    string windTunnelDataPath_;

    ofstream kmlFile_;
    ofstream gpsFile_;
    ofstream logFile_;
    stringstream lineStream_;

    HeaderData headerData_;
    HeaderData startTimeForSession_;
    HeaderData endTimeForSession_;
    InFileReadingStatus status_;
    ParsedNumericData parsedNumericData_;
    ConfigFileLine configFileLine_;
    SanityChecks sanityChecks_;
    convertVelocity convertVelocity_;
    StatsFileData currentFileStats_;
    IconUrls iconsUrls_;

    vector<char> inputFileContents_;

    vector<double> angles_;
    vector<double> ReynloldsNumbers_;
    vector<vector<double>> pressureCoeeffiencts_; // 2D vector of pressure coefficients with angle and Reynolds number as indices

    clock_t startClock_;
    double totalTimeInSeconds_;
};
