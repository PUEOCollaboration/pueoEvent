
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

struct id_pair {
    int run;
    int event;
  };

bool has_event_ids(const std::vector<id_pair> vec, int run, int event) {
    for (unsigned int i=0; i<vec.size(); i++) {
      if ((vec[i].run == run)&&(vec[i].event == event)) {
        return true;
      }
    }
    return false;
  }


int main(int nargs, char** args) {
  if (nargs != 3) {
    std::cout << "Converts raw PUEO data files into ROOT format. \n Required arguments:\n input directory\n output directory\n";
    return 1;
  }
  const int surf_mapping[26] = {5, 4, 3, 2, 1, 0, 7, 8, 9, 10, 11, 12, 26, 25, 24, 23, 22, 21, 14, 15, 16, 17, 18, 19, 13, 6}; 
  const int channel_mappings[8] = {4, 5, 6, 7, 0, 1, 2, 3};
  std::vector<id_pair> event_ids;
  std::vector<int> runs;
  const std::filesystem::path source_dir{args[1]};
  pueo_handle_t pueo_handle;
  pueo_single_waveform_t wf = {};
  for (auto const &direntry : std::filesystem::directory_iterator{source_dir}) {
    pueo_handle_init(&pueo_handle, direntry.path().string().c_str(), "r");
    int read = pueo_read_single_waveform(&pueo_handle, &wf);
    if (!has_event_ids(event_ids, wf.run, wf.event)) {
      event_ids.push_back(id_pair());
      event_ids[event_ids.size()-1].run = wf.run;
      event_ids[event_ids.size()-1].event = wf.event;
      if (count(runs.begin(), runs.end(), wf.run) == 0) {
        runs.push_back(wf.run);
      }
    }
    pueo_handle_close(&pueo_handle);
  }


  // Loop over runs
  for (unsigned int i_run=0; i_run<runs.size(); i_run++) {
    std::vector<int> events;
    for (unsigned int i_id=0; i_id<event_ids.size(); i_id++) {
      if ((event_ids[i_id].run == runs[i_run]) && (count(events.begin(), events.end(), event_ids[i_id].event)==0)) {
        events.push_back(event_ids[i_id].event);
      }
    }
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

    std::sort(events.begin(), events.end());
    std::vector<pueo::RawEvent*> output_events;
    for (unsigned int i=0; i<events.size(); i++) {
      output_events.push_back(new pueo::RawEvent());
      output_events[i]->runNumber = runs[i_run];
      output_events[i]->eventNumber = events[i];
      for (unsigned int i_ch=0; i_ch<208; i_ch++) {
        output_events[i]->data[i_ch].resize(1024);
        for (unsigned int j=0; j<1024; j++) {
          output_events[i]->data[i_ch][j] = 0;
        }
      }
    }
    for (auto const &direntry : std::filesystem::directory_iterator{source_dir}) {
      pueo_handle_init(&pueo_handle, direntry.path().string().c_str(), "r");
      int read = pueo_read_single_waveform(&pueo_handle, &wf);
      if ((wf.run == runs[i_run]) && (wf.wf.length > 0)) {
        // pueo_dump_single_waveform(stdout, &wf);
        if (wf.wf.channel_id >= 208) {
          pueo_handle_close(&pueo_handle);
          continue;
        }          
        int output_event_index = std::find(events.begin(), events.end(), wf.event) - events.begin();
        int i_surf = surf_mapping[wf.wf.channel_id / 8];
        int i_surf_channel = channel_mappings[wf.wf.channel_id % 8];
        int i_channel = i_surf * 8 + i_surf_channel;
        for (unsigned int i_sample=0; i_sample<std::min(wf.wf.length,(u_int16_t) 1024); i_sample++) {
          output_events[output_event_index]->data[i_channel][i_sample] = wf.wf.data[i_sample];
          
        }
      }
      pueo_handle_close(&pueo_handle);
    }

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
