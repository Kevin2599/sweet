/*
 * SWE_Sphere_TS_l_rexi_n_erk.cpp
 *
 *  Created on: 1 Oct 2017
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#include "SWE_Sphere_TS_l_rexi_n_erk_ver01.hpp"



void SWE_Sphere_TS_l_rexi_n_erk::run_timestep(
		SphereData_Spectral &io_phi,		///< prognostic variables
		SphereData_Spectral &io_vort,	///< prognostic variables
		SphereData_Spectral &io_div,		///< prognostic variables

		double i_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (timestepping_order2 == 1)
	{
		if (version_id == 0)
		{
			// first order REXI for linear part
			timestepping_l_rexi.run_timestep(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);

			// first order explicit for non-linear
			timestepping_l_erk_n_erk.euler_timestep_update_n(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);
		}
		else if (version_id == 1)
		{
			// first order explicit for non-linear
			timestepping_l_erk_n_erk.euler_timestep_update_n(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);

			// first order REXI for linear part
			timestepping_l_rexi.run_timestep(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);
		}
		else
		{
			FatalError("Invalid version id");
		}
	}
	else if (timestepping_order2 == 2 || timestepping_order2 == 4)
	{
		if (version_id == 0)
		{
			// HALF time step for linear part
			timestepping_l_rexi.run_timestep(
					io_phi, io_vort, io_div,
					i_dt*0.5,
					i_simulation_timestamp
				);

			// FULL time step for non-linear part
			timestepping_rk_nonlinear.run_timestep(
					&timestepping_l_erk_n_erk,
					&SWE_Sphere_TS_l_erk_n_erk::euler_timestep_update_n,	///< pointer to function to compute euler time step updates
					io_phi, io_vort, io_div,
					i_dt,
					timestepping_order,		/// This must be 2nd order accurate to get overall 2nd order accurate method
					i_simulation_timestamp
				);

			// HALF time step for linear part
			timestepping_l_rexi.run_timestep(
					io_phi, io_vort, io_div,
					i_dt*0.5,
					i_simulation_timestamp+i_dt*0.5	/* TODO: CHECK THIS, THIS MIGHT BE WRONG!!! */
				);

		}
		else if (version_id == 1)
		{

			// HALF time step for non-linear part
			timestepping_rk_nonlinear.run_timestep(
					&timestepping_l_erk_n_erk,
					&SWE_Sphere_TS_l_erk_n_erk::euler_timestep_update_n,	///< pointer to function to compute euler time step updates
					io_phi, io_vort, io_div,
					i_dt*0.5,
					timestepping_order,		/// This must be 2nd order accurate to get overall 2nd order accurate method
					i_simulation_timestamp
				);

			// FULL time step for linear part
			timestepping_l_rexi.run_timestep(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);

			// HALF time step for non-linear part
			timestepping_rk_nonlinear.run_timestep(
					&timestepping_l_erk_n_erk,
					&SWE_Sphere_TS_l_erk_n_erk::euler_timestep_update_n,	///< pointer to function to compute euler time step updates
					io_phi, io_vort, io_div,
					i_dt*0.5,
					timestepping_order,		/// This must be 2nd order accurate to get overall 2nd order accurate method
					i_simulation_timestamp
				);
		}
		else
		{
			FatalError("Invalid version id");
		}
	}
	else
	{
		FatalError("Not yet supported!");
	}
}



/*
 * Setup
 */
void SWE_Sphere_TS_l_rexi_n_erk::setup(
		REXI_SimulationVariables &i_rexiSimVars,
		int i_order,	///< order of RK time stepping method
		int i_order2,	///< order of RK time stepping method of non-linear parts
		double i_timestep_size,
		bool i_use_f_sphere,
		int i_version_id
)
{
	version_id = i_version_id;

	timestepping_order = i_order;
	timestepping_order2 = i_order2;

	if (timestepping_order != timestepping_order2)
		FatalError("Mismatch of orders, should be equal");

	timestep_size = i_timestep_size;

	if (timestepping_order2 == 1)
	{
		timestepping_l_rexi.setup(
				i_rexiSimVars,
				"phi0",
				i_timestep_size,
				i_use_f_sphere,
				false
		);
	}
	else if (timestepping_order2 == 2 || timestepping_order2 == 4)
	{
		if (version_id == 0)
		{
			timestepping_l_rexi.setup(
					i_rexiSimVars,
					"phi0",
					i_timestep_size*0.5,
					i_use_f_sphere,
					false
			);
		}
		else if (version_id == 1)
		{
			timestepping_l_rexi.setup(
					i_rexiSimVars,
					"phi0",
					i_timestep_size,
					i_use_f_sphere,
					false
			);
		}
		else
		{
			FatalError("Invalid version");
		}
	}
	else
	{
		FatalError("Invalid timestepping order");
	}


	//
	// Only request 1st order time stepping methods for irk and erk
	// These 1st order methods will be combined to higher-order methods in this class
	//
	timestepping_l_erk_n_erk.setup(1, 1);
}



SWE_Sphere_TS_l_rexi_n_erk::SWE_Sphere_TS_l_rexi_n_erk(
		SimulationVariables &i_simVars,
		SphereOperators_SphereData &i_op
)	:
		simVars(i_simVars),
		op(i_op),
		timestepping_l_rexi(simVars, op),
		timestepping_l_erk_n_erk(simVars, op),
		version_id(0),
		timestepping_order(-1)
{
//	setup(simVars.disc.timestepping_order);
}



SWE_Sphere_TS_l_rexi_n_erk::~SWE_Sphere_TS_l_rexi_n_erk()
{
}

