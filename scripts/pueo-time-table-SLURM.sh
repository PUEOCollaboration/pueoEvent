#!/bin/bash

#SBATCH -A PAS2608
#SBATCH -t 2:30:00
#SBATCH --ntasks-per-node=1
#SBATCH --output=/fs/scratch/PAS2608/jason/global_timing_calibration/out/%a.out
#SBATCH --error=/fs/scratch/PAS2608/jason/global_timing_calibration/out/%a.err
#SBATCH --mail-type=FAIL
#SBATCH --mail-user=unmovingcastle

  
root -l -q "${PARENT_DIR}/examples/header_time_postprocessor_toy.C(${SLURM_ARRAY_TASK_ID}, \"${TIMEMARK_ROOTFILE_PATH}\", \"${TIME_TABLE_PATH}\")"
