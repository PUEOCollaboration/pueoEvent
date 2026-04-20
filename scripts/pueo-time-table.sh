#!/bin/bash

# @file pueo-time-table
# @brief Submits a SLURM job array on OSC (each array job is one call to pueo-time-table-SLURM.sh)
#        in order to create a "time table" for each run in PUEO_ROOT_DATA

PARENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd) # absolute path of pueoEvent/
TIME_TABLE_PATH=${HOME}/time_tables/                         # output path, structure is time_tables/run<run_number>/time_table.root
TIMEMARK_ROOTFILE_PATH="/fs/ess/PAS0654/PUEO/flight/timemark.root"            # path to all_timemarks.root


if [[ -z "$PUEO_ROOT_DATA" ]]; then
  echo -e "\033[1;31mPUEO_ROOT_DATA not defined\033[0;m"
  exit 1
fi
if [[ -z "$ROOT_INCLUDE_PATH" ]]; then
  echo -e "\033[1;31mROOT_INCLUDE_PATH not defined\033[0;m"
  exit 1
fi

# get a list of all runs in PUEO_ROOT_DATA, use them to create time_tables/run<#>/ directories
run_num_list=()
for file in "${PUEO_ROOT_DATA}"/* ; do
  run=$(echo "$file" | sed -E 's/.*run([0-9]+)/\1/')
  mkdir -p ${TIME_TABLE_PATH}/run${run}
  run_num_list+=($((run - 700)))  # very stupid, very dangerous, but SLURM id upperlimit is 1000
done

# convert to comma separated
IFS=, run_num_csv="${run_num_list[*]}"
unset IFS

# store stdout/stderr log: 
# this line is here because we're not using SLURM directives --output and --error inside the job script.
rm -rf /fs/scratch/PAS2608/jason/global_timing_calibration/
mkdir /fs/scratch/PAS2608/jason/global_timing_calibration/

# use the run number as SLURM job array numbers; 
# `--export=ALL` is needed to forward current environment, else the job will fail 
# (will complain about CERN ROOT not found)
sbatch --export=ALL,PARENT_DIR=${PARENT_DIR},TIMEMARK_ROOTFILE_PATH=${TIMEMARK_ROOTFILE_PATH},TIME_TABLE_PATH=${TIME_TABLE_PATH} \
       --array=${run_num_csv} \
       ${PARENT_DIR}/scripts/pueo-time-table-SLURM.sh
