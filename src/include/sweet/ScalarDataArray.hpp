/*
 * ScalarDataArray.hpp
 *
 *  Created on: 28 Jun 2015
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */
#ifndef SRC_SCALAR_DATA_ARRAY_HPP_
#define SRC_SCALAR_DATA_ARRAY_HPP_

#include <stdlib.h>
#include <string>
#include <iostream>
#include <limits>
#include <functional>
#include <sweet/sweetmath.hpp>
#include <sweet/openmp_helper.hpp>
#include <sweet/MemBlockAlloc.hpp>
#include <sweet/FatalError.hpp>
#include <sweet/plane/PlaneDataConfig.hpp>

/*
 * Precompiler helper functions to handle loops in spectral and physical space
 */

#if SWEET_THREADING_SPACE

	#define SCALAR_DATA_FOR_IDX(CORE)				\
		SWEET_THREADING_SPACE_PARALLEL_FOR_SIMD	\
			for (std::size_t idx = 0; idx < number_of_elements; idx++)	\
			{	CORE;	}

#else

	#define SCALAR_DATA_FOR_IDX(CORE)				\
			for (std::size_t idx = 0; idx < number_of_elements; idx++)	\
			{	CORE;	}

#endif


#if SWEET_THREADING_SPACE
#	include <omp.h>
#endif




class ScalarDataArray
{

public:
	std::size_t number_of_elements;

	/**
	 * physical space data
	 */
	double *scalar_data;

	/**
	 * allow empty initialization
	 */
public:
	ScalarDataArray()	:
		number_of_elements(0)
	{
	}



private:
	void p_allocate_buffers()
	{
		scalar_data = MemBlockAlloc::alloc<double>(
				number_of_elements*sizeof(double)
		);
	}


public:
	/**
	 * copy constructor, used e.g. in
	 * 	ScalarDataArray tmp_h = h;
	 * 	ScalarDataArray tmp_h2(h);
	 *
	 * Duplicate all data
	 */
	ScalarDataArray(
			const ScalarDataArray &i_dataArray
	)
	{
		number_of_elements = i_dataArray.number_of_elements;

		p_allocate_buffers();

		SCALAR_DATA_FOR_IDX(
				scalar_data[idx] = i_dataArray.scalar_data[idx];
		);
	}


public:
	/**
	 * copy constructor, used e.g. in
	 * 	ScalarDataArray tmp_h = h;
	 * 	ScalarDataArray tmp_h2(h);
	 *
	 * Duplicate all data
	 */
	ScalarDataArray(
			const PlaneDataConfig *i_planeDataConfig
	)
	{
		number_of_elements = i_planeDataConfig->physical_array_data_number_of_elements;

		p_allocate_buffers();
	}



public:
	/**
	 * setup the ScalarDataArray in case that the special
	 * empty constructor with int as a parameter was used.
	 *
	 * Calling this setup function should be in general avoided.
	 */
public:
	void setup(
			std::size_t i_number_of_elements
	)
	{
		if (number_of_elements != 0)
			p_free_buffer();

		number_of_elements = i_number_of_elements;

		p_allocate_buffers();
	}



	/**
	 * default constructor
	 */
public:
	ScalarDataArray(
			std::size_t i_number_of_elements
	)	: number_of_elements(i_number_of_elements)
	{
		number_of_elements = i_number_of_elements;

		if (number_of_elements == 0)
			return;

		p_allocate_buffers();
	}


	void p_free_buffer()
	{
		if (number_of_elements != 0)
			MemBlockAlloc::free(scalar_data, number_of_elements*sizeof(double));

		number_of_elements = 0;
	}


public:
	~ScalarDataArray()
	{
		p_free_buffer();
	}


	inline
	void set(
			std::size_t i,
			double i_value
	)
	{
		scalar_data[i] = i_value;
	}


