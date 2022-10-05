#pragma once
#ifndef GLOBAL_H
#define GLOBAL_H

#include <cmath>
#include <limits>
#include <memory>
#include <thread>
#include <random>

// Usings

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;
const double pi2 = 6.283185307179586476925286766559;
const double pi_inv = 0.3183098861837907;
const double pi2_inv = 0.1591549430918953357;

// Utility Functions

inline double degrees_to_radians(double degrees) {
	return degrees * pi / 180.0;
}

// Random
std::default_random_engine e1;
std::uniform_real_distribution<double> u1(0.0, 1.0);
inline double random_double() {
	return u1(e1);
}

inline double random_double(double min, double max) {
	// Returns a random real in [min,max).
	return min + (max - min)*random_double();
}

std::default_random_engine e2;
std::uniform_int_distribution<unsigned> u2(0, 2);
inline int random_int_012() {
	return u2(e2);
}

// Clamp

inline double clamp(double x, double min, double max) {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

// disney brdf functions

double sqr(double x) { return x * x; }

double SchlickFresnel(double u) {
    double m = clamp(1 - u, 0, 1);
    double m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

double GTR1(double NdotH, double a) {
    if (a >= 1) return 1 / pi;
    double a2 = a * a;
    double t = 1 + (a2 - 1) * NdotH * NdotH;
    return (a2 - 1) / (pi * log(a2) * t);
}

double GTR2(double NdotH, double a) {
    double a2 = a * a;
    double t = 1 + (a2 - 1) * NdotH * NdotH;
    return a2 / (pi * t * t);
}

double GTR2_aniso(double NdotH, double HdotX, double HdotY, double ax, double ay) {
    return 1 / (pi * ax * ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}

double smithG_GGX(double NdotV, double alphaG) {
    double a = alphaG * alphaG;
    double b = NdotV * NdotV;
    return 1 / (NdotV + sqrt(a + b - a * b));
}

double smithG_GGX_aniso(double NdotV, double VdotX, double VdotY, double ax, double ay) {
    return 1.0 / (NdotV + sqrt(sqr(VdotX * ax) + sqr(VdotY * ay) + sqr(NdotV)));
}




// Common Headers

#include "ray.h"
#include "vec3.h"


// Test
//#define test_mode
bool sample_light_flag;

#endif