
extern "C" {
  #include "pueo/rawio.h"
  #include "pueo/rawdata.h"
}


#include <iostream>
#include <stdbool.h>
#include "TString.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "pueo/RawEvent.h"


int main(int nargs, char** args) {
  if (nargs != 3) {
    std::cout << "Converts raw PUEO data files into ROOT format. \n Required arguments:\n input filename\n output filename\n";
    return 1;
  }

  const int surf_mapping[26] = {5, 4, 3, 2, 1, 0, 7, 8, 9, 10, 11, 12, 26, 25, 24, 23, 22, 21, 14, 15, 16, 17, 18, 19, 13, 6}; 
  const int channel_mappings[8] = {4, 5, 6, 7, 0, 1, 2, 3};

  /* Read waveform */
  pueo_handle_t pueo_handle;
  pueo_single_waveform_t wf = {};
  pueo_handle_init(&pueo_handle, args[1], "r");
  int read = pueo_read_single_waveform(&pueo_handle, &wf);
  pueo_dump_single_waveform(stdout,&wf);

    /* Set up output ROOT tree*/
  TString outputfile_name = args[2];
  TFile *outputFile = new TFile(outputfile_name, "RECREATE");
  TTree *outputTree = new TTree("eventTree", "eventTree");
  pueo::RawEvent pueo_event;
  outputTree->Branch("event", &pueo_event);


  //pueo_dump_single_waveform(stdout, &wf);
  int i_surf = surf_mapping[wf.wf.channel_id / 8];
  int i_surf_channel = channel_mappings[wf.wf.channel_id % 8];
  int i_channel = i_surf * 8 + i_surf_channel;
  /* Write waveform into ROOT tree */
  for (unsigned int i=0; i<208; i++) {
    pueo_event.data[i].resize(1024);
      for (unsigned int j=0; j<1; j++) {
        pueo_event.data[i][j] = 0;
    }
  }
  std::cout << i_channel << std::endl;
  if (wf.wf.length > 0) {
    pueo_event.data[wf.wf.channel_id].resize(wf.wf.length);
     for (unsigned int i_sample=0; i_sample<wf.wf.length; i_sample++) {
      pueo_event.data[i_channel][i_sample] = wf.wf.data[i_sample];
     }
  }
  pueo_event.runNumber = wf.run;
  pueo_event.eventNumber = wf.event;
  outputTree->Fill();
  outputFile->Write();
  outputFile->Close();
  return 0;
}
