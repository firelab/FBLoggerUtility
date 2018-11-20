#include "stdafx.h"
#include "convertVelocity.h"

convertVelocity::convertVelocity()
{
	maxIters = 100;
	velocityTolerance = 0.1;
	ReLarge = 20000;
	relaxCp = 0.8;
	diskDiameter = 0.04445;
}

convertVelocity::~convertVelocity()
{

}

bool convertVelocity::convert(double &temperature, double &pressureXvoltage, double &pressureYvoltage, double &pressureZvoltage, double &sensorBearing, const std::vector<std::vector<double>> &lookupTablePressureCoefficient, const std::vector<double> &lookupTableAngle, const std::vector<double> &lookupTableReynoldsNumber)

{
	temperatureK = temperature + 273.15;	//convert to Kelvin

	pressureX = (pressureXvoltage - 1.265) * 0.9517 * 249.09;	//convert from voltage to pascals
	pressureY = (pressureYvoltage - 1.265) * 0.9517 * 249.09;	//convert from voltage to pascals
	pressureZ = (pressureZvoltage - 1.265) * 0.9517 * 249.09;	//convert from voltage to pascals

/////////////TEST VALUES///////////////////////////

//	pressureX = 2.49;
//	pressureY = 6.21;
//	pressureZ = 0.249;
//	diskDiameter = 0.0254;
//	temperatureK = 300.0;

//////////////////////////////////////////////////


	//start with assumed pressure coefficient of one
	cpx = 1.0;
	cpy = 1.0;
	cpz = 1.0;

	//compute velocity components
	u = getVelocity(temperatureK, pressureX, cpx);
	v = getVelocity(temperatureK, pressureY, cpy);
	w = getVelocity(temperatureK, pressureZ, cpz);

	ReX = ReLarge;
	ReY = ReLarge;
	ReZ = ReLarge;

	iters = 0;
	do {
		iters += 1;

		uOld = u;
		vOld = v;
		wOld = w;

		velocityMagnitude = sqrt(u*u + v*v + w*w);

		//compute angles relative to each probe's normal (zero degrees is the direction normal to the probe,
		//    90 degrees is flow parallel to the probe face)
		xAngle = acos(u / velocityMagnitude) * 180.0 / PI;
		yAngle = acos(v / velocityMagnitude) * 180.0 / PI;
		zAngle = acos(w / velocityMagnitude) * 180.0 / PI;

		//get new cp from calibration curve using latest Reynold's numbers
		//    use a relaxation method to update value to ensure smooth convergence
		cpx = relaxCp * getCp(xAngle, ReX, lookupTablePressureCoefficient, lookupTableAngle, lookupTableReynoldsNumber) + (1.0 - relaxCp) * cpx;
		cpy = relaxCp * getCp(yAngle, ReY, lookupTablePressureCoefficient, lookupTableAngle, lookupTableReynoldsNumber) + (1.0 - relaxCp) * cpy;
		cpz = relaxCp * getCp(zAngle, ReZ, lookupTablePressureCoefficient, lookupTableAngle, lookupTableReynoldsNumber) + (1.0 - relaxCp) * cpz;

		//compute Reynold's number and new cp from calibration curve for each disk
		ReX = air.get_rho(temperatureK) * getVelocity(temperatureK, pressureX, cpx) * diskDiameter / air.get_mu(temperatureK);
		cpx = getCp(xAngle, ReX, lookupTablePressureCoefficient, lookupTableAngle, lookupTableReynoldsNumber);
		ReY = air.get_rho(temperatureK) * getVelocity(temperatureK, pressureY, cpy) * diskDiameter / air.get_mu(temperatureK);
		cpy = getCp(yAngle, ReY, lookupTablePressureCoefficient, lookupTableAngle, lookupTableReynoldsNumber);
		ReZ = air.get_rho(temperatureK) * getVelocity(temperatureK, pressureZ, cpz) * diskDiameter / air.get_mu(temperatureK);
		cpz = getCp(zAngle, ReZ, lookupTablePressureCoefficient, lookupTableAngle, lookupTableReynoldsNumber);

		//compute revised velocity estimates based on the revised pressure coefficients
		u = getVelocity(temperatureK, pressureX, cpx);
		v = getVelocity(temperatureK, pressureY, cpy);
		w = getVelocity(temperatureK, pressureZ, cpz);

		velocityResidual = getVelocityResidual();

	} while (velocityResidual > velocityTolerance && iters < maxIters);

	//rotate to "north" coordinate system....  someday....?  Currently stay in local sensor coordinate system


	if (velocityResidual > velocityTolerance)
		return false;
	else
		return true;
}

