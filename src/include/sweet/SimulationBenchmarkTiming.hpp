/*
 * SimulationBenchmarkTiming.hpp
 *
 *  Created on: 23 Sep 2018
 *      Author: Martin Schreiber
 */



#include <sweet/Stopwatch.hpp>

class SimulationBenchmarkTimings
{
public:
	Stopwatch main;
	Stopwatch main_setup;
	Stopwatch main_timestepping;

#if SWEET_REXI_TIMINGS
	// TODO: call SWEET_REXI_TIMINGS somehow different, e.g. SWEET_BENCHMARK_MICRO_TIMINGS
	Stopwatch main_timestepping_nonlinearities;

	Stopwatch rexi;
	Stopwatch rexi_setup;
	Stopwatch rexi_shutdown;
	Stopwatch rexi_timestepping;
	Stopwatch rexi_timestepping_solver;
	Stopwatch rexi_timestepping_broadcast;
	Stopwatch rexi_timestepping_reduce;
	Stopwatch rexi_timestepping_miscprocessing;
#endif

	static SimulationBenchmarkTimings& getInstance()
	{
		static SimulationBenchmarkTimings instance;
		return instance;
	}

	void reset()
	{
		main.reset();
		main_setup.reset();
		main_timestepping.reset();
#if SWEET_REXI_TIMINGS
		main_timestepping_nonlinearities.reset();


		rexi.reset();
		rexi_setup.reset();
		rexi_shutdown.reset();
		rexi_timestepping.reset();
		rexi_timestepping_solver.reset();
		rexi_timestepping_broadcast.reset();
		rexi_timestepping_reduce.reset();
		rexi_timestepping_miscprocessing.reset();
#endif
	}


	void output()
	{
		if (main() != 0 || main_setup() != 0 || main_timestepping() != 0)
		{
			std::cout << "[MULE] simulation_benchmark_timings.main: " << main() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.main_setup: " << main_setup() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.main_timestepping: " << main_timestepping() << std::endl;
#if SWEET_REXI_TIMINGS
			std::cout << "[MULE] simulation_benchmark_timings.main_timestepping_nonlinearities: " << main_timestepping_nonlinearities() << std::endl;
#endif
		}

#if SWEET_REXI_TIMINGS
		if (
				rexi() != 0 ||
				rexi_setup() != 0 ||
				rexi_shutdown() != 0 ||
				rexi_timestepping() != 0 ||
				rexi_timestepping_solver() != 0 ||
				rexi_timestepping_broadcast() != 0 ||
				rexi_timestepping_reduce() != 0 ||
				rexi_timestepping_miscprocessing() != 0
		)
		{
			std::cout << "[MULE] simulation_benchmark_timings.rexi: " << rexi() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_setup: " << rexi_setup() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_shutdown: " << rexi_shutdown() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_timestepping: " << rexi_timestepping() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_timestepping_solver: " << rexi_timestepping_solver() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_timestepping_broadcast: " << rexi_timestepping_broadcast() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_timestepping_reduce: " << rexi_timestepping_reduce() << std::endl;
			std::cout << "[MULE] simulation_benchmark_timings.rexi_timestepping_miscprocessing: " << rexi_timestepping_miscprocessing() << std::endl;
		}
#endif
	}


	SimulationBenchmarkTimings()
	{
		reset();
	}
};
