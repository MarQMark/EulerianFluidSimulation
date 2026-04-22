#include <stdio.h>
#include <string.h>

#include "LinearSolver.hpp"

#include "simulation/ScalField.h"
#include "simulation/VecField.h"


/** Vector helper functions */
void VecAddEqual(int n, float r[], const float v[]){
	for (int i = 0; i < n; i++)
		r[i] = r[i] + v[i];
}

void VecDiffEqual(int n, float r[], const float v[]){
	for (int i = 0; i < n; i++)
		r[i] = r[i] - v[i];
}

void VecAssign(int n, float v1[], const float v2[]){
	memcpy(v1, v2, n * sizeof(float));
}

void VecTimesScalar(int n, float v[], double s) {
	for (int i = 0; i < n; i++)
		v[i] *= s;
}

double VecDot(int n, const float v1[], const float v2[]) {
	double dot = 0;
	for (int i = 0; i < n; i++)
	{
		dot += v1[i] * v2[i];
	}
	return dot;
}

double VecSqrLen(int n, const float v[]) {
	return VecDot(n, v, v);
}

void ImplicitMatrixLap::MatVecMult(const float x[], float r[]) const
{
	/** Interior corners */
	// Bottom left
	r[Idx2DTo1D(0, 0)] = x[Idx2DTo1D(0, 1)]
					   + x[Idx2DTo1D(1, 0)]
					   - 2.0f * x[Idx2DTo1D(0, 0)];
	// Top right
	r[Idx2DTo1D(m_GridSize - 1, m_GridSize - 1)] = x[Idx2DTo1D(m_GridSize - 1, m_GridSize - 2)]
												 + x[Idx2DTo1D(m_GridSize - 2, m_GridSize - 1)]
												 - (2.0 * x[Idx2DTo1D(m_GridSize - 1, m_GridSize - 1)]);
	// Bottom right
	r[Idx2DTo1D(m_GridSize - 1, 0)] = x[Idx2DTo1D(m_GridSize - 1, 1)]
								    + x[Idx2DTo1D(m_GridSize - 2, 0)]
									- (2.0 * x[Idx2DTo1D(m_GridSize - 1, 0)]);
	// Top left
	r[Idx2DTo1D(0, m_GridSize - 1)] = x[Idx2DTo1D(1, m_GridSize - 1)]
									+ x[Idx2DTo1D(0, m_GridSize - 2)]
									- 2.0 * x[Idx2DTo1D(0, m_GridSize - 1)];

	/** Interior edges */
	for (int i = 1; i < m_GridSize - 1; ++i)
	{
		// Left column
		r[Idx2DTo1D(0, i)] = x[Idx2DTo1D(0, i - 1)]
						   + x[Idx2DTo1D(0, i + 1)]
						   + x[Idx2DTo1D(1, i)]
						   - 3.0 * x[Idx2DTo1D(0, i)];
		// Right column
		r[Idx2DTo1D(m_GridSize - 1, i)] = x[Idx2DTo1D(m_GridSize - 1, i - 1)]
										+ x[Idx2DTo1D(m_GridSize - 1, i + 1)]
										+ x[Idx2DTo1D(m_GridSize - 2, i)]
										- (3.0 * x[Idx2DTo1D(m_GridSize - 1, i)]);
		// Bottom row
		r[Idx2DTo1D(i, 0)] = x[Idx2DTo1D(i - 1, 0)]
						   + x[Idx2DTo1D(i + 1, 0)]
						   + x[Idx2DTo1D(i, 1)]
						   - 3.0 * x[Idx2DTo1D(i, 0)];
		// Top row
		r[Idx2DTo1D(i, m_GridSize - 1)] = x[Idx2DTo1D(i - 1, m_GridSize - 1)]
										+ x[Idx2DTo1D(i + 1, m_GridSize - 1)]
										+ x[Idx2DTo1D(i, m_GridSize - 2)]
										- 3.0 * x[Idx2DTo1D(i, m_GridSize - 1)];
	}

	/** Middle grid cells */
	for (int i = 1; i < m_GridSize - 1; i++)
	{
		for (int j = 1; j < m_GridSize - 1; j++)
		{
			r[Idx2DTo1D(i, j)] = x[Idx2DTo1D(i + 1, j)]
							+ x[Idx2DTo1D(i - 1, j)]
							+ x[Idx2DTo1D(i, j + 1)]
							+ x[Idx2DTo1D(i, j - 1)]
							- (4.0 * x[Idx2DTo1D(i, j)]);
		}
	}
}

double ConjGrad(int n,
				const ImplicitMatrix *A,
				float x[],
				const float b[],
				double epsilon, // how low should we go?
				int *steps)
{
	int i = 0;
	int iMax;
	float alpha, beta, rSqrLen, rSqrLenOld, u;

	float *r = new float[n];
	float *d = new float[n];
	float *t = new float[n];
	float *temp = new float[n];

	VecAssign(n, x, b);

	VecAssign(n, r, b);
	A->MatVecMult(x, temp);
	VecDiffEqual(n, r, temp);

	rSqrLen = VecSqrLen(n, r);
	//printf("test %f\n", rSqrLen);

	VecAssign(n, d, r);

	if (steps && *steps > 0)
	{
		iMax = *steps;
	}
	else
	{
		iMax = 100000;
	}

	if (rSqrLen > epsilon)
	{
		while (i < iMax)
		{
			i++;
			A->MatVecMult(d, t);
			u = VecDot(n, d, t);

			if (u == 0)
			{
				printf("(SolveConjGrad) d'Ad = 0\n");
				break;
			}

			// How far should we go?
			alpha = rSqrLen / u;

			// Take a step along direction d
			VecAssign(n, temp, d);
			VecTimesScalar(n, temp, alpha);
			VecAddEqual(n, x, temp);

			if (i & 0x3F)
			{
				VecAssign(n, temp, t);
				VecTimesScalar(n, temp, alpha);
				VecDiffEqual(n, r, temp);
			}
			else
			{
				// For stability, correct r every 64th iteration
				VecAssign(n, r, b);
				A->MatVecMult(x, temp);
				VecDiffEqual(n, r, temp);
			}

			rSqrLenOld = rSqrLen;
			rSqrLen = VecSqrLen(n, r);

			//printf("error: %f\n", rSqrLen);
			// Converged! Let's get out of here
			if (rSqrLen <= epsilon)
			{
				break;
			}

			// Change direction: d = r + beta * d
			beta = rSqrLen / rSqrLenOld;
			VecTimesScalar(n, d, beta);
			VecAddEqual(n, d, r);
		}
	}

	delete[] r;
	delete[] d;
	delete[] t;
	delete[] temp;

	if (steps)
	{
		*steps = i;
	}

	return rSqrLen;
}
