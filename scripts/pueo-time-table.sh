#!/bin/bash

PARENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd) # absolute path of pueoEvent/
TIME_TABLE_PATH=${HOME}/time_tables/                         # output path, structure is time_tables/run<run_number>/time_table.root
TIMEMARK_ROOTFILE_PATH="/work/all_timemarks.root"            # path to all_timemarks.root


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
  runs+=($run)
done

# convert to comma separated
IFS=, run_num_csv="${runs[*]}"
unset IFS

# use the run number as SLURM job array numbers
sbatch --export=PARENT_DIR=${PARENT_DIR},TIMEMARK_ROOTFILE_PATH=${TIMEMARK_ROOTFILE_PATH},TIME_TABLE_PATH=${TIME_TABLE_PATH} \
       --array=${run_num_csv} \
       ${PARENT_DIR}/scripts/pueo-time-table-SLURM.sh
