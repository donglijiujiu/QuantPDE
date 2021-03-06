////////////////////////////////////////////////////////////////////////////////
// convergence_table.cpp
// ---------------------
//
// Outputs the rate of convergence for computing the price of a
// European/American (digital or nondigital) call/put.
//
// Author: Parsiad Azimzadeh
////////////////////////////////////////////////////////////////////////////////

#include <QuantPDE/Core>
#include <QuantPDE/Modules/Lambdas>
#include <QuantPDE/Modules/Operators>

using namespace QuantPDE;
using namespace QuantPDE::Modules;

///////////////////////////////////////////////////////////////////////////////

#include <iomanip>  // setw
#include <iostream> // cout, cerr
#include <memory>   // unique_ptr
#include <numeric>  // accumulate
#include <unistd.h> // getopt

using namespace std;

///////////////////////////////////////////////////////////////////////////////

/**
 * Prints help to stderr.
 */
void help() {
	cerr <<
"convergence_table [OPTIONS]" << endl << endl <<
"Outputs the rate of convergence for computing the price of a European/American" << endl <<
"(digital or nondigital) call/put." << endl <<
endl <<
"-A" << endl <<
endl <<
"    Computes the price of an American option (default is European)." << endl <<
endl <<
"-d REAL" << endl <<
endl <<
"    Sets the dividend rate (default is 0.)." << endl <<
endl <<
"-D REAL" << endl <<
endl <<
"    Sets the payoff to be digital (default is vanilla)." << endl <<
endl <<
"-f" << endl <<
"    The initial timestep size is decreased by a factor of 4 (default is 2) to" << endl <<
"    ensure quadratic convergence in the American put case." << endl <<
endl <<
"-K REAL" << endl <<
endl <<
"    Sets the strike price (default is 100.)." << endl <<
endl <<
"-N POSITIVE_INTEGER" << endl <<
endl <<
"    Sets the initial number of steps to take in time (default is 25)." << endl <<
endl <<
"-p" << endl <<
endl <<
"    Computes the price of a European put (default is call)." << endl <<
endl <<
"-r REAL" << endl <<
endl <<
"    Sets interest rate (default is 0.04)." << endl <<
endl <<
"-R NONNEGATIVE_INTEGER" << endl <<
endl <<
"    Sets the maximum number of refinement steps in the computation (default is" << endl <<
"    5). Each refinement steps doubles the size of the spatial grid and the" << endl <<
"    number of timesteps (if variable timestepping is on, the initial timestep" << endl <<
"    is divided by 4 after refinement)." << endl <<
endl <<
"-S REAL" << endl <<
endl <<
"    Sets the initial stock price (default is 100.)." << endl <<
endl <<
"-T POSITIVE_REAL" << endl <<
endl <<
"    Sets the expiry time (default is 1.)." << endl <<
endl <<
"-v REAL" << endl <<
endl <<
"    Sets the volatility (default is 0.2)." << endl <<
endl <<
"-V" << endl <<
"    Uses variable-size timestepping (default is constant-size)." << endl << endl;
}

