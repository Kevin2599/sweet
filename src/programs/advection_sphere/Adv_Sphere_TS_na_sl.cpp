/*
 * Adv_Sphere_TS_na_sl.cpp
 *
 *  Created on: 29 Mar 2018
 *      Author: Martin Schreiber <M.Schreiber@exeter.ac.uk>
 */

#include "Adv_Sphere_TS_na_sl.hpp"




void Adv_Sphere_TS_na_sl::run_timestep(
		SphereData &io_phi,		///< prognostic variables
		SphereData &io_vort,	///< prognostic variables
		SphereData &io_div,		///< prognostic variables

		double i_fixed_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (i_fixed_dt <= 0)
		FatalError("Only constant time step size allowed");

	double dt = simVars.timecontrol.current_timestep_size;

	// IMPORTANT!!! WE DO NOT USE THE ROBERT TRANSFORMATION HERE!!!
	op.vortdiv_to_uv(io_vort, io_div, diag_u, diag_v);

	if (i_simulation_timestamp == 0)
	{
		diag_u_prev = diag_u;
		diag_v_prev = diag_v;
	}

#if 1


	// OUTPUT: position of departure points at t
	ScalarDataArray posx_d(io_phi.sphereDataConfig->physical_array_data_number_of_elements);
	ScalarDataArray posy_d(io_phi.sphereDataConfig->physical_array_data_number_of_elements);

	semiLagrangian.semi_lag_departure_points_settls(
			diag_u_prev, diag_v_prev,
			diag_u, diag_v,
			posx_a, posy_a,
			dt,
			simVars.sim.earth_radius,
			posx_d, posy_d
	);

	diag_u_prev = diag_u;
	diag_v_prev = diag_v;

	SphereData new_prog_phi(io_phi.sphereDataConfig);

	sampler2D.bicubic_scalar(
			io_phi,
			posx_d,
			posy_d,
			new_prog_phi
	);
#else

	SphereData new_prog_phi(io_phi.sphereDataConfig);

	sampler2D.bicubic_scalar(
//		sampler2D.bilinear_scalar(
			io_phi,
#if 0
			posx_d,
			posy_d,
#else
			posx_a,
			posy_a,
#endif
			new_prog_phi
	);

#endif

	io_phi = new_prog_phi;
}



/*
 * Setup
 */
void Adv_Sphere_TS_na_sl::setup(
		int i_order	///< order of RK time stepping method
)
{
	timestepping_order = i_order;

	const SphereDataConfig *sphereDataConfig = op.sphereDataConfig;

	posx_a.setup(sphereDataConfig->physical_array_data_number_of_elements);
	posy_a.setup(sphereDataConfig->physical_array_data_number_of_elements);

	// setup some test sampling points
	// we use 2 arrays - one for each sampling position

	posx_a.update_lambda_array_indices(
		[&](int idx, double &io_data)
		{
			int i = idx % sphereDataConfig->physical_num_lon;
			//int j = idx / sphereDataConfig->physical_data_size[0];

			io_data = 2.0*M_PI*(double)i/(double)sphereDataConfig->physical_num_lon;
			assert(io_data >= 0);
			assert(io_data < 2.0*M_PI);
		}
	);
	posy_a.update_lambda_array_indices(
			[&](int idx, double &io_data)
		{
			//int i = idx % sphereDataConfig->physical_data_size[0];
			int j = idx / sphereDataConfig->physical_num_lon;

			io_data = sphereDataConfig->lat[j];

			assert(io_data >= -M_PI*0.5);
			assert(io_data <= M_PI*0.5);
		}
	);

#if 0
	posx_a.print();
	std::cout << std::endl;
	posy_a.print();
	exit(1);
#endif

	sampler2D.setup(simVars.sim.domain_size, sphereDataConfig);

	//PXT- This just calls sampler2D.setup, so any reason for having it?
	semiLagrangian.setup(simVars.sim.domain_size, sphereDataConfig);
}


Adv_Sphere_TS_na_sl::Adv_Sphere_TS_na_sl(
		SimulationVariables &i_simVars,
		SphereOperators &i_op
)	:
		simVars(i_simVars),
		op(i_op),

		diag_u(op.sphereDataConfig),
		diag_v(op.sphereDataConfig),

		diag_u_prev(op.sphereDataConfig),
		diag_v_prev(op.sphereDataConfig)
{
	setup(simVars.disc.timestepping_order);
}



Adv_Sphere_TS_na_sl::~Adv_Sphere_TS_na_sl()
{
}

