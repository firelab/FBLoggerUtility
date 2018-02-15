#pragma once

#include <string>
#include <iostream>
#include <fstream>

using namespace std;

class Fluid
{
public:
	Fluid();
	~Fluid();
	bool read_fluidProperties(string inputFile);

	double get_cSubP(double);
	double get_rho(double);
	double get_mu(double);
	double get_v(double);
	double get_k(double);
	double get_alpha(double);
	double get_pr(double);

	bool print_name();
	bool print_t();
	bool print_cSubP();
	bool print_rho();
	bool print_mu();
	bool print_v();
	bool print_k();
	bool print_alpha();
	bool print_pr();
	bool print_table();

	string name;
	int nRows;
	double *t;
	double *rho;
	double *cSubP;
	double *mu;
	double *v;
	double *k;
	double *alpha;
	double *pr;
};