int main(int argc, char **argv) {

	// Default options
	Real expiry         = 1.;
	Real interest       = 0.04;
	Real volatility     = 0.2;
	Real dividends      = 0.;
	Real asset          = 100.;
	Real strike         = 100.;
	int  maxRefinement  = 5;
	int  steps          = 12;
	bool call           = true;
	bool variable       = false;
	bool american       = false;
	bool quadratic      = false;
	bool digital        = false;

	const Real target   = 1;

	// Setting options with getopt
	{ char c;
	while((c = getopt(argc, argv, "Ad:DfhK:N:pr:R:S:T:v:V")) != -1) {
		switch(c) {
			case 'A':
				american = true;
				break;
			case 'd':
				dividends = atof(optarg);
				break;
			case 'D':
				digital = true;
				break;
			case 'f':
				quadratic = true;
				break;
			case 'h':
				help();
				return 0;
			case 'K':
				strike = atof(optarg);
				break;
			case 'N':
				steps = atoi(optarg);
				if(steps <= 0) {
					cerr <<
"error: the number of steps must be positive" << endl;
					return 1;
				}
				break;
			case 'p':
				call = false;
				break;
			case 'r':
				interest = atof(optarg);
				break;
			case 'R':
				maxRefinement = atoi(optarg);
				if(maxRefinement < 0) {
					cerr <<
"error: the maximum level of refinement must be nonnegative" << endl;
					return 1;
				}
				break;
			case 'S':
				if((asset = atof(optarg)) <= 0.) {
					cerr <<
"error: the initial stock price must be positive" << endl;
					return 1;
				}
				break;
			case 'T':
				if((expiry = atof(optarg)) <= 0.) {
					cerr <<
"error: expiry time must be positive" << endl;
					return 1;
				}
				break;
			case 'v':
				volatility = atof(optarg);
				break;
			case 'V':
				variable = true;
				break;
			case ':':
			case '?':
				cerr << endl;
				help();
				return 1;
		}
	} }

	// Setting up the table
	const int td = 20;
	cout
		<< setw(td) << "Nodes"                  << "\t"
		<< setw(td) << "Steps"                  << "\t"
		<< setw(td) << "Mean Inner Iterations"  << "\t"
		<< setw(td) << "Value"                  << "\t"
		<< setw(td) << "Change"                 << "\t"
		<< setw(td) << "Ratio"
		<< endl
	;
	cout.precision(6);
	Real previousValue = nan(""), previousChange = nan("");

	// Initial discretization
	// Create grid based on initial stock price and strike price
	RectilinearGrid1 initialGrid(
		  (asset  * Axis::special)
		+ (strike * Axis::special)
	);

	// Payoff function
	Function1 payoff = digital ? (
		call ? digitalCallPayoff(strike) : digitalPutPayoff(strike)
	) : ( call ? callPayoff(strike) : putPayoff(strike) );

	unsigned factor = 1; // factor *= 2 or 4 at every step
	for(int ref = 0; ref <= maxRefinement; ++ref) {

		///////////////////////////////////////////////////////////////
		// Refine spatial grid
		///////////////////////////////////////////////////////////////

		// Refine the grid ref times
		auto grid = initialGrid.refined( ref );

		///////////////////////////////////////////////////////////////
		// Solve problem
		///////////////////////////////////////////////////////////////

		unsigned outer;
		Real inner = nan("");
		Real value;
		{
			// Black-Scholes operator (L in V_t = LV)
			BlackScholes1 bs(
				grid,
				interest, volatility, dividends
			);

			// Timestepping method
			unique_ptr<Iteration> stepper(variable
				? (Iteration *) new ReverseVariableStepper(
					0.,                       // startTime
					expiry,                   // endTime
					expiry / steps / factor,   // dt
					target / factor            // target
				)
				: (Iteration *) new ReverseConstantStepper(
					0.,                     // startTime
					expiry,                 // endTime
					expiry / steps / factor  // dt
				)
			);

			// Time discretization method
			ReverseRannacher1 discretization(grid, bs);
			discretization.setIteration(*stepper);

			// American-specific components; penalty method or not?
			IterationNode *root;
			unique_ptr<ToleranceIteration> tolerance;
			unique_ptr<PenaltyMethodDifference1> penalty;
			if(american) {
				// American case
				penalty = unique_ptr<PenaltyMethodDifference1>(
					new PenaltyMethodDifference1(
						grid,
						discretization,
						payoff
					)
				);

				tolerance = unique_ptr<ToleranceIteration>(
					new ToleranceIteration()
				);

				penalty->setIteration(*tolerance);
				stepper->setInnerIteration(*tolerance);

				root = penalty.get();
			} else {
				// European case
				root = &discretization;
			}

			// Linear system solver
			BiCGSTABSolver solver;

			// Compute solution
			auto solution = stepper->solve(
				grid,
				payoff,
				*root,
				solver
			);

			// Outer iterations
			outer = stepper->iterations()[0];

			// Average number of inner iterations
			if(american) {
				auto its = tolerance->iterations();
				inner = accumulate(its.begin(), its.end(), 0.)
						/ its.size();
			}

			// Solution at S = stock (default is 100.)
			// Linear interpolation is used to get the value off
			// the grid
			value = solution(asset);
		}

		///////////////////////////////////////////////////////////////
		// Table
		///////////////////////////////////////////////////////////////

		// Change and ratio between successive solutions
		Real
			change = value - previousValue,
			ratio = previousChange / change
		;

		// Print out row of table
		cout
			<< scientific
			<< setw(td) << grid.size() << "\t"
			<< setw(td) << outer       << "\t"
			<< setw(td) << inner       << "\t"
			<< setw(td) << value       << "\t"
			<< setw(td) << change      << "\t"
			<< setw(td) << ratio
			<< endl
		;

		previousChange = change;
		previousValue = value;

		factor *= 2;

		// Decrease by 4 at each step
		if(quadratic) {
			factor *= 2;
		}
	}

	return 0;
}
