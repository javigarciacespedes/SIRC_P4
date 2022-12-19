#ifndef RANDOM_H
#define RANDOM_H

#include "rngstream.h"

class Rndgen {
    public:
      virtual double val () =0;
};

typedef Rndgen* RND_GEN;

// devuelve el siguiente valor
// de un generador pseudoaleatorio de tipo RND_GEN
#define SIG_VAL(V) V->val()

// distribucion uniforme entre A y B
class Uniform : public Rndgen, RngStream {
	double A, B;
	public:
		Uniform (double min, double max);
		double val ();
};

#define INIT_UNIFORM(A,B) new Uniform (A,B)

// distribucion exponencial negativa de tasa A (media=1/A)
class NegExp : public Rndgen, RngStream {
	double A;
	public:
		NegExp (double tasa);
		double val ();
};

#define INIT_NEGEXP(A) new NegExp (A)

// distribucion Erlang-K con media 1/A y K=B (varianza 1/(A*A*K)
// igual a suma de K exponenciales negativas de tasa K*A y media 1/(A*K)
class ErlangK : public Rndgen, RngStream {
	double A; double B;
	public:
		ErlangK (double tasa, double K);
		double val ();
};

#define INIT_ERLANGK(A,B) new ErlangK (A,B)

// distribucion normal de media A y desviacion estandar B
class Normal : public Rndgen, RngStream {
	double A, B;
	public:
		Normal (double media, double stddev);
		double val ();
};

#define INIT_NORMAL(A,B) new Normal (A,B)

// valor 1 con probabilidad A y 0 con probabilidad 1-A
class Bernoulli : public Rndgen, RngStream {
	double A;
	public:
		Bernoulli (double p);
		double val ();
};

#define INIT_BERNOULLI(A) new Bernoulli (A)

// valores A A+1 ... B-1 B equiprobables
class UniformD : public Rndgen, RngStream {
	int A, B;
	public:
		UniformD (int min, int max);
		double val ();
};

#define INIT_UNIFORM_D(A,B) new UniformD (A,B)



/*
long Poisson(double A, long &U);
// Drawing from the Poisson distribution with parameter A.

long Discrete(double A[], long N, long &U);
// The one-dimensional array A of N elements of type double, augmented
// by the element 1 to the right, is interpreted as a step function
// of the subscript, defining a discrete (cumulative) distribution function.
// The function value satisfies
//   0 <= Discrete(...) <= N
// It is defined as the smallest i such that A[i] > r, where r is a random
// number in the interval [0;1] and A[N] = 1.

double Linear(double A[], double B[], long N, long &U);
// The value is a drawing from a (cumulative) distribution function f,
// which is obtained by linear interpolation in a non-equidistant table defined
// by A and B, such that A[i] = f(B[i]).
// It is assumed that A and B are one-dimensional arrays of the same
// length, N, that the first and last elements of A are equal to 0 and 1,
// respectively, and that A[i] ³ A[j] and B[i] > B[j] for i > j.

long Histd(double A[], long N, long &U);
// The value is an integer in the range [0;N-1] where N is the number of
// elements in the one-dimensional array A. The latter is interpreted as a
// histogram defining the relative frequencies of the values.
*/


#endif
