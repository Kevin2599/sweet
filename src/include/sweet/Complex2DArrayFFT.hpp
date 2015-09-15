/*
 * Complex2DArrayFFT.hpp
 *
 *  Created on: 11 Jul 2015
 *      Author: Martin Schreiber <schreiberx@gmail.com>
 */
#ifndef SRC_INCLUDE_SWEET_COMPLEX2DARRAYFFT_HPP_
#define SRC_INCLUDE_SWEET_COMPLEX2DARRAYFFT_HPP_

#include <fftw3.h>
#include <sweet/DataArray.hpp>
#include <sweet/NUMABlockAlloc.hpp>
#include <cstddef>
#include <complex>
#include <sweet/openmp_helper.hpp>


/**
 * this class is a convenient handler for arrays with complex numbers
 * to convert them to Fourier space forward and backward.
 *
 * In contrast to the DataArray class, this class also supports
 * complex numbers in Cartesian space.
 */
class Complex2DArrayFFT
{
	typedef std::complex<double> complex;

public:
	/**
	 * resolution of data array
	 */
	std::size_t resolution[2];

	/**
	 * Is this an aliased version?
	 * Then this uses different FFTW plans
	 */
	bool aliased_scaled = false;

	/**
	 * Data associated to this complex array
	 */
	double *data;

	/**
	 * Flag which is true if this is the initializer class of the FFTW library
	 */
	bool is_data_initialized;


	/**
	 * container for different plans supported by this class
	 */
	class Plans
	{
	public:
		fftw_plan to_cart;
		fftw_plan to_spec;
		fftw_plan to_cart_aliasing;
		fftw_plan to_spec_aliasing;
	};


	/**
	 * Singleton of plans
	 */
	Plans &fft_getSingleton_Plans()
	{
		static Plans plans;
		return plans;
	}


	/**
	 * Singleton of reference counter
	 */
	int &fft_getSingleton_RefCounter()
	{
		static int ref_counter = 0;
		return ref_counter;
	}



	/**
	 * Setup the FFTW
	 *
	 * This may be only called once
	 */
	void fft_setup()
	{
		is_data_initialized = true;

		assert(fft_getSingleton_RefCounter() >= 0);

		int &ref_counter = fft_getSingleton_RefCounter();
#if SWEET_REXI_PARALLEL_SUM
#	pragma omp atomic
#endif
		ref_counter++;

		if (ref_counter != 1)
			return;

		{
			// create dummy array for plan creation
			// IMPORTANT! if we use the same array for input/output,
			// a plan will be created with does not support out-of-place
			// FFTs, see http://www.fftw.org/doc/New_002darray-Execute-Functions.html
			double *dummy_data = NUMABlockAlloc::alloc<double>(sizeof(double)*resolution[0]*resolution[1]*2);

			fft_getSingleton_Plans().to_spec =
					fftw_plan_dft_2d(
						resolution[1],	// n0 = ny
						resolution[0],	// n1 = nx
						(fftw_complex*)data,
						(fftw_complex*)dummy_data,
						FFTW_FORWARD,
						FFTW_PRESERVE_INPUT
					);

			if (fft_getSingleton_Plans().to_spec == nullptr)
			{
				std::cerr << "Failed to create plan_forward for fftw" << std::endl;
				exit(-1);
			}

			fft_getSingleton_Plans().to_cart =
					fftw_plan_dft_2d(
						resolution[1],	// n0 = ny
						resolution[0],	// n1 = nx
						(fftw_complex*)data,
						(fftw_complex*)dummy_data,
						FFTW_BACKWARD,
						FFTW_PRESERVE_INPUT
					);

			if (fft_getSingleton_Plans().to_cart == nullptr)
			{
				std::cerr << "Failed to create plan_backward for fftw" << std::endl;
				exit(-1);
			}

			NUMABlockAlloc::free(dummy_data, sizeof(double)*resolution[0]*resolution[1]*2);
		}

		{
			double *dummy_data_aliasing_in = NUMABlockAlloc::alloc<double>(sizeof(double)*resolution[0]*resolution[1]*2*4);
			double *dummy_data_aliasing_out = NUMABlockAlloc::alloc<double>(sizeof(double)*resolution[0]*resolution[1]*2*4);

			fft_getSingleton_Plans().to_spec_aliasing =
					fftw_plan_dft_2d(
						resolution[1]*2,	// n0 = ny
						resolution[0]*2,	// n1 = nx
						(fftw_complex*)dummy_data_aliasing_in,
						(fftw_complex*)dummy_data_aliasing_out,
						FFTW_FORWARD,
						FFTW_PRESERVE_INPUT
					);

			if (fft_getSingleton_Plans().to_spec_aliasing == nullptr)
			{
				std::cerr << "Failed to create plan_forward for fftw" << std::endl;
				exit(-1);
			}


			fft_getSingleton_Plans().to_cart_aliasing =
					fftw_plan_dft_2d(
						resolution[1]*2,	// n0 = ny
						resolution[0]*2,	// n1 = nx
						(fftw_complex*)dummy_data_aliasing_out,
						(fftw_complex*)dummy_data_aliasing_in,
						FFTW_BACKWARD,
						FFTW_PRESERVE_INPUT
					);

			if (fft_getSingleton_Plans().to_cart_aliasing == nullptr)
			{
				std::cerr << "Failed to create plan_backward for fftw" << std::endl;
				exit(-1);
			}


			NUMABlockAlloc::free(dummy_data_aliasing_out, sizeof(double)*resolution[0]*resolution[1]*2*4);
			NUMABlockAlloc::free(dummy_data_aliasing_in, sizeof(double)*resolution[0]*resolution[1]*2*4);
		}
	}