	void update_lambda_array_indices(
			std::function<void(int,double&)> i_lambda	///< lambda function to return value for lat/mu
	)
	{
		SCALAR_DATA_FOR_IDX(
				i_lambda(idx, scalar_data[idx])
		);
	}


	inline
	double physical_get(
			std::size_t i
	)	const
	{
		return scalar_data[i];
	}


	inline
	void physical_set_all(
			double i_value
	)
	{
		SCALAR_DATA_FOR_IDX(
				scalar_data[idx] = i_value;
		);
	}



	/**
	 * return true, if any value is infinity
	 */
	bool reduce_boolean_all_finite() const
	{
		bool isallfinite = true;

#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(&&:isallfinite)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			isallfinite = isallfinite && std::isfinite(scalar_data[i]);

		return isallfinite;
	}



	/**
	 * return the maximum of all absolute values
	 */
	double reduce_maxAbs()	const
	{
		double maxabs = -1.0;

#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(max:maxabs)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			maxabs = std::max(maxabs, std::abs(scalar_data[i]));

		return maxabs;
	}



	/**
	 * reduce to root mean square
	 */
	double reduce_rms()
	{
		double sum = 0;
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			sum += scalar_data[i]*scalar_data[i];

		sum = std::sqrt(sum/(double)(number_of_elements));

		return sum;
	}


	/**
	 * reduce to root mean square
	 */
	double reduce_rms_quad()
	{
		double sum = 0;
		double c = 0;

#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
		{
			double value = scalar_data[i]*scalar_data[i];

			// Use Kahan summation
			double y = value - c;
			double t = sum + y;
			c = (t - sum) - y;
			sum = t;
		}

		sum -= c;

		sum = std::sqrt(sum/(double)(number_of_elements));

		return sum;
	}



	/**
	 * return the maximum of all absolute values
	 */
	double reduce_max()	const
	{
		double maxvalue = -std::numeric_limits<double>::max();
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(max:maxvalue)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			maxvalue = std::max(maxvalue, scalar_data[i]);

		return maxvalue;
	}


	/**
	 * return the maximum of all absolute values
	 */
	double reduce_min()	const
	{
		double minvalue = std::numeric_limits<double>::max();
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(min:minvalue)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			minvalue = std::min(minvalue, scalar_data[i]);

		return minvalue;
	}


	/**
	 * return the maximum of all absolute values
	 */
	double reduce_sum()	const
	{
		double sum = 0;
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			sum += scalar_data[i];

		return sum;
	}


	/**
	 * return the maximum of all absolute values, use quad precision for reduction
	 */
	double reduce_sum_quad()	const
	{
		double sum = 0;
		double c = 0;
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
		{
			double value = scalar_data[i];

			// Use Kahan summation
			double y = value - c;
			double t = sum + y;
			c = (t - sum) - y;
			sum = t;
		}

		sum -= c;

		return sum;
	}

	/**
	 * return the maximum of all absolute values
	 */
	double reduce_norm1()	const
	{
		double sum = 0;
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			sum += std::abs(scalar_data[i]);


		return sum;
	}

	/**
	 * return the sum of the absolute values.
	 */
	double reduce_norm1_quad()	const
	{
		double sum = 0;
		double c = 0;
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
		{

			double value = std::abs(scalar_data[i]);
			// Use Kahan summation
			double y = value - c;
			double t = sum + y;
			c = (t - sum) - y;
			sum = t;
		}

		sum -= c;

		return sum;
	}


	/**
	 * return the sqrt of the sum of the squared values
	 */
	double reduce_norm2()	const
	{
		double sum = 0;
#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
			sum += scalar_data[i]*scalar_data[i];


		return std::sqrt(sum);
	}


