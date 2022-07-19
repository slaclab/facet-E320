/*
 * Uses the ceres solver to fit a 2D curve (based on the sample code / tutorial provided
 * by the ceres solver documentation).
 *
 * compile using: 
 * g++ curve_fitting_01.cc -lceres -lglog -I/usr/local/include/eigen3/  -pthread -llapack
 *
 * The data (data.h) were generated using python3 generate_data.py
 *
 * Ceres Solver - A fast non-linear least squares minimizer
 * copyright 2015 Google Inc. All rights reserved.
 * http://ceres-solver.org/
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright Sebastian Meuren, 2022 
 */

#include "ceres/ceres.h"
//#include "glog/logging.h"
#include "data.h"

using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solve;
using ceres::Solver;


/*
 * Defines a cost function for one residual and four parameters
 */
class MyCostFunction : public ceres::SizedCostFunction<1, 1, 1, 1, 1> {
	public:
		MyCostFunction(const double x, const double y, const double z) : xval(x), yval(y), zval(z){
		}
		virtual ~MyCostFunction() {}
		virtual bool Evaluate(double const* const* parameters,
                        double* residuals,
                        double** jacobians) const {

	const double	amplitude	= parameters[0][0];
	const double	offset	 	= parameters[1][0];
	const double	sigmax		= parameters[2][0];
	const double	sigmay		= parameters[3][0];
		
	//std::cout << "params: " << amplitude << "\t" << offset << "\t" << sigmax << "\t" << sigmay << std::endl;

    const double 	result		= offset + amplitude\
    								*exp(-xval*xval/(2.0*sigmax*sigmax)\
    									 -yval*yval/(2.0*sigmay*sigmay));
 
    residuals[0] = result - zval;

	// Compute the Jacobian if asked for.
	if (jacobians != nullptr){
			jacobians[0][0] = exp(-xval*xval/(2.0*sigmax*sigmax) -yval*yval/(2.0*sigmay*sigmay));
			jacobians[1][0] = 1.0;
			jacobians[2][0] = amplitude * (xval*xval/(sigmax*sigmax*sigmax))\
									*exp(-xval*xval/(2.0*sigmax*sigmax)
    									 -yval*yval/(2.0*sigmay*sigmay));
			jacobians[3][0] = amplitude * (yval*yval/(sigmay*sigmay*sigmay))\
    								*exp(-xval*xval/(2.0*sigmax*sigmax)\
    									 -yval*yval/(2.0*sigmay*sigmay));
	}
    return true;
  }
	private:
		const double xval;
		const double yval;
		const double zval;
};





int main(int argc, char** argv) {
	//google::InitGoogleLogging(argv[0]);

	using ceres::AutoDiffCostFunction;
	using ceres::CauchyLoss;
	using ceres::CostFunction;
	using ceres::Problem;
	using ceres::Solve;
	using ceres::Solver;

	// The variable to solve for with its initial value.
	double	amplitude	= 1.0;
	double	offset	 	= 0.0;
	double	sigmax		= 1.0;
	double	sigmay		= 1.0;

	// Build the problem.
	Problem problem;
	
	int datacount = 50*50;
	
	for (int i = 0; i < datacount; ++i) {
		CostFunction* cost_function =	
			new MyCostFunction(data[3 * i], data[3 * i + 1], data[3 * i + 2]);
 	 	problem.AddResidualBlock(cost_function, nullptr, &amplitude, &offset, &sigmax, &sigmay);
	}
	// Run the solver
	Solver::Options options;
	options.linear_solver_type 				= ceres::DENSE_QR;
	options.minimizer_progress_to_stdout	= true;


	Solver::Summary summary;
	Solve(options, &problem, &summary);
	std::cout << summary.BriefReport() << "\n";
	std::cout	<< "fit result: " << amplitude << " " << offset\
				<< " " << sigmax << " " << sigmay << std::endl;
	return 0;
}