	void fftw_shutdown()
	{
		if (!is_data_initialized)
			return;

		int &ref_counter = fft_getSingleton_RefCounter();
#if SWEET_REXI_PARALLEL_SUM
#	pragma omp atomic
#endif
		ref_counter--;

		if (ref_counter > 0)
			return;

		assert(ref_counter >= 0);

		fftw_destroy_plan(fft_getSingleton_Plans().to_spec);
		fftw_destroy_plan(fft_getSingleton_Plans().to_cart);

		fftw_destroy_plan(fft_getSingleton_Plans().to_spec_aliasing);
		fftw_destroy_plan(fft_getSingleton_Plans().to_cart_aliasing);
	}



	Complex2DArrayFFT()	:
		data(nullptr),
		is_data_initialized(false)
	{

#if !SWEET_USE_LIBFFT
		std::cerr << "This class only makes sense with FFT" << std::endl;
		exit(1);
#endif

	}



public:
	Complex2DArrayFFT(
			const std::size_t i_res[2],
			bool i_aliased_scaled = false
	)	:
		is_data_initialized(false)
	{

#if !SWEET_USE_LIBFFT
		std::cerr << "This class only makes sense with FFT" << std::endl;
		exit(1);
#endif

		aliased_scaled = i_aliased_scaled;

		resolution[0] = i_res[0];
		resolution[1] = i_res[1];

		data = NUMABlockAlloc::alloc<double>(sizeof(double)*resolution[0]*resolution[1]*2);

		fft_setup();
	}


#if 0
	/**
	 * allocator which allocated memory blocks aligned at 128 byte boundaries
	 */
public:
	template <typename T=void>
	static
	T *alloc(
			std::size_t i_size
	)
	{
		T *data;

		// posix_memalign is thread safe
		// http://www.qnx.com/developers/docs/6.3.0SP3/neutrino/lib_ref/p/posix_memalign.html
		int retval = posix_memalign((void**)&data, 128, i_size);
		if (retval != 0)
		{
			std::cerr << "Unable to allocate memory" << std::endl;
			assert(false);
			exit(-1);
		}
		return data;
	}


	/**
	 * Free memory which was previously allocated
	 */
public:
	template <typename T>
	void free(T *i_ptr)
	{
		free(i_ptr);
	}
#endif


public:
	void setup(
			const std::size_t i_res[2],
			bool i_aliased_scaled = false
	)
	{
		aliased_scaled = i_aliased_scaled;

		resolution[0] = i_res[0];
		resolution[1] = i_res[1];

		data = NUMABlockAlloc::alloc<double>(sizeof(double)*resolution[0]*resolution[1]*2);

		fft_setup();
	}


public:
	Complex2DArrayFFT(
			const Complex2DArrayFFT &i_testArray
	)	:
		is_data_initialized(false)
	{
#if !SWEET_USE_LIBFFT
		std::cerr << "This class only makes sense with FFT" << std::endl;
		exit(1);
#endif

		resolution[0] = i_testArray.resolution[0];
		resolution[1] = i_testArray.resolution[1];

		aliased_scaled = i_testArray.aliased_scaled;

		data = NUMABlockAlloc::alloc<double>(sizeof(double)*resolution[0]*resolution[1]*2);

		fft_setup();

		par_doublecopy(data, i_testArray.data, resolution[0]*resolution[1]*2);
	}


