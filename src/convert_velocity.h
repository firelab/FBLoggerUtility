#pragma once

#include "air.h"
#include <math.h>
#include <vector>

class convertVelocity
{
public:
	convertVelocity();
	~convertVelocity();
	bool convert(double &temperature, double &pressureXvoltage, double &pressureYvoltage, double &pressureZvoltage, double &sensorBearing, const std::vector<std::vector<double>> &lookupTablePressureCoefficient, const std::vector<double> &lookupTableAngle, const std::vector<double> &lookupTableReynoldsNumber);

	double u;			//X component of velocity (m/s)
	double v;			//Y component of velocity (m/s)
	double w;			//Z component of velocity (m/s)

private:
	double getVelocity(double &temperature, double &pressure, double &cp);
	double getVelocityResidual();
	double getCp(double angle, double ReynoldsNumber, const std::vector<std::vector<double>> &lookupTablePressureCoefficient, const std::vector<double> &lookupTableAngle, const std::vector<double> &lookupTableReynoldsNumber);

	Air air;
	double pressureX;	//X direction differential pressure (Pascals)
	double pressureY;	//Y direction differential pressure (Pascals)
	double pressureZ;	//Z direction differential pressure (Pascals)
	double cpx;			//X pressure coefficient
	double cpy;			//X pressure coefficient
	double cpz;			//X pressure coefficient
	double uOld;		//old X component of velocity (m/s)
	double vOld;		//old Y component of velocity (m/s)
	double wOld;		//old Z component of velocity (m/s)
	int iters;			//number of iterations
	int maxIters;		//maximum number of iterations
	double velocityTolerance;	//velocity change tolerance (m/s)
	double xAngle;		//velocity angle relative to normal of x direction probe (degrees) [0 - 180]
	double yAngle;		//velocity angle relative to normal of y direction probe (degrees) [0 - 180]
	double zAngle;		//velocity angle relative to normal of z direction probe (degrees) [0 - 180]
	double velocityMagnitude;	//velocity magnitude (m/s)
	double ReX;			//Reynold's number for x-direction
	double ReY;			//Reynold's number for y-direction
	double ReZ;			//Reynold's number for z-direction
    double Re;          //Reynold's number for velocity magnitude
	double ReLarge;		//Large Reynold's number, sufficiently on flat part of cp(angle,Re) curve
	double relaxCp;		//cp relaxation value to ensure smooth convergence [0 - 1]
	double diskDiameter;	//diameter of probe disk, used in Reynold's number calculation (m)
	double temperatureK;	//air temperature (K)
	double velocityResidual;	//change in velocity from last iteration
    double pi;
};
