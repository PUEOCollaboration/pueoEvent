#include "TTree.h"
#include "TFile.h"
#include "pueo/RawEvent.h"
#include "pueo/RawHeader.h"
#include "pueo/Nav.h"
#include "pueo/rawio.h"
#include <stdio.h>
#include <filesystem>
#include "ROOT/RDataFrame.hxx"

#define  PUEO_PACKET_INVALID   0
#define  PUEO_PACKET_HEAD      0x0ead
#define  PUEO_FULL_WAVEFORMS   0xda7a
#define  PUEO_SINGLE_WAVEFORM  0xea7a
#define  PUEO_ENCODED_WAVEFORM 0xabcd
#define  PUEO_NAV_ATT          0xa771
#define  PUEO_NAV_SAT          0x5a75
#define  PUEO_NAV_POS          0x1a75
#define  PUEO_DAQ_HSK          0xdacc
#define  PUEO_DAQ_HSK_SUMMARY  0xdac5
#define  PUEO_SS               0x501a
#define  PUEO_SENSORS_TELEM    0xb1de
#define  PUEO_SENSORS_DISK     0xcb1d
#define  PUEO_CMD_ECHO         0xecc0
#define  PUEO_LOGS             0x7a11
#define  PUEO_FILE_DOWNLOAD    0xd1d1
#define  PUEO_SLOW             0x510e
#define  PUEO_TIMEMARK         0xc10c
#define  PUEO_STARTRACKER      0xe71c
#define  PUEO_PRIO_STATUS      0x5cad
#define  PUEO_SAVED_PRIORITIES 0x7135  

namespace fs =std::filesystem;

int main(){
  pueo_handle_t hndl;
  if (pueo_handle_init(&hndl, "../2025-12-31-R005.wfs", "r") < 0 ) {
    fprintf(stderr, "\e\[1;31mFile handle init failed (%s, line %d)\n",__PRETTY_FUNCTION__,__LINE__);
    exit(1);
  }

  std::unordered_set<UInt_t> all_runs;

  TFile * current_file = nullptr;
  // TFile event_file("eventFile.root", "RECREATE");
  // TFile head_file ("headFile.root" , "RECREATE");
  // TFile gps_file  ("gpsEvent.root" , "RECREATE");

  TTree * current_tree = nullptr;
  // TTree * event_tree = new TTree("eventTree", "eventTree");
  // TTree * head_tree  = new TTree("headTree",  "headTree");
  // TTree * gps_tree   = new TTree("gpsTree",   "gpsTree");

  pueo::RawEvent * raw_event;
  // pueo::RawHeader raw_header;
  // pueo::nav::Attitude attitude;

  // event_tree->Branch("event",  &raw_event);
  // head_tree ->Branch("header", &raw_header);
  // gps_tree  ->Branch("gps",    &attitude);

  pueo_packet_t * pkt = nullptr;
  while(pueo_ll_read_realloc(&hndl, &pkt) > 0 ) {

    switch(pkt->head.type){
      case PUEO_PACKET_INVALID:{
        break;
      }
      case PUEO_PACKET_HEAD:{
        break;
      }
      case PUEO_FULL_WAVEFORMS: {
        const pueo_full_waveforms_t * fwf = (const pueo_full_waveforms_t *)(pkt->payload);
        UInt_t run = fwf->run;

        if (all_runs.insert(run).second) { // successful insert (ie a new run is found)
          
          if (current_file != nullptr){ // close currently opened file
            puts("closing file");
            current_file->Close();
          }
            
          // check dir existence, since the directory might already exists via other means
          if (!fs::exists(fs::path(Form("run%d", run)))) fs::create_directory(Form("run%d", run));

          // create a new TFile if file doesn't exist on disk already, update if it does
          current_file = TFile::Open(Form("run%d/eventFile%d.root", run, run), "update");
          
          // if the tree already exists, retrive it; else, create it
          if (current_file->GetListOfKeys()->Contains("eventTree")) {
            current_tree = current_file->Get<TTree>("eventTree");
            current_tree->SetBranchAddress("event", &raw_event);
          } else {
            current_tree = new TTree("eventTree", "eventTree");
            current_tree -> Branch("event", &raw_event); // branch is retrieved if it exists
          }

        } 
        raw_event = new pueo::RawEvent(fwf);
        // raw_header = pueo::RawHeader(fwf);
        // attitude = pueo::nav::Attitude(fwf);
        break;
      }
      default: 
        break;
    }

    current_tree->Fill();
    // event_tree->Fill();
    // head_tree->Fill();
    // gps_tree->Fill();

    delete raw_event;
    raw_event=nullptr;
  }

  if (current_file != nullptr){ // close currently opened file
    puts("closing file");
    // kOverwrite: repalces old metadata (ie don't make new cycles)
    //   https://root-forum.cern.ch/t/adding-entries-to-a-ttree/9575/2
    //   https://root.cern.ch/doc/master/classTObject.html#aeac9082ad114b6702cb070a8a9f8d2ed
    current_file->Write(nullptr, TObject::kOverwrite);
    current_file->Close();
  }
  // event_file.Write();
  // head_file.Write();
  // gps_file.Close();
}




















      // case PUEO_SINGLE_WAVEFORM:{
      //   break;
      // }
      // case PUEO_ENCODED_WAVEFORM:{
      //   break;
      // }
      // case PUEO_NAV_ATT:{
      //   break;
      // }
      // case PUEO_NAV_SAT:{
      //   break;
      // }
      // case PUEO_NAV_POS:{
      //   break;
      // }
      // case PUEO_DAQ_HSK:{
      //   break;
      // }
      // case PUEO_DAQ_HSK_SUMMARY:{
      //   break;
      // }
      // case PUEO_SS:{
      //   break;
      // }
      // case PUEO_SENSORS_TELEM:{
      //   break;
      // }
      // case PUEO_SENSORS_DISK:{
      //   break;
      // }
      // case PUEO_CMD_ECHO:{
      //   break;
      // }
      // case PUEO_LOGS:{
      //   break;
      // }
      // case PUEO_FILE_DOWNLOAD:{
      //   break;
      // }
      // case PUEO_SLOW:{
      //   break;
      // }
      // case PUEO_TIMEMARK:{
      //   break;
      // }
      // case PUEO_STARTRACKER:{
      //   break;
      // }
      // case PUEO_PRIO_STATUS:{
      //   break;
      // }
      // case PUEO_SAVED_PRIORITIES:{
      //   break;
      // }
