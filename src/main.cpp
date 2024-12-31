#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

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

//            std::ifstream windTunnelDataFile("/home/jforthofer/src/FBLoggerUtility/wind_tunnel_data/TG201811071345.txt", std::ios::in);
//            const int NUM_ANGLES = 100;
//            const int NUM_REYNOLDS_NUMBERS = 11;
//            const int NUM_DATA_LINES = NUM_ANGLES * NUM_REYNOLDS_NUMBERS;
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




	


    double pressureXvoltage = 1.219326932;
    double pressureYvoltage = 1.404189628;
    double pressureZvoltage = 1.276611831;
//    double pressureYvoltage = 1.288460104;
//    double pressureZvoltage = 1.267494146;
	double temperature = 13.6;
	double sensorBearing = 0.0;

	convertVelocity Convert;

    if (Convert.convert(temperature, pressureXvoltage, pressureYvoltage, pressureZvoltage, sensorBearing, pressureCoefficients_, angles_, ReynoldsNumbers_) == true)
		printf("Conversion successful\nu = %lf\nv = %lf\nw = %lf\n", Convert.u, Convert.v, Convert.w);
	else
		printf("CONVERSION FAILED\n");

	std::cout << "Conversion complete.\n" << std::endl;

	return 0;
}

