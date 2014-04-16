#pragma once

#include <math.h>
#include <stdlib.h>

const float TAU = M_PI * 2.0f;
const float PI8 = M_PI / 8 / 2; //pi/8/2
const float PI38 = 3 * PI8; //3pi/8/2
const float PI4 = M_PI / 4;

//uniform random in [a,b)
inline float randf(float a, float b)
{
    return ((b - a) * ((float)rand() / RAND_MAX)) + a;
}

//uniform random int in [a,b)
inline int randi(int a, int b)
{
    return (rand() % (b - a)) + a;
}

//normalvariate random N(mu, sigma)
inline double randn(double mu, double sigma)
{
    static bool deviateAvailable = false; // flag
    static float storedDeviate; // deviate from previous calculation
    if (!deviateAvailable)
    {
        double var1 = 0.0, var2 = 0.0;
        double rsquared = 0.0;
        do
        {
            var1 = 2.0 * (double(rand()) / double(RAND_MAX)) - 1.0;
            var2 = 2.0 * (double(rand()) / double(RAND_MAX)) - 1.0;
            rsquared = var1 * var1 + var2 * var2;
        } while (rsquared >= 1.0 || rsquared == 0.0);
        double polar = sqrt(-2.0 * log(rsquared) / rsquared);
        storedDeviate = var1 * polar;
        deviateAvailable = true;
        return var2 * polar * sigma + mu;
    }
    else
    {
        deviateAvailable = false;
        return storedDeviate * sigma + mu;
    }
}

template<class T>
inline void CeilingCap(T *val, const T &max)
{
    if (*val > max)
        *val = max;
}

template<class T>
inline void FloorCap(T *val, const T &min)
{
    if (*val < min)
        *val = min;
}

template<class T>
inline void Clamp(T *val, const T &min, const T &max)
{
    if (*val < min)
        *val = min;
    else if (*val > max)
        *val = max;
}

template<class T>
inline T Clamp(const T &val, const T &min, const T &max)
{
    if (val < min)
        return min;
    else if (val > max)
        return max;

    return val;
}
