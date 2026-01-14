/* FIlter effect example
* Date: 10/05/2022
* Modified by (list):
* Ye gu Kang
*/

#include <math.h>
#include "microcontroller.h"
#include "Filters.h"


// Global variables
double f_cutoff; // Cut-off frequency
double Sig_input[2]; // Input
double Sig_out_HPF[2]; // 1st order HPF
double Sig_out_LPF[2]; // 1st order LPF
double Sig_out_APF[2]; // 1st order APF
double Sig_out_HPF2[2]; // 2nd order HPF
double Sig_out_LPF2[2]; // 2nd order LPF
double Sig_out_BPF[2]; // 2nd order BPF
double Sig_out_BSF[2]; // 2nd order BSF
double LPF_num_coefs[2];
double LPF_den_coefs[2];
double HPF_num_coefs[2];
double HPF_den_coefs[2];

void init_software(void)
{
    // Init. all filters
    for (int i = 0; i < 2; i++){
        Sig_input[i] = 0;
        Sig_out_HPF[i] = Sig_out_LPF[i] = Sig_out_APF[i] = 0;
        Sig_out_HPF2[i] = Sig_out_LPF2[i] = 0;
        Sig_out_BPF[i] = Sig_out_BSF[i] = 0;
    }

    // Init. HPF coef.
    HPF_num_coefs[0] = 0.969531252908746;
    HPF_num_coefs[1] = -0.969531252908746;
    HPF_den_coefs[0] = 1.0;
    HPF_den_coefs[1] = -0.939062505817492;

    // Init. LPF coef.
    LPF_num_coefs[0]=  0.030468747091254;
    LPF_num_coefs[1]=  0.030468747091254;
    LPF_den_coefs[0] = 1.0;
    LPF_den_coefs[1] = -0.939062505817492;

}



void microcontroller(const double * adc, double * dac)
{

    SHIFT1(Sig_input);

    // Read inputs   (Replace in hardware)
    Sig_input[0] = (double) *(adc);

    // 1st order LPF
    /*
    Sig_out_LPF[1] = Sig_out_LPF[0];
    Sig_out_LPF[0] = (LPF_num_coefs[0])*(Sig_input[0]) + (LPF_num_coefs[1])*(Sig_input[1]) - (LPF_den_coefs[1])*(Sig_out_LPF[1]);
    */
    SHIFT1(Sig_out_LPF);
    Filter(Sig_input, Sig_out_LPF, LPF_num_coefs, LPF_den_coefs,1);

    // 1st order HPF
    /*
    Sig_out_HPF[1] = Sig_out_HPF[0];
    Sig_out_HPF[0] = (HPF_num_coefs[0])*(Sig_input[0]) + (HPF_num_coefs[1])*(Sig_input[1]) - (HPF_den_coefs[1])*(Sig_out_HPF[1]);
    */
    SHIFT1(Sig_out_HPF);
    Filter(Sig_input, Sig_out_HPF, HPF_num_coefs, HPF_den_coefs,1);

    // 1st order APF
    SHIFT1(Sig_out_APF);
    Sig_out_APF[0] = Sig_out_HPF[0] - Sig_out_LPF[0];

    // 2nd order LPF
    SHIFT1(Sig_out_LPF2);
    Filter(Sig_out_LPF, Sig_out_LPF2, LPF_num_coefs, LPF_den_coefs, 1);

    // 2nd order HPF
    SHIFT1(Sig_out_HPF2);
    Filter(Sig_out_HPF, Sig_out_HPF2, HPF_num_coefs, HPF_den_coefs, 1);

    // 2nd order BPF
    SHIFT1(Sig_out_BPF);
    Filter(Sig_out_LPF, Sig_out_BPF, HPF_num_coefs, HPF_den_coefs, 1);

    // 2nd order BSF
    SHIFT1(Sig_out_BSF);
    Sig_out_BSF[0] = Sig_out_LPF2[0] + Sig_out_HPF2[0];

    // Write output
    dac[0] = (double) Sig_input[0];
    dac[1] = (double) Sig_out_HPF[0];
    dac[2] = (double) Sig_out_LPF[0];
    dac[3] = (double) Sig_out_APF[0];
    dac[4] = (double) Sig_out_HPF2[0];
    dac[5] = (double) Sig_out_LPF2[0];
    dac[6] = (double) Sig_out_BPF[0] * 2.0;
    dac[7] = (double) Sig_out_BSF[0];

}

