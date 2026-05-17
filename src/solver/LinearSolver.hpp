// Karen's CGD

#include "simulation/ScalField.h"


/*****************************************************************************
 * Matrix class the solver will accept
 *****************************************************************************/
class ImplicitMatrix
{
public:
	virtual ~ImplicitMatrix()
	{
	}

	virtual void MatVecMult(const float x[], float r[]) const = 0;
};

struct ImplicitMatrixDiff : ImplicitMatrix {
	int _width, _height;
	float _scale; // ν * dt / (dx * dx), assuming dx == dy == 1

	ImplicitMatrixDiff(int width, int height, float visc, float dt)
		: _width(width), _height(height), _scale(visc * dt) {}

	// Computes out = (I - ν·dt·∇²) * in
	void MatVecMult(const float in[], float out[]) const override {
		for (int y = 1; y < _height - 1; y++) {
			for (int x = 1; x < _width - 1; x++) {
				int idx = y * _width + x;
				// Diagonal: identity + 4 neighbors from laplacian
				out[idx] = in[idx] * (1.0 + 4.0 * _scale)
						 - _scale * (in[idx - 1]         // left
								   + in[idx + 1]         // right
								   + in[idx - _width]    // bottom
								   + in[idx + _width]);  // top
			}
		}
		// Boundary rows/columns stay zero (no-slip)
		for (int x = 0; x < _width; x++) {
			out[x] = in[x];                                   // bottom row
			out[(_height - 1) * _width + x] = in[(_height - 1) * _width + x];
		}
		for (int y = 0; y < _height; y++) {
			out[y * _width] = in[y * _width];                 // left col
			out[y * _width + _width - 1] = in[y * _width + _width - 1];
		}
	}
};


/*****************************************************************************
 * This implicit matrix represents a discrete Laplacian operator.
 *****************************************************************************/
class ImplicitMatrixLap : public ImplicitMatrix
{
public:
	explicit ImplicitMatrixLap(int gridSize) : m_GridSize(gridSize)
	{
	}

	/*************************************************************************
	 * Calculates the result of an implicit matrix multiply with the vector x
	 * and stores it in r.
	 * This implicit matrix multiply calculates the Laplacian of x
	 * assuming x is a grid of m_NumCells by m_NumCells. In general,
	 * the Laplacian at x[i] will be -4*x[i] plus each of its neighboring
	 * cells in the grid. This would then be divided by the size of a cell, but
	 * I assume that the cell size is unit length.
	 *
	 * @param x The grid to take the Laplacian of.
	 * @param r Stores the calculated Laplacian.
	 *************************************************************************/
	virtual void MatVecMult(const float x[], float r[]) const override;

private:
	// The width and length of grid of cells.
	int m_GridSize;

	int Idx2DTo1D(int x, int y) const
	{
		return x + (m_GridSize * y);
	}
};

/*****************************************************************************
 * Solves Ax = b for a symmetric, positive definite matrix A.
 * A is represented implicitly by the function MatVecMult()
 * which performs a matrix vector multiply, Av, and places the result in r.
 * 
 * @param n             The length of the vectors x and b.
 * @param A             Symmetric, positive definite matrix.
 * @param epsilon       The error tolerance.
 * @param steps[in,out] When passed in, steps is the maximum number of
 *                      iterations to take to solve. If not set, or set to
 *                      less than 1, MAX_STEPS is used instead.
 *                      Upon completion, steps contains the actual number of
 *                      iterations taken.
 *****************************************************************************/
double ConjGrad(int n,
				const ImplicitMatrix *A,
				float x[],
				const float b[],
				double epsilon,
				int *steps);

float ConjugateGradient(ScalField& pressure, const ScalField& divergence, const int steps);