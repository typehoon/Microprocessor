/*
 * Filters.h
 *
 *  Updated on: 25/04/2019
 *      by: Laborda
 *  Created on: 14 de jun. de 2016
 *      Author: guerr
 */

#ifndef FILTERS_H_
#define FILTERS_H_

#include <math.h>

#define PI (3.141592653589793)
#define TWOPI (6.283185307179586)
#define TWOPIDIV3 (2.094395102393195)
#define PIDIV2 (1.570796326794897)
#define PIDIV3 (1.047197551196598)
#define SQRT2 (1.414213562373095)
#define SQRT3DIV2 (0.866025403784439)
#define SQRT3DIV3 (0.577350269189626)
#define TWOSQRT3DIV3 (1.154700538379251)

#define N    0
#define N1   1
#define N2	 2

#define DTDIV2  100e-6  // TS/2

//---------------------------------- MACROS---------------------------------------------------
#define INIT1(var,value) *var = *(var+1) = (value) // Initialize to a value a 2 element vector
#define SATURATE(var, max,min) \
                            do { (var) = ((var) > (max) ? (max) : (var));\
                                (var) = ((var) < (min) ? (min) : (var));\
                            } while(0)
#define SHIFT1(var) *(var+1) = *var  // Shifts a 2 element vector (order 1)
#define SHIFT2(var) *(var+2) = *(var+1); *(var+1) = *var;

//---------------------------------- FUNCTIONS---------------------------------------------------

/* ***********************************************************************************************************************
 * Function name: Filter
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* input: pointer to the vector containing last n-samples of the input signal (shifted)
 *      float* output: pointer to the vector containing last n-samples of the output signal (shifted)
 *      float* num_coefs: pointer to a vector with the numerator coefficients [b0, b1, ..., bn]
 *      float* den_coefs: pointer to a vector with the numerator coefficients [1, a1, ..., an]. a0 = 1 always.
 *      const int order: filter order
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: N order filter (b0+b1*z^-1+...+bn*z^-n)/(1+ a1*z^-1+...+an*z^-n)
 *  Note 1: It is recommended to scale up the coefficient values if they are very small
 *          The output must be scaled down accordingly
 *  Note 2: The input and output vectors must be shifted before calling this function
 ****************************************************************************************************************************/
static inline void Filter(double input[], double output[], const double num_coefs[], const double den_coefs[], const int order)
{
     register int index;

     // Filter calculation
     output[N] = (num_coefs[N]*input[N]);

     for(index = 1; index <= order; index++)
         output[N]+= num_coefs[index] * input[index];

     for(index = 1; index <= order; index++)
         output[N]-= den_coefs[index] * output[index];

}

/* ***********************************************************************************************************************
 * Function name: Filter_rr
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* input: pointer to the vector containing last n-samples of the input signal (shifted)
 *      float* output: pointer to the vector containing last n-samples of the output signal (shifted)
 *      float* num_coefs: pointer to a vector with the numerator coefficients [b0, b1, ..., bn]
 *      float* den_coefs: pointer to a vector with the numerator coefficients [1, a1, ..., an]. a0 = 1 always.
 *      const int order: filter order
 *      const float max: saturation upper limit
 *      const float min: saturation lower limit
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: N order filter (b0+b1*z^-1+...+bn*z^-n)/(1+ a1*z^-1+...+an*z^-n) with realizable references
 *  Note 1: It is recommended to scale up the coefficient values if they are very small
 *          The output must be scaled down accordingly
 *  Note 2: The input and output vectors must be shifted before calling this function
 ****************************************************************************************************************************/
static inline void Filter_rr(float input[], float output[], const float num_coefs[], const float den_coefs[], const int order, const float max, const float min)
{
    register int index;

    // Filter calculation
    output[N] = (num_coefs[N]*input[N]);

    for(index = 1; index <= order; index++)
        output[N]+= num_coefs[index] * input[index];

    for(index = 1; index <= order; index++)
        output[N]-= den_coefs[index] * output[index];

     // Saturation
     if (output[N] > max)
     {
         input[N] += (max - output[N])/(num_coefs[N]);
         output[N] = max;
     }
     else if (output[N] < min)
     {
         input[N] += (min - output[N])/(num_coefs[N]);
         output[N] = min;
     }
}

