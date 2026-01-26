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

  // TFile event_file("eventFile.root", "RECREATE");
  // TFile head_file ("headFile.root" , "RECREATE");
  // TFile gps_file  ("gpsEvent.root" , "RECREATE");

  // TTree * event_tree = new TTree("eventTree", "eventTree");
  // TTree * head_tree  = new TTree("headTree",  "headTree");
  // TTree * gps_tree   = new TTree("gpsTree",   "gpsTree");

  // pueo::RawEvent * raw_event;
  // pueo::RawHeader raw_header;
  // pueo::nav::Attitude attitude;

  // event_tree->Branch("event",  &raw_event);
  // head_tree ->Branch("header", &raw_header);
  // gps_tree  ->Branch("gps",    &attitude);

int main(){
  pueo_handle_t hndl;
  if (pueo_handle_init(&hndl, "../2025-12-31-R005.wfs", "r") < 0 ) {
    fprintf(stderr, "\e\[1;31mFile handle init failed (%s, line %d)\n",__PRETTY_FUNCTION__,__LINE__);
    exit(1);
  }

  std::unordered_map<UInt_t, TFile *> all_runs_found_thus_far; // dictionary
  TFile * eventFile = nullptr;
  TTree * eventTree = nullptr;
  pueo::RawEvent * event = nullptr;
  pueo_packet_t * pkt = nullptr; // TODO: do I need to free the packet?

  while(pueo_ll_read_realloc(&hndl, &pkt) > 0)
  {
    switch(pkt->head.type)
    {
      case PUEO_PACKET_INVALID:
        {
          break;
        }
      case PUEO_PACKET_HEAD:
        {
          break;
        }
      case PUEO_FULL_WAVEFORMS: 
        {
          const pueo_full_waveforms_t * fwf = (const pueo_full_waveforms_t *)(pkt->payload);
          UInt_t run = fwf->run;
          event = new pueo::RawEvent(fwf);

          auto search_result = all_runs_found_thus_far.find(run);
          if ( search_result != all_runs_found_thus_far.end() ) // ie. we've seen this run before
          {
            eventFile = search_result->second;
            printf("found run%d, retrieved %s\n", run, eventFile->GetName());
            eventTree = eventFile->Get<TTree>("eventTree");

          } else { 
            // ie. never encounter this run before, but the run might already exist on disk
            fs::exists(fs::path(Form("run%d", run))) ? : fs::create_directory(Form("run%d", run));

            // create a new TFile if file doesn't exist on disk already, update (retrieve) if it does
            eventFile = new TFile(Form("run%d/eventFile%d.root", run, run),"update");

            auto a_new_pair = std::make_pair(run, eventFile);
            all_runs_found_thus_far.insert(a_new_pair);

            // if the tree already exists, retrive it; else, create it
            if (eventFile->GetListOfKeys()->Contains("eventTree")) 
            {
              eventTree = eventFile->Get<TTree>("eventTree");
              eventTree->SetBranchAddress("event", event);
            } else {
              eventTree = new TTree("eventTree", "eventTree"); // create a new tree and
              eventTree->SetDirectory(eventFile);       // explicitly set its associated TFile, 
              eventTree->Branch("event", &event);      // lest ROOT does weird shit
            }
          }
          eventTree->Fill();
          delete event;
          event=nullptr;
          break;
        }
      default: 
        break;
    }
  }

  // kOverwrite: repalces old metadata (ie don't make new cycles)
  //   https://root-forum.cern.ch/t/adding-entries-to-a-ttree/9575/2
  //   https://root.cern.ch/doc/master/classTObject.html#aeac9082ad114b6702cb070a8a9f8d2ed
  for (auto & pair: all_runs_found_thus_far){
    pair.second->Write(nullptr, TObject::kOverwrite);
    printf("closing file %s", pair.second->GetName());
    pair.second->Close();
  }
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

// Testing the behavior as described in the pull request.
void dictionary_logic_test()
{
  int runs[3] {1,2,1};
  int r=-1;
  std::unordered_map<UInt_t, TFile *> all_runs; // dictionary

  TFile * current_file = nullptr;
  TTree * current_tree = nullptr;

  for (int i=0; i<std::size(runs); ++i) 
  {
    r = runs[i];

    if ( all_runs.find(r) != all_runs.end() ) // run number exists in dictionary
    {
      current_file = all_runs.find(r)->second;
      current_tree = current_file->Get<TTree>("runTree");
      printf("found run%d\n",runs[i]);
    } 
    else
    {
      fs::exists(fs::path(Form("run%d", r))) ? : fs::create_directory(Form("run%d", r));
      
      // create a new TFile if file doesn't exist on disk already, update (retrieve) if it does
      current_file = new TFile(Form("run%d/run%d.root", r, r),"update");

      auto me_pair = std::make_pair( r, current_file);
      all_runs.insert(me_pair);

      // if the tree already exists, retrive it; else, create it
      if (current_file->GetListOfKeys()->Contains("runTree")) 
      {
        current_tree = current_file->Get<TTree>("runTree");
        current_tree->SetBranchAddress("run", &r);
      } 
      else 
      {
        current_tree = new TTree("runTree", "runTree"); // create a new tree and
        current_tree->SetDirectory(current_file);       // explicitly set its associated TFile, 
        current_tree->Branch("run", &r);                // lest ROOT does weird shit
      }
    }
    current_tree->Fill();
  }

  for (auto& pair: all_runs)
  {
    printf("%s\n", pair.second->GetName());
    pair.second->Write();
    pair.second->Close();
  }
}
