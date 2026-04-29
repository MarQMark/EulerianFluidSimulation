#include "ConjugateGradient.h"

#include "simulation/Properties.h"

#if 0
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
#else
void MatVecMult(ScalField& result, const ScalField& x, const ScalField& vof) {
	const int w = x.width();
	const int h = x.height();
	const float FULL = 0.99f;
	const float SURF = 0.01f;

	for (int y = 0; y < h; y++) {
		for (int x_ = 0; x_ < w; x_++) {

			// Air or surface cell: Dirichlet p = 0 → identity row
			if (vof(x_, y) < FULL) {
				result(x_, y) = x(x_, y);
				continue;
			}

			// Full fluid cell: count active neighbors
			float sum = 0.0f;
			int   nCount = 0;

			auto sampleNeighbor = [&](int nx, int ny) {
				if (nx < 0 || nx >= w || ny < 0 || ny >= h) return; // solid wall
				if (vof(nx, ny) < SURF) return;  // air neighbor — skip coupling
				// Surface neighbor: Dirichlet p=0, contributes 0 to sum but still
				// counts toward diagonal so the row stays consistent
				nCount++;
				if (vof(nx, ny) >= FULL)
					sum += x(nx, ny);
				// if surface neighbor: x(nx,ny) is 0 by identity row, so += 0
			};

			sampleNeighbor(x_ - 1, y);
			sampleNeighbor(x_ + 1, y);
			sampleNeighbor(x_, y - 1);
			sampleNeighbor(x_, y + 1);

			result(x_, y) = sum - (float)nCount * x(x_, y);
		}
	}
}
#endif

float ConjugateGradient2(ScalField& pressure, const ScalField& divergence, const ScalField& vof, const int steps) {
	ScalField t(pressure.width(), pressure.height());
	ScalField tmp(pressure.width(), pressure.height());
	ScalField r(divergence);

	pressure = divergence;
	MatVecMult(t, pressure, vof);
	r -= t;
	ScalField d(r);

	float sqrtLen = r.sqLen();
	if (sqrtLen < Properties::Get()->THRESHOLD)
		return sqrtLen;

	for (int i = 0; i < steps; i++) {
		MatVecMult(t, d, vof);
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
			MatVecMult(t, pressure, vof);
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