/*
 * Bayes++ the Bayesian Filtering Library
 * Copyright (c) 2002 Michael Stevens, Australian Centre for Field Robotics
 * See Bayes++.htm for copyright license details
 *
 * $Header$
 * $NoKeywords: $
 */

/*
 * Information Root Filter.
 */
#include "bayesFlt.hpp"
#include "infRtFlt.hpp"
#include "matSup.hpp"
#include "uLAPACK.hpp"	// Common LAPACK interface
#include <algorithm>

/* Filter namespace */
namespace Bayesian_filter
{
	using namespace Bayesian_filter_matrix;


Information_root_filter::Information_root_filter (size_t x_size, size_t /*z_initialsize*/) :
		Extended_filter(x_size),
		r(x_size), R(x_size,x_size)
/*
 * Set the size of things we know about
 */
{}

Information_root_info_filter::Information_root_info_filter (size_t x_size, size_t z_initialsize) :
		Information_form_filter (x_size),
		Information_root_filter (x_size, z_initialsize)
{}


void Information_root_filter::init ()
/*
 * Inialise the filter from x,X
 * Predcondition:
 *		x,X
 * Postcondition:
 *		inv(R)*inv(R)' = X is PSD
 *		r = R*x
 */
{
						// Information Root
	Float rcond = UCfactor (R, X);
	rclimit.check_PSD(rcond, "Initial X not PSD");
	(void)UTinverse (R);
						// Information Root state r=R*x
	r.assign (prod(R,x));
}

void Information_root_info_filter::init_yY ()
/*
 * Special Initialisation directly from Information form
 * Predcondition:
 *		y,Y
 * Postcondition:
 *		R'*R = Y is PSD
 *		r = inv(R)'*y
 */
{
					// Temporary R Matrix for factorisation
	const size_t n = x.size();
	LTriMatrix LC(n,n);
					// Information Root
	Float rcond = LdLfactor (LC, Y);
	rclimit.check_PSD(rcond, "Initial Y not PSD");

	{				// Lower triangular Choleksy factor of LdL'
		size_t i,j;
		for (i = 0; i < n; ++i)
		{
			using namespace std;		// for sqrt
			LTriMatrix::value_type sd = LC(i,i);
			sd = sqrt(sd);
			LC(i,i) = sd;
						// Multiply columns by square of non zero diagonal
			for (j = i+1; j < n; ++j)
			{
				LC(j,i) *= sd;		// TODO Use Row operation
			}
		}
	}
	R = FM::trans(LC);			// R = (L*sqrt(d))'

	UTriMatrix RI(n,n);
	RI = R;
	(void)UTinverse(RI);
	r.assign (prod(FM::trans(RI),y));
}

void Information_root_filter::update ()
/*
 * Recompute x,X from r,R
 * Precondition:
 *		r(k|k),R(k|k)
 * Postcondition:
 *		r(k|k),R(k|k)
 *		x = inv(R)*r
 *		X = inv(R)*inv(R)'
 */
{
	UTriMatrix RI (R);	// Invert Cholesky factor
	bool singular = UTinverse (RI);
	if (singular)
		filter_error ("R not PD");

	X.assign (prod_SPD(RI));		// X = RI*RI'
	x.assign (prod(RI,r));

	assert_isPSD (X);
}

void Information_root_info_filter::update_yY ()
/*
 * Recompute y,Y from r,R
 * Precondition:
 *		r(k|k),R(k|k)
 * Postcondition:
 *		r(k|k),R(k|k)
 *		Y = R'*R
 *		y = Y*x
 */
{
	Information_root_filter::update();
	Y.assign (prod(trans(R),R));		// Y = R'*R
	y.assign (prod(Y,x));
}


void Information_root_filter::inverse_Fx (FM::DenseColMatrix& invFx, const FM::Matrix& Fx)
/*
 * Numerical Inversion of Fx using LU factorisation
 * Required LAPACK getrf (with PIVOTING) and getrs
 */
{
								// LU factorise with pivots
	DenseColMatrix FxLU (Fx);
	LAPACK::pivot_t ipivot(FxLU.size1());		// Pivots initialised to zero
	ipivot.clear();

	int info = LAPACK::getrf(FxLU, ipivot);
	if (info < 0)
		throw Bayes_filter_exception ("Fx not LU factorisable");

	FM::identity(invFx);				// Invert
	info = LAPACK::getrs('N', FxLU, ipivot, invFx);
	if (info != 0)
		throw Bayes_filter_exception ("Predict Fx not LU invertable");
}



Bayes_base::Float
 Information_root_filter::predict (Linrz_predict_model& f, const FM::ColMatrix& invFx, bool linear_r)
/* Linrz Prediction: using precomputed inverse of f.Fx
 * Precondition:
 *   r(k|k),R(k|k)
 * Postcondition:
 *   r(k+1|k) computed using QR decomposition see Ref[1]
 *   R(k+1|k)
 *
 * r can be computed in two was:
 * Either directly in the linear form or  using extended form via R*f.f(x)
 *
 * Requires LAPACK geqrf for QR decomposition (without PIVOTING)
 */
{
	if (!linear_r)
		update ();		// x is required for f(x);

						// Require Root of correlated predict noise (may be semidefinite)
	Matrix Gqr (f.G);
	for (Vec::const_iterator qi = f.q.begin(); qi != f.q.end(); ++qi)
	{
		if (*qi < 0)
			filter_error ("Predict q Not PSD");
		column(Gqr, qi.index()) *= std::sqrt(*qi);
	}
						// Form Augmented matrix for factorisation
	const size_t x_size = x.size();
	const size_t q_size = f.q.size();
						// Column major required for LAPACK, also this property is using in indexing
	DenseColMatrix A(q_size+x_size, q_size+x_size+unsigned(linear_r));
	FM::identity (A);	// Prefill with identity for topleft and zero's in off diagonals

	Matrix RFxI (prod(R, invFx));
	A.sub_matrix(q_size,q_size+x_size, 0,q_size).assign (prod(RFxI, Gqr));
	A.sub_matrix(q_size,q_size+x_size, q_size,q_size+x_size).assign (RFxI);
	if (linear_r)
		A.sub_column(q_size,q_size+x_size, q_size+x_size).assign (r);

						// Calculate factorisation so we have and upper triangular R
	DenseVec tau(q_size+x_size);
	int info = LAPACK::geqrf (A, tau);
	if (info != 0)
			filter_error ("Predict no QR factor");
						// Extract the roots, junk in strict lower triangle
	R = UpperTri( A.sub_matrix(q_size,q_size+x_size, q_size,q_size+x_size) );
    if (linear_r)
		r = A.sub_column(q_size,q_size+x_size, q_size+x_size);
	else
		r.assign (prod(R,f.f(x)));	// compute r using f(x)

	return UCrcond(R);	// compute rcond of result
}


Bayes_base::Float
 Information_root_filter::predict (Linrz_predict_model& f)
/* Linrz Prediction: computes inverse model using inverse_Fx 
 */
{
						// Require inverse(Fx)
	DenseColMatrix FxI(f.Fx.size1(), f.Fx.size2());
	inverse_Fx (FxI, f.Fx);

	return predict (f, ColMatrix(FxI), false);
}


Bayes_base::Float
 Information_root_filter::predict (Linear_predict_model& f)
/* Linear Prediction: computes inverse model using inverse_Fx
 */
{
						// Require inverse(Fx)
	DenseColMatrix FxI(f.Fx.size1(), f.Fx.size2());
	inverse_Fx (FxI, f.Fx);

	return predict (f, ColMatrix(FxI), true);
}


Bayes_base::Float Information_root_filter::observe_innovation (Linrz_correlated_observe_model& h, const Vec& s)
/* Extended linrz correlated observe
 * Precondition:
 *		r(k+1|k),R(k+1|k)
 * Postcondition:
 *		r(k+1|k+1),R(k+1|k+1)
 *
 * Uses LAPACK geqrf for QR decomposigtion (without PIVOTING)
 * ISSUE correctness of linrz form needs validation
 */
{
	const size_t x_size = x.size();
	const size_t z_size = s.size();
						// Size consistency, z to model
	if (z_size != h.Z.size1())
		filter_error("observation and model size inconsistent");

						// Require Inverse of Root of uncorrelated observe noise
	UTriMatrix Zir(z_size,z_size);
	Float rcond = UCfactor (Zir, h.Z);
	rclimit.check_PSD(rcond, "Z not PSD");
	UTinverse (Zir);
						// Form Augmented matrix for factorisation
	DenseColMatrix A(x_size+z_size, x_size+1);	// Column major required for LAPACK, also this property is using in indexing
	A.sub_matrix(0,x_size, 0,x_size).assign (R);
	A.sub_matrix(x_size,x_size+z_size, 0,x_size).assign (prod(Zir, h.Hx));
	A.sub_column(0,x_size, x_size).assign (r);
	A.sub_column(x_size,x_size+z_size, x_size).assign (prod(Zir, s+prod(h.Hx,x)));

						// Calculate factorisation so we have and upper triangular R
	DenseVec tau(x_size+1);
	int info = LAPACK::geqrf (A, tau);
	if (info != 0)
			filter_error ("Observe no QR factor");
						// Extract the roots, junk in strict lower triangle
	R = UpperTri( A.sub_matrix(0,x_size, 0,x_size) );
	r = A.sub_column(0,x_size, x_size);

	return UCrcond(R);	// compute rcond of result
}


Bayes_base::Float Information_root_filter::observe_innovation (Linrz_uncorrelated_observe_model& h, const Vec& s)
/* Extended linrz uncorrelated observe
 * Precondition:
 *		r(k+1|k),R(k+1|k)
 * Postcondition:
 *		r(k+1|k+1),R(k+1|k+1)
 *
 * Uses LAPACK geqrf for QR decomposigtion (without PIVOTING)
 * ISSUE correctness of linrz form needs validation
 * ISSUE Efficiency. Product of Zir can be simplified
 */
{
	const size_t x_size = x.size();
	const size_t z_size = s.size();
						// Size consistency, z to model
	if (z_size != h.Zv.size())
		filter_error("observation and model size inconsistent");

						// Require Inverse of Root of uncorrelated observe noise
	DiagMatrix Zir(z_size,z_size);
	Zir.clear();
	for (size_t i = 0; i < z_size; ++i)
	{
		using namespace std;
		Zir(i,i) = 1 / sqrt(h.Zv[i]);
	}
						// Form Augmented matrix for factorisation
	DenseColMatrix A(x_size+z_size, x_size+1);	// Column major required for LAPACK, also this property is using in indexing
	A.sub_matrix(0,x_size, 0,x_size).assign (R);
	A.sub_matrix(x_size,x_size+z_size, 0,x_size).assign (prod(Zir, h.Hx));
	A.sub_column(0,x_size, x_size).assign (r);
	A.sub_column(x_size,x_size+z_size, x_size).assign (prod(Zir, s+prod(h.Hx,x)));

						// Calculate factorisation so we have and upper triangular R
	DenseVec tau(x_size+1);
	int info = LAPACK::geqrf (A, tau);
	if (info != 0)
			filter_error ("Observe no QR factor");
						// Extract the roots, junk in strict lower triangle
	R = UpperTri( A.sub_matrix(0,x_size, 0,x_size) );
	r = A.sub_column(0,x_size, x_size);

	return UCrcond(R);	// compute rcond of result
}

}//namespace
