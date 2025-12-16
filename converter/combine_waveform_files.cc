
extern "C" {
  #include "pueo/rawio.h"
  #include "pueo/rawdata.h"
}

#include <algorithm>
#include <iostream>
#include <stdbool.h>
#include "TString.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "pueo/RawEvent.h"
#include <filesystem>
#include <vector>
#include "converter_functions.h"

int main(int nargs, char** args) {
  if (nargs != 3) {
    std::cout << "Converts raw PUEO data files into ROOT format. \n Required arguments:\n input directory\n output directory\n";
    return 1;
  }
  pueo_handle_t pueo_handle;
  pueo_single_waveform_t wf = {};

  const std::filesystem::path source_dir{args[1]};
  std::vector<converter::id_pair> event_ids;
  std::vector<int> runs;

  converter::list_ids(
    &source_dir,
    &event_ids,
    &runs
  );
  
  // Loop over runs
  for (unsigned int i_run=0; i_run<runs.size(); i_run++) {
    std::vector<int> events = converter::get_events_in_run(
      &event_ids,
      runs[i_run]
    );
    
    std::stringstream output_dir;
    output_dir << args[2] << "/run" << runs[i_run];
    if (!std::filesystem::exists(output_dir.str())) {
      std::filesystem::create_directory(output_dir.str());
    }
    std::stringstream filename;
    filename << output_dir.str() << "/" << "convertedEventFile" << runs[i_run] << ".root";
    TString outputfile_name(filename.str().c_str());
    TFile *outputFile = new TFile(outputfile_name, "RECREATE");
    TTree *outputTree = new TTree("eventTree", "eventTree");

    pueo::RawEvent dummy_event;
    outputTree->Branch("event", &dummy_event);
    std::vector<pueo::RawEvent*> output_events;
    converter::prepare_output_events(
      &output_events,
      &events,
      runs[i_run]
    );
    converter::fill_output_events(
      &output_events,
      &source_dir,
      &events,
      runs[i_run]
    );
    for (unsigned int i=0; i<events.size(); i++) {
      outputTree->SetBranchAddress("event", &(output_events[i]));
      outputTree->Fill();
      delete output_events[i];
      output_events[i] = NULL;
    }
    outputTree->Write();
    outputFile->Close();
    delete outputFile;
   
  }
  return 0;
}
