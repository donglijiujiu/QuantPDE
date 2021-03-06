#ifndef QUANT_PDE_CORE_CRANK_NICOLSON_HPP
#define QUANT_PDE_CORE_CRANK_NICOLSON_HPP

#include <climits>

/**
 * QuantPDE namespace.
 */
namespace QuantPDE {

/**
 * The Crank-Nicolson method.
 *
 * Let \f$\Delta t\equiv t^1 - t^0\f$ where \f$t^1\f$ is the current time and
 * \f$t^0\f$ is the previous time. This creates the linear system
 * \f[ \left[I + A(t^1)\Delta t/2\right]\mathbf{x}^1 = \left[I - A(t^0)\Delta
 * t/2\right]\mathbf{x}^0 + \left[b\left(t^1\right) + b\left(t^0\right)\right]/2
 * \f]
 *
 * @tparam ThetaInverse 1 for implicit method, 2 for Crank-Nicolson, and
 *                      std::numeric_limits<Real>::infinity() for infinity.
 *
 * @see QuantPDE::TimeIteration
 */
template <Index Dimension, bool Forward, int ThetaInverse = 2>
class CrankNicolson : public Discretization<Dimension> {

	static_assert(ThetaInverse > 0, "ThetaInverse must be positive.");
	static constexpr Real theta = 1. / ((Real) ThetaInverse);

	LinearSystem &system;

	inline Real dt() const {
		const Real
			t1 = this->nextTime(),
			t0 = this->time(0)
		;

		const Real dt = Forward ? t1 - t0 : t0 - t1;
		assert(dt > epsilon);

		return dt;
	}

	virtual Matrix Ad(Real t1) {
		// Explicit
		if(theta < QuantPDE::epsilon) {
			return this->domain.identity();
		}

		// Not explicit
		return
			this->domain.identity()
			+ system.A(t1) * theta * dt()
		;
	}

	virtual Vector bd(Real t1) {
		const Real    t0 = this->time(0);
		const Vector &v0 = this->iterand(0);

		// TODO: Optimize for explicit method

		return (
			this->domain.identity()
			- system.A(t0) * (1-theta) * dt()
		) * v0 + ( theta * system.b(t1) + (1-theta) * system.b(t0) );
	}

public:

	virtual bool isATheSame() const {
		// Explicit
		if(theta < QuantPDE::epsilon) {
			return true;
		}

		// Not explicit
		return this->isTimestepTheSame() && system.isATheSame();
	}

	/**
	 * @param domain The spatial domain to solve the problem on.
	 * @param system Creates
	 *               \f$A\left(t\right)\f$ and \f$b\left(t\right)\f$.
	 */
	template <typename D>
	CrankNicolson(
		D &domain,
		LinearSystem &system
	) noexcept :
		Discretization<Dimension>(domain),
		system(system) {
	}

};

template <Index Dimension>
using ReverseCrankNicolson = CrankNicolson<Dimension, false>;

template <Index Dimension>
using ForwardCrankNicolson = CrankNicolson<Dimension, true>;

typedef ReverseCrankNicolson<1> ReverseCrankNicolson1;
typedef ReverseCrankNicolson<2> ReverseCrankNicolson2;
typedef ReverseCrankNicolson<3> ReverseCrankNicolson3;

typedef ForwardCrankNicolson<1> ForwardCrankNicolson1;
typedef ForwardCrankNicolson<2> ForwardCrankNicolson2;
typedef ForwardCrankNicolson<3> ForwardCrankNicolson3;

template <Index Dimension>
using ReverseExplicitMethod = CrankNicolson<Dimension, false,
	std::numeric_limits<int>::max()>;

template <Index Dimension>
using ForwardExplicitMethod = CrankNicolson<Dimension, true,
	std::numeric_limits<int>::max()>;

typedef ReverseExplicitMethod<1> ReverseExplicitMethod1;
typedef ReverseExplicitMethod<2> ReverseExplicitMethod2;
typedef ReverseExplicitMethod<3> ReverseExplicitMethod3;

typedef ForwardExplicitMethod<1> ForwardExplicitMethod1;
typedef ForwardExplicitMethod<2> ForwardExplicitMethod2;
typedef ForwardExplicitMethod<3> ForwardExplicitMethod3;

} // QuantPDE

#endif
