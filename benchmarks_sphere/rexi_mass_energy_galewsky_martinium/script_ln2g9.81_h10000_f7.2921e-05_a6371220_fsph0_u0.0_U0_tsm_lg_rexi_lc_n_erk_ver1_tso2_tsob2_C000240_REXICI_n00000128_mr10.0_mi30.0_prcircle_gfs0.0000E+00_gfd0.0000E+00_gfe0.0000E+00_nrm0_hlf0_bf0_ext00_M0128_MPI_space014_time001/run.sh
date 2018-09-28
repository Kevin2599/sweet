#! /bin/bash

#
# Platform: coolmuc_mpp2
# Generating function: jobscript_get_header
# Job id: g9.81_h10000_f7.2921e-05_a6371220_fsph0_u0.0_U0_tsm_lg_rexi_lc_n_erk_ver1_tso2_tsob2_C000240_REXICI_n00000128_mr10.0_mi30.0_prcircle_gfs0.0000E+00_gfd0.0000E+00_gfe0.0000E+00_nrm0_hlf0_bf0_ext00_M0128
#




# Provide platform ID helper for scripts.
# This makes platform detection on the compute nodes easier since they might have mainly numerical identifiers
export SWEET_PLATFORM="coolmuc_mpp2"

# Loading SWEET environment variables
cd "/home/martin/workspace/sweet"
source ./local_software/env_vars.sh "/home/martin/workspace/sweet/platforms/50_coolmuc_mpp2/env_vars.sh" || exit 1



# chdir to execution directory
cd "/home/martin/workspace/sweet/benchmarks_sphere/rexi_mass_energy_galewsky_martinium/script_ln2g9.81_h10000_f7.2921e-05_a6371220_fsph0_u0.0_U0_tsm_lg_rexi_lc_n_erk_ver1_tso2_tsob2_C000240_REXICI_n00000128_mr10.0_mi30.0_prcircle_gfs0.0000E+00_gfd0.0000E+00_gfe0.0000E+00_nrm0_hlf0_bf0_ext00_M0128_MPI_space014_time001"



#
# Platform: coolmuc_mpp2
# Generating function: jobscript_get_exec_prefix
# Job id: g9.81_h10000_f7.2921e-05_a6371220_fsph0_u0.0_U0_tsm_lg_rexi_lc_n_erk_ver1_tso2_tsob2_C000240_REXICI_n00000128_mr10.0_mi30.0_prcircle_gfs0.0000E+00_gfd0.0000E+00_gfe0.0000E+00_nrm0_hlf0_bf0_ext00_M0128
#




#
# Platform: coolmuc_mpp2
# Generating function: jobscript_get_exec_command
# Job id: g9.81_h10000_f7.2921e-05_a6371220_fsph0_u0.0_U0_tsm_lg_rexi_lc_n_erk_ver1_tso2_tsob2_C000240_REXICI_n00000128_mr10.0_mi30.0_prcircle_gfs0.0000E+00_gfd0.0000E+00_gfe0.0000E+00_nrm0_hlf0_bf0_ext00_M0128
#


# mpiexec ... would be here without a line break
EXEC="$SWEETROOT/build/swe_sphere_spspec_spdeal_rxthpar_numa2_fft_gnu_release  -g 9.81 -H 10000 -f 7.2921e-05 -F 0 -a 6371220 -M 128 --pde-id 0 --staggering=0 -S 1 -X 1 -Y 1 -s 100 --benchmark= -v 2 --dt=240 -o 60 -O - -u 0.0 -t 432000 --stability-checks=1 -d 14 --use-linear-div=0 --use-local-visc=0 --timestepping-method=lg_rexi_lc_n_erk_ver1 --timestepping-order=2 --timestepping-order2=2 --normal-mode-analysis-generation=0 --rexi-method=ci --rexi-half=0 --rexi-normalization=0 --rexi-sphere-preallocation=1 --rexi-use-direct-solution=0 --rexi-ext-modes=0 --rexi-ci-n=128 --rexi-ci-max-real=10.0 --rexi-ci-max-imag=30.0 --rexi-ci-primitive=circle --rexi-ci-gaussian-filter-scale=0.0 --rexi-ci-gaussian-filter-dt-norm=0.0 --rexi-ci-gaussian-filter-exp-N=0.0 --polvani-rossby=-1.0 --polvani-froude=-1.0 --use-robert-functions=1 --compute-error=0 --shtns-use-plans=0"
echo "$EXEC"
$EXEC



#
# Platform: coolmuc_mpp2
# Generating function: jobscript_get_exec_suffix
# Job id: g9.81_h10000_f7.2921e-05_a6371220_fsph0_u0.0_U0_tsm_lg_rexi_lc_n_erk_ver1_tso2_tsob2_C000240_REXICI_n00000128_mr10.0_mi30.0_prcircle_gfs0.0000E+00_gfd0.0000E+00_gfe0.0000E+00_nrm0_hlf0_bf0_ext00_M0128
#




#
# Platform: coolmuc_mpp2
# Generating function: jobscript_get_footer
# Job id: g9.81_h10000_f7.2921e-05_a6371220_fsph0_u0.0_U0_tsm_lg_rexi_lc_n_erk_ver1_tso2_tsob2_C000240_REXICI_n00000128_mr10.0_mi30.0_prcircle_gfs0.0000E+00_gfd0.0000E+00_gfe0.0000E+00_nrm0_hlf0_bf0_ext00_M0128
#

