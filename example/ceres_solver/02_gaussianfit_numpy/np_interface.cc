/********************************************************************************
 * Fit 2D (rotated) Gaussian to a 2D numpy float image.
 * Fitting is carried out using the ceres solver.
 *
 * Requires Python 3!
 * 
 * Compile: python3 setup.py build
 * Test:    python3 test.py
 *
 *
 * Ceres Solver - A fast non-linear least squares minimizer
 * copyright 2015 Google Inc. All rights reserved.
 * http://ceres-solver.org/
 *
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
 *
 ********************************************************************************/

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <arrayobject.h>
#include <math.h>
#include <stdio.h>
#include <stdexcept>

#include "ceres/ceres.h"


using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solve;
using ceres::Solver;

/********************************************************************************
 * Fitting a rotated 2D Gaussian to a 2D image array
 ********************************************************************************/
// Defines a cost function for one residual and seven parameters
#define	NUMBER_OF_PARAMS	7
class GaussianCostFunction : public ceres::SizedCostFunction<1, 1, 1, 1, 1, 1, 1, 1> {
	public:
		GaussianCostFunction(const double x, const double y, const double z)\
			 : xval(x), yval(y), zval(z){}
		virtual ~GaussianCostFunction() {}
		virtual bool Evaluate(double const* const* parameters,
                        double* residuals,
                        double** jacobians) const {

	// get the parameters
	const double	xnot 			= parameters[0][0];
	const double	ynot 			= parameters[1][0];
	const double	sigmaone 		= parameters[2][0];
	const double	sigmatwo		= parameters[3][0];
	const double	phi				= parameters[4][0];
	const double	offset			= parameters[5][0];
	const double	amplitude		= parameters[6][0];

	// rotate the orientation
	const double	cphi			= cos(phi);
	const double	sphi			= sin(phi);
	const double	xprime			= cphi*(xval-xnot) - sphi*(yval-ynot);
	const double	yprime			= sphi*(xval-xnot) + cphi*(yval-ynot);

	const double	mainfunc		= exp(-xprime*xprime/(2.0*sigmaone*sigmaone)\
											-yprime*yprime/(2.0*sigmatwo*sigmatwo));

	// evaluate the Gaussian
	const double	result			= offset + amplitude * mainfunc;

    residuals[0] = result - zval;
	// Compute the Jacobian if asked for.
	if (jacobians != nullptr){
			double	res;


			// Partial derivative wrt xnot
			res			= amplitude * mainfunc\
							* ((-xprime/(sigmaone*sigmaone))*(-cphi)\
							   + (-yprime/(sigmatwo*sigmatwo))*(-sphi));
			jacobians[0][0] = res;
			
			// Partial derivative wrt ynot
			res			= amplitude * mainfunc\
							* ((-xprime/(sigmaone*sigmaone)) * (sphi)\
							   + (-yprime/(sigmatwo*sigmatwo)) * (-cphi));
			jacobians[1][0] = res;
			
			// Partial derivative wrt sigmaone
			res			= amplitude * mainfunc\
							* (xprime*xprime/(sigmaone*sigmaone*sigmaone));
			jacobians[2][0] = res;
			
			// Partial derivative wrt sigmatwo
			res			= amplitude * mainfunc\
							* (yprime*yprime/(sigmatwo*sigmatwo*sigmatwo));
			jacobians[3][0] = res;
			
			// Partial derivative wrt phi
			res			= amplitude * mainfunc\
							* ((-xprime/(sigmaone*sigmaone))\
							   *(-sphi*(xval-xnot)-cphi*(yval-ynot))\
							   +
							   (-yprime/(sigmatwo*sigmatwo))\
							   *(cphi*(xval-xnot)-sphi*(yval-ynot)));
			jacobians[4][0] = res;
			
			
			// Partial derivative wrt offset
			jacobians[5][0] = 1.0;

			// Partial derivative wrt amplitude
			jacobians[6][0] = mainfunc;

	}
    return true;
  }
	private:
		const double xval;
		const double yval;
		const double zval;
};

