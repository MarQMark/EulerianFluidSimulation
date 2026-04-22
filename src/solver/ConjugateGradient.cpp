#include "ConjugateGradient.h"

#include "simulation/Properties.h"

void MatVecMult(ScalField& result, const ScalField& x) {

	const unsigned int w = x.width();
	const unsigned int h = x.height();
	/** Interior corners */
	// Bottom left
	result(0, 0) = x(0, 1)
					   + x(1, 0)
					   - 2.f * x(0, 0);
	// Top right
	result(w - 1, h - 1) = x(w - 1, h - 2)
							 + x(w - 2, h - 1)
							 - (2.f * x(w - 1, h - 1));
	// Bottom right
	result(w - 1, 0) = x(w - 1, 1)
									+ x(w - 2, 0)
									- (2.f * x(w - 1, 0));
	// Top left
	result(0, h - 1) = x(1, h - 1)
									+ x(0, h - 2)
									- 2.f * x(0, h - 1);

	/** Interior edges */
	for (int i = 1; i < h - 1; ++i) {
		// Left column
		result(0, i) = x(0, i - 1)
						   + x(0, i + 1)
						   + x(1, i)
						   - 3.f * x(0, i);
		// Right column
		result(w - 1, i) = x(w - 1, i - 1)
										+ x(w - 1, i + 1)
										+ x(w - 2, i)
										- (3.f * x(w - 1, i));
	}

	for (int i = 1; i < w - 1; ++i) {
		// Bottom row
		result(i, 0) = x(i - 1, 0)
						   + x(i + 1, 0)
						   + x(i, 1)
						   - 3.f * x(i, 0);
		// Top row
		result(i, h - 1) = x(i - 1, h - 1)
										+ x(i + 1, h - 1)
										+ x(i, h - 2)
										- 3.f * x(i, h - 1);
	}

	/** Middle grid cells */
	for (int i = 1; i < w - 1; i++)
	{
		for (int j = 1; j < h - 1; j++)
		{
			result(i, j) = x(i + 1, j)
							+ x(i - 1, j)
							+ x(i, j + 1)
							+ x(i, j - 1)
							- (4.f * x(i, j));
		}
	}
}

float ConjugateGradient(ScalField& pressure, const ScalField& divergence, const int steps) {
	ScalField t(pressure.width(), pressure.height());
	ScalField tmp(pressure.width(), pressure.height());
	ScalField r(divergence);

	pressure = divergence;
	MatVecMult(t, pressure);
	r -= t;
	ScalField d(r);

	float sqrtLen = r.sqLen();
	if (sqrtLen < Properties::Get()->THRESHOLD)
		return sqrtLen;

	for (int i = 0; i < steps; i++) {
		MatVecMult(t, d);
		const float alpha = sqrtLen / d.dot(t);
		//tmp = d;
		//tmp *= alpha;
		//pressure += tmp;
		//pressure += (d * alpha);
		pressure.addScaled(d, alpha);

		if (i & 0x3F) {
			//tmp = t;
			//tmp *= alpha;
			//r -= tmp;
			//r -= t * alpha;
			r.addScaled(t, -alpha);
		}
		else {
			MatVecMult(t, pressure);
			r = divergence;
			r -= t;
			//r = divergence - t;
		}

		const float oldSqrtLen = sqrtLen;
		sqrtLen = r.sqLen();
		if (sqrtLen < Properties::Get()->THRESHOLD)
			return sqrtLen;

		const float beta = sqrtLen / oldSqrtLen;
		d *= beta;
		d += r;
		//d = (d * beta) + r;
	}

	return sqrtLen;
}