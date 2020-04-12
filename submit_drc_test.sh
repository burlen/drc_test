#!/bin/bash -l
#SBATCH -N 498
#SBATCH -C knl
#SBATCH -t 00:30:00
#SBATCH -q premium
#SBATCH -A m636
#SBATCH -J in_transit_iso_knl
#SBATCH -d singleton
#SBATCH -o "drc_test-%j.out"
module swap PrgEnv-intel PrgEnv-gnu

M=8192
N=256

CPN=17
TPC=`echo "68/${CPN}" | bc`
RM=`echo "${M}/${CPN} + 1" | bc`

set -x

export DRC_LOG_LEVEL=DEBUG
export OMP_NUM_THREADS=${TPC}
srun -N ${RM} -n ${M} -r 0 ./drc_test.exe