extern "C" // required when using C++ compiler
{
/********************************************************************************
 * Function that handles numpy arrays based on
 * https://numpy.org/devdocs/user/c-info.how-to-extend.html
 ********************************************************************************/
static PyObject *
twodim_Gaussian_fit(PyObject *dummy, PyObject *args)
{
	/* create argument objects */
	PyObject*	data_arg	= NULL;
	PyObject*	param_arg	= NULL;

	/* create array objects */
	PyArrayObject*	data_array	= NULL;
	PyArrayObject*	param_array	= NULL;

	try{
		/**********************
		 * Parse the arguments
		 */
		if(PyArg_ParseTuple(args, "OO!", &data_arg, &PyArray_Type, &param_arg) != 1){ 
			PyErr_SetString(PyExc_AttributeError, "PyArg_ParseTuple failed");
			return NULL;
		};
		/* get the arrays */
		data_array 			= (PyArrayObject*)\
								PyArray_FROM_OTF(data_arg, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
		if(data_array == NULL){
			throw std::runtime_error("PyArray_FROM_OTF: data_array failed");
		}

		#if NPY_API_VERSION >= 0x0000000c
		param_array 	= (PyArrayObject*)\
							PyArray_FROM_OTF(param_arg, NPY_DOUBLE, NPY_ARRAY_INOUT_ARRAY2);
		#else
		param_array 	= (PyArrayObject*)\
							PyArray_FROM_OTF(param_arg, NPY_DOUBLE, NPY_ARRAY_INOUT_ARRAY);
		#endif
		if(param_array == NULL){
			throw std::runtime_error("PyArray_FROM_OTF: param_array failed");
		}

		/********************************************************************************
		 * Sanity checks
		 ********************************************************************************/
		/* Test that we have 1D vectors */
		int	data_dim	= PyArray_NDIM(data_array);
		if(data_dim != 2){
			throw std::runtime_error("invalid first argument: 2D numpy array (data)");
		}

		int	param_dim	= PyArray_NDIM(param_array);
		if(param_dim != 1){
			throw std::runtime_error("invalid second argument: 1D numpy array (fit parameter)");
		}

		npy_intp*	data_length_array	= PyArray_DIMS(data_array);
		size_t		xpixel_count		= data_length_array[1];
		size_t		ypixel_count		= data_length_array[0];

		printf("xpixel_count: %i\n", xpixel_count);
		printf("ypixel_count: %i\n", ypixel_count);

		if(xpixel_count < 1){
			throw std::runtime_error("x-length should be larger than 0");
		}
		if(ypixel_count < 1){
			throw std::runtime_error("y-length should be larger than 0");
		}
		
		npy_intp*	param_length_array	= PyArray_DIMS(param_array);
		size_t		param_length		= param_length_array[0];

		if(param_length != NUMBER_OF_PARAMS){
			throw std::runtime_error("parameter array: invalid length");
		}

		/* Get the data pointer */
		double*	data_ptr 	= (double *)PyArray_DATA(data_array);
		double*	param_ptr	= (double *)PyArray_DATA(param_array);

		/********************************************************************************
		 * Perform the curve fitting
		 ********************************************************************************/

		// The variable to solve for with its initial value.
		double	xnot 			= param_ptr[0];
		double	ynot 			= param_ptr[1];
		double	sigmaone 		= param_ptr[2];
		double	sigmatwo		= param_ptr[3];
		double	phi				= param_ptr[4];
		double	offset			= param_ptr[5];
		double	amplitude		= param_ptr[6];


		// Build the problem.
		Problem problem;

		// add a const function for each data point
		for (int j = 0; j < ypixel_count; j++)
		{
			for (int i = 0; i < xpixel_count; i++)
			{
				int	flatindex						= j*xpixel_count + i;
				// store the pixel index for this data point
				const double 	xval			= (double)i;
				const double	yval			= (double)j;

				CostFunction* cost_function =	
					new GaussianCostFunction(xval, yval, data_ptr[flatindex]);
		 	 	problem.AddResidualBlock(cost_function, nullptr,\
		 	 			&xnot, &ynot, &sigmaone, &sigmatwo, &phi, &offset, &amplitude);

			}
		}		
		
		// Run the solver
		Solver::Options options;
		options.linear_solver_type 				= ceres::DENSE_QR;
		options.minimizer_progress_to_stdout	= true;

		Solver::Summary summary;
		Solve(options, &problem, &summary);
		std::cout << summary.BriefReport() << "\n";
		std::cout	<< "fit result: " << xnot << " " << ynot\
					<< " " << sigmaone << " " << sigmatwo << " " << phi << " "\ 
					<< offset << " " << amplitude << std::endl;
 
		param_ptr[0]		= xnot;
		param_ptr[1]		= ynot;
		param_ptr[2]		= sigmaone;
		param_ptr[3]		= sigmatwo;
		param_ptr[4]		= phi;
		param_ptr[5]		= offset;
		param_ptr[6]		= amplitude;


		/********************************************************************************
		 * Post processing
		 ********************************************************************************/

		/******************************************
		 * done, normal clean-up code
		 */


		Py_DECREF(data_array);
		#if NPY_API_VERSION >= 0x0000000c
		PyArray_ResolveWritebackIfCopy(param_array);
		#endif
		Py_DECREF(param_array);
		Py_INCREF(Py_None);
		return Py_None;
	}
	catch(const std::runtime_error& exception){
		PyErr_SetString(PyExc_RuntimeError, exception.what());

		Py_DECREF(data_array);
		#if NPY_API_VERSION >= 0x0000000c
		PyArray_ResolveWritebackIfCopy(param_array);
		#endif
		Py_DECREF(param_array);
		Py_INCREF(Py_None);
		return NULL;
	}
}
}


/********************************************************************************
 * Functions provided by this module
 ********************************************************************************/
static PyMethodDef twodim_gaussian_fit_methods[] =
{
	{"fit", twodim_Gaussian_fit, METH_VARARGS, "Perform 2D Gaussian fit against an image provided by a numpy array"},
	{NULL, NULL, 0, NULL}
};

/********************************************************************************
 * Init function for the module
 ********************************************************************************/
/* module initialization */
/* Python version 3*/
static struct PyModuleDef twodim_gaussian_fit_module =
{
    PyModuleDef_HEAD_INIT,
    "testnumpycext", "Some documentation",
    -1,
    twodim_gaussian_fit_methods
};

PyMODINIT_FUNC PyInit_twodimgaussianfit(void)
{
	import_array() 
	return PyModule_Create(&twodim_gaussian_fit_module);
}
/*******************************************************************************/




