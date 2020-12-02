#include <stdio.h>

#include <math.h>


#define N 50

#define PI 3.1415926

int main()
{
	double T[N], A = 1.0, F0 = 1/50.0, Phi = 0.0;

	int i = 0;

	for (i = 0;i < N; i++){

		T[i] = A * sin(2*PI*F0*i + Phi);

	}

	for (i = 0;i < N; i++){

		printf("%lf,", T[i]);

	}

	return 0;
}
