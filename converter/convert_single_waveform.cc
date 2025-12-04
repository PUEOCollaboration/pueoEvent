
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

  /* Read waveform */
  pueo_handle_t pueo_handle;
  pueo_single_waveform_t wf = {};
  pueo_handle_init(&pueo_handle, args[1], "r");
  int read = pueo_read_single_waveform(&pueo_handle, &wf);
  
    /* Set up output ROOT tree*/
  TString outputfile_name = args[2];
  TFile *outputFile = new TFile(outputfile_name, "RECREATE");
  TTree *outputTree = new TTree("eventTree", "eventTree");
  pueo::RawEvent pueo_event;
  outputTree->Branch("event", &pueo_event);


  /* Write waveform into ROOT tree */
  for (unsigned int i=0; i<208; i++) {
    pueo_event.data[i].resize(1024);
      for (unsigned int j=0; j<1; j++) {
        pueo_event.data[i][j] = 0;
    }
  }
  if (wf.wf.length > 0) {
    pueo_event.data[wf.wf.channel_id].resize(wf.wf.length);
     for (unsigned int i_sample=0; i_sample<wf.wf.length; i_sample++) {
      pueo_event.data[0][i_sample] = wf.wf.data[i_sample];
     }
  }
  pueo_event.runNumber = wf.run;
  pueo_event.eventNumber = wf.event;
  outputTree->Fill();
  outputFile->Write();
  outputFile->Close();
  pueo_handle_close(&pueo_handle);
  return 0;
}