	/**
	 * return the sqrt of the sum of the squared values, use quad precision for reduction
	 */
	double reduce_norm2_quad()	const
	{
		double sum = 0.0;
		double c = 0.0;

#if SWEET_THREADING_SPACE
#pragma omp parallel for PROC_BIND_CLOSE reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < number_of_elements; i++)
		{
			double value = scalar_data[i]*scalar_data[i];

			// Use Kahan summation
			double y = value - c;
			double t = sum + y;
			c = (t - sum) - y;
			sum = t;
		}

		sum -= c;

		return std::sqrt(sum);
	}


public:
	/**
	 * assignment operator
	 */
	ScalarDataArray &operator=(double i_value)
	{
		physical_set_all(i_value);

		return *this;
	}


public:
	/**
	 * assignment operator
	 */
	ScalarDataArray &operator=(int i_value)
	{
		physical_set_all(i_value);

		return *this;
	}


public:
	/**
	 * assignment operator
	 */
	ScalarDataArray &operator=(
			const ScalarDataArray &i_data
	)
	{
		number_of_elements = i_data.number_of_elements;

		SCALAR_DATA_FOR_IDX(
				scalar_data[idx] = i_data.scalar_data[idx];
		);

		return *this;
	}


	/**
	 * Compute element-wise addition
	 */
	inline
	ScalarDataArray operator+(
			const ScalarDataArray &i_array_data
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = scalar_data[idx] + i_array_data.scalar_data[idx];
			);