/* ***********************************************************************************************************************
 * Function name: Filter_ff_rr
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* input: pointer to the vector containing last n-samples of the input signal (shifted)
 *      float* output: pointer to the vector containing last n-samples of the output signal (shifted)
 *      float* num_coefs: pointer to a vector with the numerator coefficients [b0, b1, ..., bn]
 *      float* den_coefs: pointer to a vector with the numerator coefficients [1, a1, ..., an]. a0 = 1 always.
 *      const int order: filter order
 *      const float max: saturation upper limit
 *      const float min: saturation lower limit
 *      float feedforward: desired feedforward value to add to the output
 *
 *.***************************************************
 * Output parameters:
 *
 *      float output_plus_ff: filtered output value (current sample) with feedforward term added (output[N]+feedforward)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: N order filter (b0+b1*z^-1+...+bn*z^-n)/(1+ a1*z^-1+...+an*z^-n) with realizable references and feedforward term summed to the ouput
 *  Note 1: It is recommended to scale up the coefficient values if they are very small
 *          The output must be scaled down accordingly
 *  Note 2: The input and output vectors must be shifted before calling this function
 ****************************************************************************************************************************/
static inline float Filter_ff_rr(float input[], float output[], const float num_coefs[], const float den_coefs[], const int order, const float max, const float min, float feedforward)
{
    register int index;
    float output_plus_ff;

    // Filter calculation
    output[N] = (num_coefs[N]*input[N]);

    for(index = 1; index <= order; index++)
        output[N]+= num_coefs[index] * input[index];

    for(index = 1; index <= order; index++)
        output[N]-= den_coefs[index] * output[index];

     // Feedforward term addition
     output_plus_ff = output[N] + feedforward;

     // Saturation
     if (output_plus_ff > max)
     {
         output_plus_ff = max;
         input[N] += (max - feedforward - output[N])/(num_coefs[N]);
         output[N] = max -feedforward;
     }
     else if (output_plus_ff < min)
     {
         output_plus_ff = min;
         input[N] += (min -feedforward - output[N])/(num_coefs[N]);
         output[N] = min -feedforward;
     }

    return(output_plus_ff);
}

/* ***********************************************************************************************************************
 * Function name: Filter1
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* input: pointer to the vector containing last 2 samples of the input signal (shifted)
 *      float* output: filtered output vector (last 2 samples of the output signal (shifted))
 *      const float* num_coefs: vector with B0 and B1 parameters of the discrete filter
 *      const float* den_coefs: vector with A0=1 and A1 parameter of the discrete filter
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: First order filter G(z) = (b0+b1*z^-1)/(1+ a1*z^-1)
 *  Note 1: It is recommended to scale up the coefficient values if they are very small
 *          The output must be scaled down accordingly
 *  Note 2: The input and output vectors must be shifted before calling this function
 ****************************************************************************************************************************/
static inline void Filter1(double input[], double output[], const double num_coefs[], const double den_coefs[])
{
    output[N] = (num_coefs[N])*(input[N]) + (num_coefs[N1])*(input[N1]) - (den_coefs[N1])*(output[N1]);
}

/* ***********************************************************************************************************************
 * Function name: PI_Reg
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* error:  pointer to the vector containing last 2 samples of the input error signal (shifted)
 *      float* control_action: pointer to the vector containing last 2 samples of the output control action signal (shifted)
 *      const float* num_coefs: pointer to a vector with the numerator coefficients [b0, b1] (take a look to the guide about how to
 *                        calculate them from Kp and Ki)
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: PI regulator implementation for DQ struct data with asymmetries
 *  Note 1: It is recommended to scale up the coefficient values if they are very small
 *          The output must be scaled down accordingly
 *  Note 2: The input and output vectors must be shifted before calling this function
 ****************************************************************************************************************************/