	inline
	void par_doublecopy(double *o_data, double *i_data, std::size_t i_size)
	{
#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < i_size; i++)
			o_data[i] = i_data[i];
	}

	~Complex2DArrayFFT()
	{
		fftw_shutdown();

		NUMABlockAlloc::free(data, sizeof(double)*resolution[0]*resolution[1]*2);
	}



public:
	Complex2DArrayFFT& operator=(
			const Complex2DArrayFFT &i_testArray
	)
	{
		assert(resolution[0] == i_testArray.resolution[0]);
		assert(resolution[1] == i_testArray.resolution[1]);

		par_doublecopy(data, i_testArray.data, resolution[0]*resolution[1]*2);
		return *this;
	}



	Complex2DArrayFFT toSpec()
	{
		Complex2DArrayFFT o_testArray(resolution, aliased_scaled);

		if (aliased_scaled)
		{
			fftw_execute_dft(
					fft_getSingleton_Plans().to_spec_aliasing,
					(fftw_complex*)this->data,
					(fftw_complex*)o_testArray.data
				);
		}
		else
		{
			fftw_execute_dft(
					fft_getSingleton_Plans().to_spec,
					(fftw_complex*)this->data,
					(fftw_complex*)o_testArray.data
				);
		}

		return o_testArray;
	}


	Complex2DArrayFFT toCart()
	{
		Complex2DArrayFFT o_testArray(resolution, aliased_scaled);

		if (aliased_scaled)
		{
			fftw_execute_dft(
					fft_getSingleton_Plans().to_cart_aliasing,
					(fftw_complex*)this->data,
					(fftw_complex*)o_testArray.data
				);
		}
		else
		{
			fftw_execute_dft(
					fft_getSingleton_Plans().to_cart,
					(fftw_complex*)this->data,
					(fftw_complex*)o_testArray.data
				);
		}

		/*
		 * to the scaling only if we convert the data back to cartesian space
		 */
		double scale = (1.0/((double)resolution[0]*(double)resolution[1]));
#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			o_testArray.data[i] *= scale;
			o_testArray.data[i+1] *= scale;
		}

		return o_testArray;
	}


	void set(int y, int x, double re, double im)
	{
		data[(y*resolution[0]+x)*2+0] = re;
		data[(y*resolution[0]+x)*2+1] = im;
	}


	void set(int y, int x, const std::complex<double> &i_value)
	{
		data[(y*resolution[0]+x)*2+0] = i_value.real();
		data[(y*resolution[0]+x)*2+1] = i_value.imag();
	}


	void setRe(int y, int x, double re)
	{
		data[(y*resolution[0]+x)*2+0] = re;
	}

	void setIm(int y, int x, double im)
	{
		data[(y*resolution[0]+x)*2+1] = im;
	}

	double getRe(int y, int x)	const
	{
		return data[(y*resolution[0]+x)*2+0];
	}

	double getIm(int y, int x)	const
	{
		return data[(y*resolution[0]+x)*2+1];
	}

	complex get(int y, int x)	const
	{
		std::size_t idx = (y*resolution[0]+x)*2;

		return complex(
				data[idx+0],
				data[idx+1]
				);
	}

	void setAll(double re, double im)
	{
#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			data[i] = re;
			data[i+1] = im;
		}
	}

	void setAll(
			std::complex<double> &i_value
	)
	{
		double re = i_value.real();
		double im = i_value.imag();

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			data[i] = re;
			data[i+1] = im;
		}
	}

	void setAllRe(double re)
	{
#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			data[i] = re;
		}
	}

	void setAllIm(double im)
	{
#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			data[i+1] = im;
		}
	}