		return out;
	}

	/**
	 * Compute element-wise square root
	 */
	inline
	ScalarDataArray sqrt()	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = std::sqrt(scalar_data[idx]);
			);

		return out;
	}


	/**
	 * Compute element-wise inverse square root
	 */
	inline
	ScalarDataArray inv_sqrt()	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = 1.0/std::sqrt(scalar_data[idx]);
			);

		return out;
	}



	/**
	 * Compute element-wise addition
	 */
	inline
	ScalarDataArray operator+(
			const double i_value
	)	const
	{
		ScalarDataArray out = *this;

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = scalar_data[idx]+i_value;
				);

		return out;
	}



	/**
	 * Compute element-wise addition
	 */
	inline
	ScalarDataArray& operator+=(
			const ScalarDataArray &i_array_data
	)
	{

		SCALAR_DATA_FOR_IDX(
				scalar_data[idx] += i_array_data.scalar_data[idx];
			);

		return *this;
	}



	/**
	 * Compute element-wise addition
	 */
	inline
	ScalarDataArray& operator+=(
			const double i_value
	)
	{



		SCALAR_DATA_FOR_IDX(
			scalar_data[idx] += i_value;
		);

		return *this;
	}



	/**
	 * Compute multiplication with scalar
	 */
	inline
	ScalarDataArray& operator*=(
			const double i_value
	)
	{


		SCALAR_DATA_FOR_IDX(
			scalar_data[idx] *= i_value;
		);

		return *this;
	}



	/**
	 * Compute division with scalar
	 */
	inline
	ScalarDataArray& operator/=(
			const double i_value
	)
	{

		SCALAR_DATA_FOR_IDX(
			scalar_data[idx] /= i_value;
		);
		return *this;
	}



	/**
	 * Compute element-wise subtraction
	 */
	inline
	ScalarDataArray& operator-=(
			const ScalarDataArray &i_array_data
	)
	{
		SCALAR_DATA_FOR_IDX(
				scalar_data[idx] -= i_array_data.scalar_data[idx];
			);

		return *this;
	}


	/**
	 * Compute element-wise subtraction
	 */
	inline
	ScalarDataArray operator-(
			const ScalarDataArray &i_array_data
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = scalar_data[idx]-i_array_data.scalar_data[idx];
				);

		return out;
	}



	/**
	 * Compute element-wise subtraction
	 */
	inline
	ScalarDataArray operator-(
			const double i_value
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(out.scalar_data[idx] = scalar_data[idx]-i_value;)

		return out;
	}



	/**
	 * Compute element-wise subtraction
	 */
	inline
	ScalarDataArray valueMinusThis(
			const double i_value
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = i_value - scalar_data[idx];
			);

		return out;
	}


	/**
	 * Compute element-wise subtraction
	 */
	inline
	ScalarDataArray valueDivThis(
			const double i_value
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = i_value/scalar_data[idx];
			);

		return out;
	}



	/**
	 * Invert sign
	 */
	inline
	ScalarDataArray operator-()
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
			out.scalar_data[idx] = -scalar_data[idx];
		);
		return out;
	}



	/**
	 * Compute element-wise multiplication
	 */
	inline
	ScalarDataArray operator*(
			const ScalarDataArray &i_array_data	///< this class times i_array_data
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = scalar_data[idx]*i_array_data.scalar_data[idx];
			);

		return out;
	}


	/**
	 * Compute element-wise multiplication
	 */
	inline
	ScalarDataArray& operator*=(
			const ScalarDataArray &i_array_data	///< this class times i_array_data
	)
	{
		SCALAR_DATA_FOR_IDX(
				scalar_data[idx] *= i_array_data.scalar_data[idx];
			);

		return *this;
	}



	/**
	 * Compute element-wise division
	 */
	inline
	ScalarDataArray operator/(
			const ScalarDataArray &i_array_data	///< this class times i_array_data
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = scalar_data[idx]/i_array_data.scalar_data[idx];
		);

		return out;
	}



	/**
	 * Compute multiplication with a scalar
	 */
	inline
	ScalarDataArray operator*(
			const double i_value
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
			out.scalar_data[idx] = scalar_data[idx]*i_value;
		);

		return out;
	}


	/**
	 * Compute element-wise division
	 */
	inline
	ScalarDataArray operator/(
			const double &i_value
	)	const
	{
		ScalarDataArray out(number_of_elements);

		SCALAR_DATA_FOR_IDX(
				out.scalar_data[idx] = scalar_data[idx] / i_value;
		);

		return out;
	}





	/**
	 * Print data
	 *
	 * Each array row is stored to a line.
	 * Per default, a tab separator is used in each line to separate the values.
	 */
	bool print(
			int i_precision = 8		///< number of floating point digits
			)	const
	{
		std::ostream &o_ostream = std::cout;

		o_ostream << std::setprecision(i_precision);

		for (std::size_t i = 0; i < number_of_elements; i++)
			o_ostream << scalar_data[i] << std::endl;

		return true;
	}


	friend
	inline
	std::ostream& operator<<(
			std::ostream &o_ostream,
			const ScalarDataArray &i_dataArray
	)
	{
		for (std::size_t i = 0; i < i_dataArray.number_of_elements; i++)
			std::cout << i_dataArray.scalar_data[i] << "\t";

		return o_ostream;
	}

};


/**
 * operator to support operations such as:
 *
 * 1.5 * arrayData;
 *
 * Otherwise, we'd have to write it as arrayData*1.5
 *
 */
inline
static
ScalarDataArray operator*(
		const double i_value,
		const ScalarDataArray &i_array_data
)
{
	return ((ScalarDataArray&)i_array_data)*i_value;
}

/**
 * operator to support operations such as:
 *
 * 1.5 - arrayData;
 *
 * Otherwise, we'd have to write it as arrayData-1.5
 *
 */
inline
static
ScalarDataArray operator-(
		const double i_value,
		const ScalarDataArray &i_array_data
)
{
	return i_array_data.valueMinusThis(i_value);
}

/**
 * operator to support operations such as:
 *
 * 1.5 + arrayData;
 *
 * Otherwise, we'd have to write it as arrayData+1.5
 *
 */
inline
static
ScalarDataArray operator+(
		const double i_value,
		const ScalarDataArray &i_array_data
)
{
	return i_array_data+i_value;
}



inline
static
ScalarDataArray operator/(
		const double i_value,
		const ScalarDataArray &i_array_data
)
{
	return i_array_data.valueDivThis(i_value);
}




#endif /* SRC_DATAARRAY_HPP_ */
