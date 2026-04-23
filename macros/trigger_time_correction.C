// @file  trigger_time_correction.C
// @brief Uses time tables created by make_time_table.C to correct the trigger time in headFile<run>.root
//        for every single valid run.
//        valid run: (1) at least one GPS-timestamped event is found
//                   (2) at least three seconds in length
//                   (3) not a preamp run (ie run number has to be >= 783

#include "pueo/RawHeader.h"
#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"
#include <regex>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
constexpr int32_t NOMINAL_CLOCK_FREQ=125000000;

struct relevant_shit{
  int32_t  corrected_event_second = 0;
  double   avg_relative_delta = 0;
  double   corrected_pps = 0;
};
using TimeTable=std::map<int32_t, relevant_shit>;

// correction only applied if time table of the run is 1. found and 2. valid (ie has corrected_event_second)
// else this function just copies
// TODO: double check trigger time subsection correction
int32_t correct_raw_header(const char * input_parent, const char * output_parent, uint32_t run, const char * time_table_path = nullptr);

// sanity check to see if things are copied over correctly.
int32_t simple_equality_check(uint32_t run, const char * orig, const char * post);

void trigger_time_correction(){

  gSystem->Load("libpueoEvent.so");
  const char * pueo_root_data = std::getenv("PUEO_ROOT_DATA");
  if (!pueo_root_data) {
    std::cerr << "\e[1;31mPUEO_ROOT_DATA not defined.\n";
    exit(1);
  }
  const char * output_run_parent = "./headers";
  // correct_raw_header(pueo_root_data, "./headers", 1392, "/work/time_tables/");

  fs::directory_iterator time_tables_dir_iter( "/work/time_tables");
  const std::regex run_pattern(R"(run(\d+))");
  std::smatch run_number_match;

  for(auto const& dir: time_tables_dir_iter)
  {
    std::string dirname = dir.path().filename().string();
    if (!dir.is_directory())
      continue;

    if (!std::regex_match(dirname, run_number_match, run_pattern))
      continue;

    uint32_t run = std::atoi(run_number_match[1].str().c_str());

    fs::path time_table_path = (dir.path()/ "time_table.root");

    // case: not an empty run directory (ie no time table created for this run)
    if (fs::exists(time_table_path)) {
      std::cout << "Found " << time_table_path << " for " << run << "\n";
      correct_raw_header(pueo_root_data, output_run_parent, run, time_table_path.c_str());
    } else {
      std::cout << "\e[1;31mNo time table for run " << run << "\e[0m\n";
      correct_raw_header(pueo_root_data, output_run_parent, run);
    }
  }

  // sanity check
  fs::recursive_directory_iterator run_dir(pueo_root_data);
  const std::regex pattern(R"(headFile(\d+))");
  for(auto const& entry: run_dir)
  {
    if (!entry.is_regular_file()) continue;

    std::string name = entry.path().stem().string(); 
    std::smatch match;
    // skip other root files in the run folder
    if (!std::regex_match(name, match, pattern)) continue;

    uint32_t r = std::atoi(match[1].str().c_str());
    std::cout << "checking equality of run " << r << "...";
    uint32_t equality_err= simple_equality_check(
      r, entry.path().string().c_str(), Form("%s/run%d/headFile%d.root",output_run_parent, r, r)
    );
    if (!equality_err) std::cout << "passed\n";
    else std::cerr << "failed!\n";

  }
}