#if 0
	/**
	 * return |u|^2 with
	 */
	double getNormSquared(int y, int x)	const
	{
		double re = data[(y*resolution[0]+x)*2+0];
		double im = data[(y*resolution[0]+x)*2+1];

		return std::abs(re*re+im*im);
	}


	/**
	 * compute
	 *
	 * int(x*value(x), x=0..inf)/int(x, x=0..inf)
	 */
	double return_centroidFromEnergy()
	{
		double nominator = 0;
		double denominator = 0;

		for (std::size_t kb = 0; kb < resolution[1]; kb++)
		{
//			for (std::size_t ka = 0; ka < resolution[0]; ka++)
//			{
				double k = std::sqrt((double)(kb*kb));
//				double k = std::sqrt((double)(ka*ka)+(double)(kb*kb));
//				double k = ka+kb;

//				double energy_ampl = std::sqrt(getNormSquared(kb, ka));
				double energy_ampl = std::sqrt(getNormSquared(kb, 0));

				nominator += k*energy_ampl;
				denominator += energy_ampl;
//			}
		}

		std::cout << nominator << std::endl;
		return nominator/denominator;
//		return nominator/(double)(resolution[0]*resolution[1]);
	}

	/**
	 * return the energy centroid with the velocity components given
	 * by this class and the parameter
	 */
	double return_energyCentroid(
			Complex2DArrayFFT &v
	)
	{
		Complex2DArrayFFT &u = *this;

		double nominator = 0;
		double denominator = 0;

		for (std::size_t kb = 0; kb < resolution[1]; kb++)
			for (std::size_t ka = 0; ka < resolution[0]; ka++)
			{
				double k = std::sqrt((double)(ka*ka)+(double)(kb*kb));

				double u2 = u.getNormSquared(kb, ka);
				double v2 = v.getNormSquared(kb, ka);

				double ampl = u2+v2;
				nominator += k*ampl;
				denominator += ampl;
			}

		return nominator/denominator;
	}
#endif


public:
	void op_setup_diff_x(
			const double i_domain_size[2],
			bool i_use_finite_difference = false
	)
	{
		setAll(0,0);

		if (i_use_finite_difference)
		{
			double h[2] = {(double)i_domain_size[0] / (double)resolution[0], (double)i_domain_size[1] / (double)resolution[1]};

			/*
			 * setup FD operator
			 */
			set(0, 1, -1.0/(2.0*h[0]), 0);
			set(0, resolution[0]-1, 1.0/(2.0*h[0]), 0);

			*this = this->toSpec();
			// TODO: maybe set highest modes to zero?
		}
		else
		{
			double scale = 2.0*M_PIl/i_domain_size[0];

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
			for (std::size_t j = 0; j < resolution[1]/2; j++)
			{
				for (std::size_t i = 1; i < resolution[0]/2; i++)
				{
					set(	j,
							i,
							0,
							(double)i*scale
						);
					set(
							resolution[1]-1-j,
							i,
							0,
							(double)i*scale
						);

					set(	j,
							resolution[0]-i,
							0,
							-(double)i*scale
						);
					set(
							resolution[1]-1-j,
							resolution[0]-i,
							0,
							-(double)i*scale
						);
				}
			}
		}
	}



public:
	void op_setup_diff2_x(
			const double i_domain_size[2],
			bool i_use_finite_difference = false
	)
	{
		setAll(0,0);


		if (i_use_finite_difference)
		{
			double h[2] = {(double)i_domain_size[0] / (double)resolution[0], (double)i_domain_size[1] / (double)resolution[1]};

			/*
			 * setup FD operator
			 */
			set(0, 1, 1.0/(h[0]*h[0]), 0);
			set(0, 0, -2.0/(h[0]*h[0]), 0);
			set(0, resolution[0]-1, 1.0/(h[0]*h[0]), 0);

			*this = this->toSpec();
			// TODO: maybe set highest modes to zero?
		}
		else
		{
			double scale = 2.0*M_PIl/i_domain_size[0];

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
			for (std::size_t j = 0; j < resolution[1]/2; j++)
			{
				for (std::size_t i = 1; i < resolution[0]/2; i++)
				{
					set(	j,
							i,
							-(double)i*scale*(double)i*scale,
							0
						);
					set(
							resolution[1]-1-j,
							i,
							-(double)i*scale*(double)i*scale,
							0
						);

					set(	j,
							resolution[0]-i,
							-(double)i*scale*(double)i*scale,
							0
						);
					set(
							resolution[1]-1-j,
							resolution[0]-i,
							-(double)i*scale*(double)i*scale,
							0
						);
				}
			}
		}
	}