double convertVelocity::getVelocity(double &temperature, double &pressure, double &cp)
{
	if (cp <= 0.1)	//cp of zero means velocity must be zero.  Negative cp may be possible, if the cp(angle,Re) gives that for high angles and low Re.
		return 0.0;
	if(pressure > 0)	//make sure pressure is positive for square root
		return sqrt(pressure / (0.5*air.get_rho(temperature)*cp));
//		return sqrt(pressure / (0.5*1.2*cp));
	else
		return -sqrt(-pressure / (0.5*air.get_rho(temperature)*cp));
//		return -sqrt(-pressure / (0.5*1.2*cp));
}

double convertVelocity::getVelocityResidual()
{
	//compute the magnitude of the difference vector
	return sqrt((u - uOld)*(u - uOld) + (v - vOld)*(v - vOld) + (w - wOld)*(w - wOld));
}

double convertVelocity::getCp(double angle, double ReynoldsNumber, const std::vector<std::vector<double>> &lookupTablePressureCoefficient, const std::vector<double> &lookupTableAngle, const std::vector<double> &lookupTableReynoldsNumber)
{
	//if (angle <= 90.0)
	//	return -1.0/90.0 * angle + 1.0;
	//else
	//	return -1.0 / 90.0 * (180.0 - angle) + 1.0;

	if (angle > 90.0)	//calibration data is only good for 0-90, cp doesn't care about sign of wind (at least here)
		angle = 180.0 - angle;
	if (ReynoldsNumber < 0.0)
		ReynoldsNumber = -1.0 * ReynoldsNumber;

	double CpValue;
	//CpValue = 1.099 + 9.311e-03*angle - 2.424e-04*angle*angle + 7.536e-07*ReynoldsNumber - 7.091e-13*ReynoldsNumber*ReynoldsNumber - 3.061e-09*angle*ReynoldsNumber;  //old conversion which had an error where Re number was incorrectly computed
	//CpValue = -1.591e-01*exp(2.702e-02*angle) + 2.490e+00 + 1.010e-05*ReynoldsNumber - 1.0;	//Curve fit from Isaac, not great

	int row = -1;
	int col = -1;
	for (int i = 0; i < lookupTableAngle.size(); i++)
	{
		if (angle < lookupTableAngle[i])
		{
			row = i - 1;
			break;
		}
	}

	for (int i = 0; i < lookupTableReynoldsNumber.size(); i++)
	{
		if (ReynoldsNumber < lookupTableReynoldsNumber[i])
		{
			col = i - 1;
			break;
		}
	}

	//handle case where values are equal to high side of arrays
	if (row < 0)	//if wasn't set above
		row = (int)lookupTableAngle.size() - 2;
	if (col < 0)	//if wasn't set above
		col = (int)lookupTableReynoldsNumber.size() - 2;

	double t, u;

	t = (angle - lookupTableAngle[row]) / (lookupTableAngle[row+1] - lookupTableAngle[row]);
	u = (ReynoldsNumber - lookupTableReynoldsNumber[col]) / (lookupTableReynoldsNumber[col + 1] - lookupTableReynoldsNumber[col]);

	CpValue = (1.0 - t) * (1.0 - u) * lookupTablePressureCoefficient[row][col] + t * (1.0 - u)*lookupTablePressureCoefficient[row][col + 1] + t * u*lookupTablePressureCoefficient[row + 1][col + 1] + (1.0 - t)*u*lookupTablePressureCoefficient[row + 1][col];

	if (CpValue < 0.0)	//Don't let Cp go less than zero, which can happen with the current curve fit near angles of 90.
		CpValue = 0.0;
	return CpValue;
}
