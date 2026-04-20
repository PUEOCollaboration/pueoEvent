#!/bin/bash

#SBATCH -A PAS2608
#SBATCH -t 2:30:00
#SBATCH --ntasks-per-node=1

#SBATCH --output=/dev/null
#SBATCH --error=/dev/null
#normally one could use %a to store the output of task number `a`, but we are remapping task numbers.

#SBATCH --mail-type=FAIL
#SBATCH --mail-user=unmovingcastle
  
# A remap b/w run number and task id is needed because SLURM task id upper limit is 1000...
run_number=$(( SLURM_ARRAY_TASK_ID + 700 ))

# Have the standard output and standard error reflect the actual run number instead of task id
exec > /fs/scratch/PAS2608/jason/global_timing_calibration/${run_number}.out
exec 2>/fs/scratch/PAS2608/jason/global_timing_calibration/${run_number}.err

root -l -q "${PARENT_DIR}/examples/header_time_postprocessor_toy.C(${run_number}, \"${TIMEMARK_ROOTFILE_PATH}\", \"${TIME_TABLE_PATH}\")"