public:
	void op_setup_diff_y(
			const double i_domain_size[2],
			bool i_use_finite_difference = false
	)
	{
		setAll(0,0);

		if (i_use_finite_difference)
		{
			double h[2] = {(double)i_domain_size[0] / (double)resolution[0], (double)i_domain_size[1] / (double)resolution[1]};

			/*
			 * setup FD operator
			 */
			set(1, 0, -1.0/(2.0*h[1]), 0);
			set(resolution[1]-1, 0, 1.0/(2.0*h[1]), 0);

			*this = this->toSpec();
			// TODO: maybe set highest modes to zero?
		}
		else
		{
			double scale = 2.0*M_PIl/i_domain_size[1];

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
			for (int j = 1; j < (int)resolution[1]/2; j++)
			{
				for (int i = 0; i < (int)resolution[0]/2; i++)
				{
					set(
							j,
							i,
							0,
							(double)j*scale
						);
					set(
							resolution[1]-j,
							i,
							0,
							-(double)j*scale
						);

					set(
							j,
							resolution[0]-i-1,
							0,
							(double)j*scale
						);
					set(
							resolution[1]-j,
							resolution[0]-i-1,
							0,
							-(double)j*scale
						);
				}
			}
		}
	}


public:
	void op_setup_diff2_y(
			const double i_domain_size[2],
			bool i_use_finite_difference = false
	)
	{
		setAll(0,0);


		if (i_use_finite_difference)
		{
			double h[2] = {(double)i_domain_size[0] / (double)resolution[0], (double)i_domain_size[1] / (double)resolution[1]};

			/*
			 * setup FD operator
			 */
			set(1, 0, 1.0/(h[1]*h[1]), 0);
			set(0, 0, -2.0/(h[1]*h[1]), 0);
			set(resolution[1]-1, 0, 1.0/(h[1]*h[1]), 0);

			*this = this->toSpec();
			// TODO: maybe set highest modes to zero?
		}
		else
		{
			double scale = 2.0*M_PIl/i_domain_size[1];

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
			for (int j = 1; j < (int)resolution[1]/2; j++)
			{
				for (int i = 0; i < (int)resolution[0]/2; i++)
				{
					set(
							j,
							i,
							-(double)j*scale*(double)j*scale,
							0
						);
					set(
							resolution[1]-j,
							i,
							-(double)j*scale*(double)j*scale,
							0
						);

					set(
							j,
							resolution[0]-i-1,
							-(double)j*scale*(double)j*scale,
							0
						);
					set(
							resolution[1]-j,
							resolution[0]-i-1,
							-(double)j*scale*(double)j*scale,
							0
						);
				}
			}
		}
	}


	/**
	 * return the maximum of all absolute values
	 */
	double reduce_sumAbs()	const
	{
		double sum = 0;
#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for reduction(+:sum)
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
			sum += std::abs(data[i*2])+std::abs(data[i*2+1]);

		return sum;
	}


	friend
	inline
	std::ostream& operator<<(std::ostream &o_ostream, const Complex2DArrayFFT &i_testArray)
	{
		for (int y = i_testArray.resolution[1]-1; y >= 0; y--)
		{
			for (std::size_t x = 0; x < i_testArray.resolution[0]; x++)
			{
				double value_re = i_testArray.getRe(y, x);
				double value_im = i_testArray.getIm(y, x);
#if 1
				if (std::abs(value_re) < 10e-10)
					value_re = 0;
				if (std::abs(value_im) < 10e-10)
					value_im = 0;
#endif
				o_ostream << "(" << value_re << ", " << value_im << ")\t";
			}
			o_ostream << std::endl;
		}
		return o_ostream;
	}



	Complex2DArrayFFT spec_div_element_wise(
			const Complex2DArrayFFT &i_d,
			double i_instability_threshold = 0
	)
	{
		Complex2DArrayFFT out(resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			double ar = data[i];
			double ai = data[i+1];
			double br = i_d.data[i];
			double bi = i_d.data[i+1];

			double den = (br*br+bi*bi);

			if (std::abs(den) < i_instability_threshold)
			{
				std::cerr << "Instability via division by 0 detected (" << den << ")" << std::endl;
				exit(1);
			}

			if (std::abs(den) == 0)
			{
				// For Laplace solution, this is the integration constant C
				out.data[i] = 0;
				out.data[i+1] = 0;
			}
			else
			{
				out.data[i] = (ar*br + ai*bi)/den;
				out.data[i+1] = (ai*br - ar*bi)/den;
			}
		}

		return out;
	}



	DataArray<2> getRealWithDataArray()
	{
		DataArray<2> out(resolution);

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t j = 0; j < resolution[1]; j++)
			for (std::size_t i = 0; i < resolution[0]; i++)
				out.set(j, i, getRe(j, i));

		return out;
	}

	DataArray<2> getImagWithDataArray()
	{
		DataArray<2> out(resolution);

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t j = 0; j < resolution[1]; j++)
			for (std::size_t i = 0; i < resolution[0]; i++)
				out.set(j, i, getIm(j, i));

		return out;
	}



	Complex2DArrayFFT &loadRealFromDataArray(
			const DataArray<2> &i_dataArray_Real
	)
	{
		i_dataArray_Real.requestDataInCartesianSpace();

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t j = 0; j < resolution[1]; j++)
			for (std::size_t i = 0; i < resolution[0]; i++)
				set(j, i, i_dataArray_Real.get(j, i), 0);

		return *this;
	}


	Complex2DArrayFFT &loadRealAndImagFromDataArrays(
			const DataArray<2> &i_dataArray_Real,
			const DataArray<2> &i_dataArray_Imag
	)
	{
		i_dataArray_Real.requestDataInCartesianSpace();
		i_dataArray_Imag.requestDataInCartesianSpace();

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t j = 0; j < resolution[1]; j++)
			for (std::size_t i = 0; i < resolution[0]; i++)
				set(	j, i,
						i_dataArray_Real.get(j, i),
						i_dataArray_Imag.get(j, i)
				);

		return *this;
	}



	/**
	 * Apply a linear operator given by this class to the input data array.
	 */
	inline
	Complex2DArrayFFT operator()(
			const Complex2DArrayFFT &i_array_data
	)	const
	{
		Complex2DArrayFFT out(resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			double ar = data[i];
			double ai = data[i+1];
			double br = i_array_data.data[i];
			double bi = i_array_data.data[i+1];

			out.data[i] = ar*br - ai*bi;
			out.data[i+1] = ar*bi + ai*br;
		}

		return out;
	}


	/**
	 * Compute multiplication with a complex scalar
	 */
	inline
	Complex2DArrayFFT operator*(
			const std::complex<double> &i_value
	)	const
	{
		Complex2DArrayFFT out(resolution, aliased_scaled);

		double br = i_value.real();
		double bi = i_value.imag();

#if !SWEET_REXI_PARALLEL_SUM
#		pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			double ar = data[i];
			double ai = data[i+1];

			out.data[i] = ar*br - ai*bi;
			out.data[i+1] = ar*bi + ai*br;
		}
		return out;
	}



	/**
	 * Compute element-wise addition
	 */
	inline
	Complex2DArrayFFT operator+(
			const Complex2DArrayFFT &i_array_data
	)	const
	{
		Complex2DArrayFFT out(resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			out.data[i] = data[i] + i_array_data.data[i];
			out.data[i+1] = data[i+1] + i_array_data.data[i+1];
		}

		return out;
	}




	/**
	 * Compute element-wise addition
	 */
	inline
	Complex2DArrayFFT& operator+=(
			const Complex2DArrayFFT &i_array_data
	)
	{
#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			data[i] += i_array_data.data[i];
			data[i+1] += i_array_data.data[i+1];
		}

		return *this;
	}



	/**
	 * Compute element-wise subtraction
	 */
	inline
	Complex2DArrayFFT operator-(
			const Complex2DArrayFFT &i_array_data
	)	const
	{
		Complex2DArrayFFT out(this->resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			out.data[i] = data[i] - i_array_data.data[i];
			out.data[i+1] = data[i+1] - i_array_data.data[i+1];
		}

		return out;
	}



	/**
	 * Compute addition with complex value
	 */
	inline
	Complex2DArrayFFT addScalar_Spec(
			const complex &i_value
	)	const
	{
		Complex2DArrayFFT out(this->resolution, aliased_scaled);


#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			out.data[i] = data[i];
			out.data[i+1] = data[i+1];
		}

		double scale = resolution[0]*resolution[1];
