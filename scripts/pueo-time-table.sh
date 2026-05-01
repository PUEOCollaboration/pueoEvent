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
long_run_list=()
short_run_list=()

for file in "${PUEO_ROOT_DATA}"/* ; do
  run=$(echo "$file" | sed -E 's/.*run([0-9]+)/\1/')
  mkdir -p ${TIME_TABLE_PATH}/run${run}

  # 1392 is 6000 seconds long and takes a while
  if [[ ${run} == "1392" ]]; then
    long_run_list+=($((run - 700)))
  else
    short_run_list+=($((run - 700)))  # very stupid, probably dangerous, but SLURM id upperlimit is 1000
  fi
done

# convert to comma separated
IFS=, short_run_list_csv="${short_run_list[*]}"
IFS=, long_run_list_csv="${long_run_list[*]}"
unset IFS

# store stdout/stderr log: 
# this line is here because we're not using SLURM directives --output and --error inside the job script.
rm -rf /fs/scratch/PAS2608/jason/global_timing_calibration/
mkdir /fs/scratch/PAS2608/jason/global_timing_calibration/

# use the run number as SLURM job array numbers; 
# `--export=ALL` is needed to forward current environment, else the job will fail 
# (will complain about CERN ROOT not found)
sbatch --export=ALL,PARENT_DIR=${PARENT_DIR},TIMEMARK_ROOTFILE_PATH=${TIMEMARK_ROOTFILE_PATH},TIME_TABLE_PATH=${TIME_TABLE_PATH} \
       --array=${long_run_list_csv} \
       --time=06:00:00 \
       ${PARENT_DIR}/scripts/pueo-time-table-SLURM.sh

sbatch --export=ALL,PARENT_DIR=${PARENT_DIR},TIMEMARK_ROOTFILE_PATH=${TIMEMARK_ROOTFILE_PATH},TIME_TABLE_PATH=${TIME_TABLE_PATH} \
       --array=${short_run_list_csv} \
       ${PARENT_DIR}/scripts/pueo-time-table-SLURM.sh
