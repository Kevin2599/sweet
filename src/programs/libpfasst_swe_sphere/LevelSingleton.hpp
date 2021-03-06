#ifndef _LEVEL_SINGLETON_CTX_HPP_
#define _LEVEL_SINGLETON_CTX_HPP_

#include <benchmarks_sphere/SWESphereBenchmarksCombined.hpp>
#include <sweet/sphere/SphereData_Spectral.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>

// Class to store the configurations and operators at each level

class LevelSingleton
{

public:
  
  int              level;
  SphereData_Config dataConfig;
  SphereData_Config dataConfigNoDealiasing;
  SphereOperators_SphereData  op;
  SphereOperators_SphereData  opNoDealiasing;

  SWESphereBenchmarksCombined benchmarks;
};

#endif