//		double scale = 1.0;
		out.data[0] += i_value.real()*scale;
		out.data[1] += i_value.imag()*scale;

		return out;
	}



	/**
	 * Compute addition with complex value
	 */
	inline
	Complex2DArrayFFT addScalar_Cart(
			const complex &i_value
	)	const
	{
		Complex2DArrayFFT out(this->resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			out.data[i] = data[i]+i_value.real();
			out.data[i+1] = data[i+1]+i_value.imag();
		}

		return out;
	}



	/**
	 * Compute subtraction with complex value
	 */
	inline
	Complex2DArrayFFT subScalar_Spec(
			const complex &i_value
	)	const
	{
		Complex2DArrayFFT out(this->resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			out.data[i] = data[i];
			out.data[i+1] = data[i+1];
		}

		double scale = resolution[0]*resolution[1];
		out.data[0] -= i_value.real()*scale;
		out.data[1] -= i_value.imag()*scale;

		return out;
	}



	/**
	 * Compute addition with complex value
	 */
	inline
	Complex2DArrayFFT subScalar_Cart(
			const complex &i_value
	)	const
	{
		Complex2DArrayFFT out(this->resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			out.data[i] = data[i]-i_value.real();
			out.data[i+1] = data[i+1]-i_value.imag();
		}

		return out;
	}



	/**
	 * Compute element-wise multiplication
	 */
	inline
	Complex2DArrayFFT operator*(
			const Complex2DArrayFFT &i_array_data
	)	const
	{
		Complex2DArrayFFT out(i_array_data.resolution, aliased_scaled);

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for OPENMP_SIMD
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			complex a = complex(data[i], data[i+1]);
			complex b = complex(i_array_data.data[i], i_array_data.data[i+1]);

			complex c = a*b;
			out.data[i+0] = c.real();
			out.data[i+1] = c.imag();
		}

		return out;
	}



	/**
	 * reduce to root mean square
	 */
	double reduce_rms_quad()
	{
		double sum = 0;
		double c = 0;

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			double radius2 = data[i]*data[i]+data[i+1]*data[i+1];
			//double value = std::sqrt(radius2);
			//value *= value;
			double value = radius2;

			// Use Kahan summation
			double y = value - c;
			double t = sum + y;
			c = (t - sum) - y;
			sum = t;
		}

		sum -= c;

		sum = std::sqrt(sum/double(resolution[0]*resolution[1]));
		return sum;
	}



	/**
	 * return the maximum of all absolute values, use quad precision for reduction
	 */
	double reduce_sum_re_quad()	const
	{
		double sum = 0;
		double c = 0;

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			double value = data[i];

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
	 * return the maximum of all absolute values, use quad precision for reduction
	 */
	double reduce_norm2_quad()	const
	{
		double sum = 0.0;
		double c = 0.0;

#if !SWEET_REXI_PARALLEL_SUM
		#pragma omp parallel for reduction(+:sum,c)
#endif
		for (std::size_t i = 0; i < resolution[0]*resolution[1]*2; i+=2)
		{
			double value = data[i]*data[i]+data[i+1]*data[i+1];

			// Use Kahan summation
			double y = value - c;
			double t = sum + y;
			c = (t - sum) - y;
			sum = t;
		}

		sum -= c;

		return std::sqrt(sum);
	}

};


inline
static
Complex2DArrayFFT operator*(
		const std::complex<double> &i_value,
		const Complex2DArrayFFT &i_array_data
)
{
	Complex2DArrayFFT out(i_array_data.resolution, i_array_data.aliased_scaled);

	double br = i_value.real();
	double bi = i_value.imag();

#if !SWEET_REXI_PARALLEL_SUM
	#pragma omp parallel for OPENMP_SIMD
#endif
	for (std::size_t i = 0; i < i_array_data.resolution[0]*i_array_data.resolution[1]*2; i+=2)
	{
		double ar = i_array_data.data[i];
		double ai = i_array_data.data[i+1];

		out.data[i] = ar*br - ai*bi;
		out.data[i+1] = ar*bi + ai*br;
	}
	return out;
}

#endif /* SRC_INCLUDE_SWEET_COMPLEX2DARRAYFFT_HPP_ */