int32_t simple_equality_check(uint32_t run, const char * orig, const char * post){

  int32_t global_error = 0;
  pueo::RawHeader * original = nullptr;
  TFile f_orig(orig);
  TTree * t_orig = f_orig.Get<TTree>("headerTree");
  t_orig->SetBranchAddress("header", &original);

  pueo::RawHeader * corrected = nullptr;
  TFile f_corr(post);
  TTree * t_corr = f_corr.Get<TTree>("headerTree");
  t_corr->SetBranchAddress("header", &corrected);

  if (t_corr->GetEntries() != t_orig->GetEntries()) {
    std::cout << "run " << run << ":unequal length\n";
    global_error = 1;
  }

  int32_t local_error = 0;
  for (int i=0; i<t_corr->GetEntries(); ++i){
    t_corr->GetEntry(i);
    t_orig->GetEntry(i);

    if(original->run!=corrected->run) local_error = 1;
    if(original->eventNumber!=corrected->eventNumber) local_error = 1;
    if(original->trigTime!=corrected->trigTime) local_error = 1;
    if(original->triggerTime!=corrected->triggerTime) local_error = 1;
    if(original->lastPPS!=corrected->lastPPS) local_error = 1;
    if(original->lastLastPPS!=corrected->lastLastPPS) local_error = 1;
    if(original->deadTime!=corrected->deadTime) local_error = 1;
    if(original->deadTimeLastPPS!=corrected->deadTimeLastPPS) local_error = 1;
    if(original->deadTimeLastLastPPS!=corrected->deadTimeLastLastPPS) local_error = 1;
    if(original->L2Mask!=corrected->L2Mask) local_error = 1;
    if(original->trigType!=corrected->trigType) local_error = 1;
    if(original->readoutTime!=corrected->readoutTime) local_error = 1;
    if(original->readoutTimeNs!=corrected->readoutTimeNs) local_error = 1;

    if (local_error) {
      std::cerr << "run " << run << " event " << original->eventNumber << " is messed up\n";
      global_error = 1;
      local_error = 0;
    }
  }

  return global_error;
}

int32_t correct_raw_header(const char * input_parent, const char * output_parent, uint32_t run, const char * time_table_path){

  TimeTable dictionary;
  if (time_table_path) {
    ROOT::RDataFrame time_table_rdf("time_table_tree", time_table_path);

    // turn time table back into a dictionary
    auto fill_table = [&dictionary]
      (int32_t orig_sec, int32_t corr_sec, double avg_rel_delta, double corr_pps){
        bool new_encounter = dictionary.find(orig_sec) == dictionary.end();
        if (new_encounter){
          dictionary.emplace(
            orig_sec, 
            relevant_shit{.corrected_event_second=corr_sec, .avg_relative_delta=avg_rel_delta,.corrected_pps=corr_pps}
          );
        }
      };

    time_table_rdf.Foreach(fill_table, {"event_second","corrected_event_second","avg_relative_delta","corrected_pps"});
  }

  pueo::RawHeader * out_header = new pueo::RawHeader();

  fs::path destination_run_dir(Form("%s/run%d/", output_parent, run));
  if(! fs::exists(destination_run_dir)){
    fs::create_directories(destination_run_dir);
  }
  
  TFile output_file(Form("%s/run%d/headFile%d.root", output_parent, run, run), "RECREATE");
  TTree * output_tree = new TTree("headerTree", "headerTree");
  output_tree->Branch("header", &out_header);

  pueo::RawHeader * in_header = nullptr;
  TFile input_file(Form("%s/run%d/headFile%d.root", input_parent, run, run));
  TTree * input_tree = input_file.Get<TTree>("headerTree");
  input_tree->SetBranchAddress("header", &in_header);

  if (!time_table_path || dictionary.begin()->second.corrected_event_second == 0){
    std::cout << "\e[1;31mNo correction for run " << run << " because time table does not exist or is invalid \e[0m\n";
    // no correction if there is no valid time table
    for (int i=0; i<input_tree->GetEntries(); ++i){
      input_tree->GetEntry(i);
      // make a copy, just to be safe; cuz I don't know if I can read and write using the same pointer :)
      *out_header = *in_header;
      output_tree->Fill();
    }
  } else { // if there is a valid time table, apply correction
    std::cout << "Correcting trigger time for run " << run << "...\n";
    TimeTable::const_iterator this_row;
    for (int i=0; i<input_tree->GetEntries(); ++i){
      input_tree->GetEntry(i);
      this_row = dictionary.find(in_header->triggerTime);

      *out_header = *in_header;

      out_header->clock_frequency = this_row->second.avg_relative_delta + NOMINAL_CLOCK_FREQ;
      out_header->corrected_pps = this_row->second.corrected_pps;

      // subsecond [clock counts]
      double subsecond = std::fmod(
        static_cast<double>(out_header->trigTime) - out_header->corrected_pps + static_cast<double>(UINT32_MAX) + 1.,
        static_cast<double>(UINT32_MAX) + 1
      );

      subsecond = subsecond * 1e9 / out_header->clock_frequency; // subsecond [nanosec]

      out_header->corrected_trigger_time = TTimeStamp(
        (time_t) this_row->second.corrected_event_second, subsecond
      );
      output_tree->Fill();
    }
  }

  output_file.Write(); 
  output_file.Close();
  input_file.Close();
  delete out_header;
  out_header=nullptr;
  return 0;
}
