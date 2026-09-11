#include "uebpgdecls.h"

// js  Constant  float set
const float T_0 = 0.0; // Temperature of freezing (0 C)
const float T_k = 273.15; // Temperature to convert C to K (273.15)
const float SB_c =
    2.041334e-7; // Stefan boltzman constant (2.041334e-7 KJ/m^2-hr-K^4) #corrected 12.23.14
const float H_f    = 333.5; // Heat of fusion (333.5 KJ= kg)
const float Hne_u  = 2834.0; // Heat of Vaporization (Ice to Vapor, 2834 KJ= kg)
const float C_w    = 4.18; // Water Heat Capacity (4.18 KJ/ kg-C)
const float C_s    = 2.09; // Ice heat capacity (2.09 KJ= kg= C)
const float C_p    = 1.005; // Air Heat Capacity (1.005 KJ= kg= K)
const float Ra_g   = 287.0; // Ideal Gas constant for dry air (287 J= kg= K)
const float K_vc   = 0.4; // Von Karmans constant (0.4)
const float Hs_f   = 3600.0; // Factor to convert = s into = hr (3600)
const float Rho_i  = 917.0; // Density of Ice (917 kg= m^3)
const float Rho_w  = 1000.0; // Density of Water (1000 kg= m^3)
const float Gra_v  = 9.81; // Gravitational acceleration (9.81 m= s^2)
const float W1da_y = 0.261799; // Daily frequency (2pi= 24 hr 0.261799 radians= hr)
const float Io     = 4914.0; //  Solar constant  Kj/m^2/hr
// pi copied from snowdxv.f90
const float P_i = 3.141592653589793238462643383279502884197169399375105820974944592308; // Pi

// data for pred-corr
const float wtol = 0.025;
const float utol = 2000.0;
// from TURBFLUX()
const float tol     = 0.001;
const int nitermax  = 20;
const int ncitermax = 21;
// flag to write warnings,...etc

// Global variables
// ###_TBC 5.6.13
int snowdgtvariteflag  = 0;
int snowdgtvariteflag2 = 0; // 0;
int snowdgtvariteflag3 = 0;
int snowdgt_outflag    = 0;
int radwarnflag        = 0;
int inpDailyorSubdaily = 1; // 0: values given at each (sub-daily time steps); 1: daily values
int uebCellX           = 0;
int uebCellY           = 0;

// common
float Tsk_save = 273.16, Tssk_old = 273.16, Tsavek_old = 273.16, Tsavek_ave = 273.16,
      Tssk_ave = 273.16 /*added 6.7.13*/;

float findMax(float a, float b) {
    return (a > b) ? a : b;
}

float findMin(float a, float b) {
    return (a < b) ? a : b;
}

int findMax(int a, int b) {
    return (a > b) ? a : b;
}

int findMin(int a, int b) {
    return (a < b) ? a : b;
}
