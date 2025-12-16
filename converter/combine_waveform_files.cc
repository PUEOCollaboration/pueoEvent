
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
#include "pueo/RawHeader.h"
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
    std::vector<pueo::RawEvent*> output_events;
    std::vector<pueo::RawHeader*> output_headers;
    converter::prepare_output_headers(
      &output_headers,
      &events,
      runs[i_run]
    );
    converter::prepare_output_events(
      &output_events,
      &events,
      runs[i_run]
    );
    converter::fill_output_events(
      &output_events,
      &output_headers,
      &source_dir,
      &events,
      runs[i_run]
    );

    std::stringstream filename;
    filename << output_dir.str() << "/" << "rootifiedEventFile" << runs[i_run] << ".root";
    std::stringstream header_filename;
    header_filename << output_dir.str() << "/" << "rootifiedHeadFile" << runs[i_run] << ".root";
    TFile *headerFile = new TFile(TString(header_filename.str().c_str()), "RECREATE");
    TTree *headerTree = new TTree("headTree", "headTree");
    pueo::RawHeader dummy_header;
    headerTree->Branch("header", &dummy_header);
    for (unsigned int i=0; i<events.size(); i++) {
      headerTree->SetBranchAddress("header", &(output_headers[i]));
      headerTree->Fill();
      delete output_headers[i];
      output_headers[i] = NULL;
    }
    headerTree->Write();
    headerFile->Close();
    delete headerFile;

    TFile *outputFile = new TFile(TString(filename.str().c_str()), "RECREATE");
    TTree *outputTree = new TTree("eventTree", "eventTree");
    pueo::RawEvent dummy_event;
    outputTree->Branch("event", &dummy_event);
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
