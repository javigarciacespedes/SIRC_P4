#include "random.h"
#include <iostream>
using namespace std;
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static void Error(const char *txt) {
  cerr << "***Error*** " << txt << endl;
  exit(0);
}

// distribucion uuniforme entre A y B
Uniform::Uniform (double min, double max) {
	if (max < min) Error ("Distribucion uniforme con max < min");
	A = min;
	B = max;
}

double Uniform::val () {
	return RandU01()*(B-A)+A;
}

// distribucion exponencial negativa de tasa A (media=1/A)
NegExp::NegExp (double tasa) {
	if (tasa < 0) Error ("Distribucion exponencial negativa con tasa < 0");
	A = tasa;
}

double NegExp::val () {
	return -log(RandU01())/A;
}

// distribucion Erlang-K con media 1/A y K=B (varianza 1/(A*A*K)
// igual a suma de K exponenciales negativas de tasa K*A y media 1/(A*K)
ErlangK::ErlangK (double tasa, double K) {
	if (tasa <= 0) Error ("Distribucion ErlangK con tasa <= 0");
	if (K <= 0) Error ("Distribucion ErlangK con K <= 0");
	A = tasa;
	B = K;
}

double ErlangK::val () {
    long Bi = (long) B, i;
    if (Bi == B)
        Bi--;
    double Sum = 0;
    for (i = 1; i <= Bi; i++)
        Sum += log(RandU01());
    return ( -(Sum + (B-(i-1))*log(RandU01())) / (A*B) );
}

// distribucion normal de media A y desviacion estandar B
Normal::Normal (double media, double stddev) {
	if (stddev < 0) Error ("Distribucion Normal con stddev < 0");
	A = media;
	B = stddev;
}

#define p0 (-0.322232431088)
#define p1 (-1)
#define p2 (-0.342242088547)
#define p3 (-0.0204231210245)
#define p4 (-0.0000453642210148)
#define q0 0.099348462606
#define q1 0.588581570495
#define q2 0.531103462366
#define q3 0.10353775285
#define q4 0.0038560700634

double Normal::val () {
    double y, x, p, R = RandU01();
    p = R > 0.5 ? 1.0 - R : R;
    y = sqrt (-log (p * p));
    x = y + ((((y * p4 + p3) * y + p2) * y + p1) * y + p0) /
            ((((y * q4 + q3) * y + q2) * y + q1) * y + q0);
    if (R < 0.5)
        x = -x;
    return B * x + A;
}

// valor entero 1 con probabilidad A y 0 con probabilidad 1-A
Bernoulli::Bernoulli (double p) {
	if (p < 0) Error ("Probabilidad < 0");
	if (p > 1) Error ("Probabilidad > 1");
	A = p;
}

double Bernoulli::val () {
	return ( RandU01() < A );
}

// valores enteros A A+1 ... B-1 B equiprobables
UniformD::UniformD (int min, int max) {
	if (max < min) Error ("Distribucion uniforme con con max < min");
	A = min;
	B = max;
}

double UniformD::val () {
	return ( RandU01()*(B-A+1)+A );
}

/*

long Poisson(double A, long &U) {
    double Limit = exp(-A), Prod = NextU;
    long n;
    for (n = 0; Prod >= Limit; n++)
        Prod *= Random;
    return n;
}


long Discrete(double A[], long N, long &U) {
    double Basic = Random;
    long i;
    for (i = 0; i < N; i++)
        if (A[i] > Basic)
            break;
    return i;
}

double Linear(double A[], double B[], long N, long &U) {
    if (A[0] != 0.0 || A[N-1] != 1.0)
        Error("Linear: Illegal value in first array");
    double Basic = Random;
    long i;
    for (i = 1; i < N; i++)
        if (A[i] >= Basic)
            break;
    double D = A[i] - A[i-1];
    if (D == 0.0)
        return B[i-1];
    return B[i-1] + (B[i]-B[i-1])*(Basic-A[i-1])/D;
}

long Histd(double A[], long N, long &U) {
    double Sum = 0.0;
    long i;
    for (i = 0; i < N; i++)
        Sum += A[i];
    double Weight = Random * Sum;
    Sum = 0.0;
    for (i = 0; i < N - 1; i++) {
        Sum += A[i];
        if (Sum >= Weight)
            break;
    }
    return i;
}

*/