static inline void PI_Reg(float error[], float control_action[], const float num_coefs[])
{
    control_action[N] += num_coefs[N]*error[N] + num_coefs[N1]*error[N1];
}

/* ***********************************************************************************************************************
 * Function name: PI_rr
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* error:  pointer to the vector containing last 2 samples of the input error signal (shifted)
 *      float* control_action: pointer to the vector containing last 2 samples of the output control action signal (shifted)
 *      const float* num_coefs: pointer to a vector with the numerator coefficients [b0, b1] (take a look to the guide about how to
 *                        calculate them from Kp and Ki)
*       const float max: maximum allowable value for the output
*       const float min: minimum allowable value for the output
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: PI (Tustin) controller with realizable references G(z) = (b0+b1*z^-1)/(1 - z^-1)
 *  Note 1: It is recommended to scale up the coefficient values if they are very small
 *          The output must be scaled down accordingly
 *  Note 2: The input and output vectors must be shifted before calling this function
 ****************************************************************************************************************************/
static inline void PI_rr(float error[], float control_action[], const float num_coefs[], const float max, const float min)
{

   // PI calculation
    control_action[N] += num_coefs[N]*error[N] + num_coefs[N1]*error[N1];

    // Saturation
   if (control_action[N] > max)
   {
       error[N] += (max - control_action[N])/(num_coefs[N]);
       control_action[N] = max;
   }
   else if (control_action[N] < min)
   {
       error[N] += (min - control_action[N])/(num_coefs[N]);
       control_action[N] = min;
   }

}

/* ***********************************************************************************************************************
 * Function name: Shiftn
 * ***************************************************
 * Author: JuanMa
 * ***************************************************
 * Input parameters:
 *
 *      float* vector: pointer to a vector to be shifted
 *      int order: order of the filter that vector is used in. Therefore order = vector_size-1
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 * N and N1 defined in "Constants.h"
 *
 ******************************************************
 *  Description: Shifts a vector one position to the right (first elemenent is not updated)
 *  Coding note: update loop must be always done in reverse order
 ****************************************************************************************************************************/
static inline void Shiftn(float vector[], int order)
{
    register int index;
    for (index = order; index > 0; index--)   // index: auxiliar variable for for loop
        vector[index] = vector[index-1];
}

/* ***********************************************************************************************************************
 * Function name: Wrap2_2PI
 * ***************************************************
 * Author: Laborda, copied from macro implementation (JuanMa)
 * ***************************************************
 * Input parameters:
 *
 *      float* var: pointer to the variable to be fit into the [0 2pi] interval
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 *      none
 *
 ******************************************************
 *  Description: wraps var to the [0 2pi] interval
 ****************************************************************************************************************************/
static inline void Wrap2_2PI(float *var)
{
   do { (*var) = ((*var) > TWOPI) ? ((*var) - TWOPI) : (*var);
        (*var) = ((*var) < 0) ?  ((*var) + TWOPI) : (*var);
   } while(((*var) > TWOPI) || ((*var) < 0));
}

/* ***********************************************************************************************************************
 * Function name: Wrap2_PIPI
 * ***************************************************
 * Author: Laborda, copied from macro implementation (JuanMa)
 * ***************************************************
 * Input parameters:
 *
 *      float* var: pointer to the variable to be fit into the [-pi pi] interval
 *
 *.***************************************************
 * Output parameters:
 *
 *      none (void)
 *
 ******************************************************
 * Global variable dependences:
 *
 *      none
 *
 ******************************************************
 *  Description: wraps var to the [-pi pi] interval
 ****************************************************************************************************************************/
static inline void Wrap2_PIPI(float *var)
{
   do { (*var) = ((*var) > PI) ? ((*var) - TWOPI) : (*var);
        (*var) = ((*var) < -PI) ?  ((*var) + TWOPI) : (*var);
   } while(((*var) > PI) || ((*var) < -PI));
}



#endif /* FILTERS_H_ */
