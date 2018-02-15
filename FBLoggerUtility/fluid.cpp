#include "stdafx.h"
#include "fluid.h"

Fluid::Fluid()
{
}

Fluid::~Fluid()
{
    if(t)
        delete[] t;
    if(rho)
        delete[] rho;
    if(cSubP)
        delete[] cSubP;
    if(mu)
        delete[] mu;
    if(v)
        delete[] v;
    if(k)
        delete[] k;
    if(alpha)
        delete[] alpha;
    if(pr)
        delete[] pr;
}

bool Fluid::read_fluidProperties(string inputFile)
{
    fstream fin;
    fin.open(inputFile.c_str(),ios::in);
    char testString[32] = "";
    char krap[32] = "";
    if(!fin)
    {
        cout << "Cannot open *.prp file.";
        fin.close();
        return false;
    }
    else
    {
        fin >> name >> krap >> nRows >> testString >> krap;
        t = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> t[i];
        }
        fin >> krap >> krap;
        rho = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> rho[i];
        }
        fin >> krap >> krap;
        cSubP = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> cSubP[i];
        }
        fin >> krap >> krap;
        mu = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> mu[i];
        }
        fin >> krap >> krap;
        v = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> v[i];
        }
        fin >> krap >> krap;
        k = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> k[i];
        }
        fin >> krap >> krap;
        alpha = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> alpha[i];
        }
        fin >> krap >> krap;
        pr = new double[nRows];
        for(int i = 0;i < nRows;i++)
        {
            fin >> pr[i];
        }
        fin.close();
        return true;
    }
}
double Fluid::get_rho(double T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(rho[i] - rho[i - 1])) + rho[i - 1];
    }
}
double Fluid::get_cSubP(double T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(cSubP[i] - cSubP[i - 1])) + cSubP[i - 1];
    }
}
double Fluid::get_mu(double T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(mu[i] - mu[i - 1])) + mu[i - 1];
    }
}
double Fluid::get_v(double T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(v[i] - v[i - 1])) + v[i - 1];
    }
}
double Fluid::get_k(double T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(k[i] - k[i - 1])) + k[i - 1];
    }
}
double Fluid::get_alpha(double  T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(alpha[i] - alpha[i - 1])) + alpha[i - 1];
    }
}
double Fluid::get_pr(double  T)
{
    if(T > t[nRows - 1])
    {
        cout << "Invalid temperature: " << T << "\n";
        return -1;
    }
    else
    {
        int i = 0;
        while(T > t[i])
            i++;
        return (((T - t[i - 1])/(t[i] - t[i - 1]))*(pr[i] - pr[i - 1])) + pr[i - 1];
    }
}
bool Fluid::print_t()
{
    cout << endl << "t" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << t[i] << endl;
    }
    return true;
}
bool Fluid::print_rho()
{
    cout << endl << "rho" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << rho[i] << endl;
    }
    return true;
}
bool Fluid::print_cSubP()
{
    cout << endl << "cSubP" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << cSubP[i] << endl;
    }
    return true;
}
bool Fluid::print_mu()
{
    cout << endl << "mu" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << mu[i] << endl;
    }
    return true;
}
bool Fluid::print_v()
{
    cout << endl << "v" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << v[i] << endl;
    }
    return true;
}
bool Fluid::print_k()
{
    cout << endl << "k" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << k[i] << endl;
    }
    return true;
}
bool Fluid::print_alpha()
{
    cout << endl << "alpha" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << alpha[i] << endl;
    }
    return true;
}
bool Fluid::print_pr()
{
    cout << endl << "pr" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << pr[i] << endl;
    }
    return true;
}
bool Fluid::print_table()
{
    cout << name << endl <<"Temp\trho\tcSubP\tmu\tv\tk\talpha\tpr" << endl;
    for(int i = 0;i < nRows;i++)
    {
        cout << t[i] << "\t" << rho[i] << "\t" << cSubP[i] << "\t" << mu[i] << "\t" << v[i] << "\t" << k[i] << "\t" << alpha[i] << "\t" << pr[i] << endl;
    }
    return true;
}
