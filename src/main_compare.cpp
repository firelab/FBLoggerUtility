#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif


#include "convert_velocity.h"


template<typename T>
std::vector<std::vector<T>> make_2d_vector(std::size_t rows, std::size_t cols)
{
    return std::vector<std::vector<T>>(rows, std::vector<T>(cols));
}


int main() {
    std::cout << "Beginning conversion..." << std::endl;


    std::vector<double> angles_, ReynoldsNumbers_;
    std::vector<std::vector<double>> pressureCoefficients_;


    //  This function is copied from logger_data_reader.cpp to emulate it's behavior, but without need to include that file and it's Qt dependencies.  Just for testing wind conversion here.
    //bool CreateWindTunnelDataVectors()
    //{
    double angle, ReynoldsNumber, pressureCoefficient;
    string line;

//    std::ifstream windTunnelDataFile("/home/jforthofer/src/FBLoggerUtility/wind_tunnel_data/TG201811071345.txt", std::ios::in);
//    const int NUM_ANGLES = 100;
//    const int NUM_REYNOLDS_NUMBERS = 11;
//    const int NUM_DATA_LINES = NUM_ANGLES * NUM_REYNOLDS_NUMBERS;
                std::ifstream windTunnelDataFile("/home/jforthofer/src/FBLoggerUtility/wind_tunnel_data/TG202411061143.txt", std::ios::in);
                const int NUM_ANGLES = 10;
                const int NUM_REYNOLDS_NUMBERS = 5;
                const int NUM_DATA_LINES = NUM_ANGLES * NUM_REYNOLDS_NUMBERS;

    bool isSuccessful = false;
    angles_.clear();
    angles_.reserve(NUM_ANGLES);
    ReynoldsNumbers_.clear();
    ReynoldsNumbers_.reserve(NUM_REYNOLDS_NUMBERS);
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
                    ReynoldsNumbers_.push_back(ReynoldsNumber);
                }
                pressureCoefficients_[angleIndex][ReynoldsNumberIndex] = pressureCoefficient;
            }
        }
        if(lineCount == NUM_DATA_LINES)
        {
            isSuccessful = true;
        }
    }

    //    return isSuccessful;
    //}


    //----Read data from file-------
    struct DataPoint {
        double pressureXvoltage, pressureYvoltage, pressureZvoltage, Xvelocity_measured, Yvelocity_measured, Zvelocity_measured; // Assuming the data consists of 6 columns of doubles.
    };

    string filename = "/home/jforthofer/src/FBLoggerUtility/wind_tunnel_data/3D_validation_data_20241106.txt";
    //string filename = "/home/jforthofer/src/FBLoggerUtility/wind_tunnel_data/1-13 testing volt-vel table no angles.txt";
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Could not open the file: " << filename << endl;
        return 1; // Exit if file cannot be opened.
    }

    vector<DataPoint> data; // Vector to store all the data points

    while (getline(file, line)) {
        stringstream ss(line);
        DataPoint point;

        // Read 6 space-separated values into the struct
        ss >> point.pressureXvoltage >> point.pressureYvoltage >> point.pressureZvoltage >> point.Xvelocity_measured >> point.Yvelocity_measured >> point.Zvelocity_measured;

        // Add the point to the vector
        data.push_back(point);
    }

    file.close();


    // Print the data to output file
    string output_filename = "output_data.txt";
    ofstream output_file(output_filename);
    if (!output_file.is_open()) {
        cerr << "Could not open the output file: " << output_filename << endl;
        return 1; // Exit if the output file cannot be created.
    }


    // Write the data to the output file

    output_file << "pressureXvoltage" << " " << "pressureYvoltage" << " " << "pressureZvoltage" << " "
                << "Xvelocity_measured" << " " << "Yvelocity_measured" << " " << "Zvelocity_measured" << " " << "speed_measured" << " "
                << "Xvelocity_predicted" << " " << "Yvelocity_predicted" << " " << "Zvelocity_predicted" << " " << "speed_predicted" << endl;

    double temperature = 13.6;
    double sensorBearing = 0.0;

    int successful = 0;
    int failed = 0;

    for (auto& point : data) {
        convertVelocity Convert;
        double speed_measured, speed_predicted;

        if (Convert.convert(temperature, point.pressureXvoltage, point.pressureYvoltage, point.pressureZvoltage, sensorBearing, pressureCoefficients_, angles_, ReynoldsNumbers_) == true)
        {
            successful++;
            printf("Conversion successful\nu = %lf\nv = %lf\nw = %lf\n", Convert.u, Convert.v, Convert.w);
        }else{
            failed++;
            printf("CONVERSION FAILED\n");
        }

        speed_measured = sqrt(point.Xvelocity_measured*point.Xvelocity_measured+point.Yvelocity_measured*point.Yvelocity_measured+point.Zvelocity_measured*point.Zvelocity_measured);
        speed_predicted = sqrt(Convert.u*Convert.u+Convert.v*Convert.v+Convert.w*Convert.w);

        output_file << point.pressureXvoltage << " " << point.pressureYvoltage << " " << point.pressureZvoltage << " "
                    << point.Xvelocity_measured << " " << point.Yvelocity_measured << " " << point.Zvelocity_measured << " " << speed_measured << " "
                    << Convert.u << " " << Convert.v << " " << Convert.w << " " << speed_predicted << endl;
    }

    output_file.close();


    std::cout << "\n\nConversion complete.\n" << std::endl;
    std::cout << "Successful\t=\t" << successful << std::endl;
    std::cout << "Failed\t=\t" << failed << std::endl;

    return 0;
}

