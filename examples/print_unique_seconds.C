#include "ROOT/RDataFrame.hxx"
#include "TSystem.h"
#include "TTree.h"
#include <unordered_set>

using ROOT::RDataFrame;
using ROOT::RDF::RResultPtr;

UInt_t rollover_difference_32bit(UInt_t big, UInt_t small){
  return big > small ? big-small : big + UINT32_MAX-small;
}

void print_unique_seconds(){

  gSystem->Load("libpueoEvent.so");

  // ROOT::DisableImplicitMT(); // default already disables this so no need
  RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/bfmr_r739_head.root");

  std::unordered_set<UInt_t> encounters;
  UInt_t event_second, last_pps, llast_pps, delta;

  // this tree only stores unique encounters
  TTree unique_trigger_time("", "");
  unique_trigger_time.Branch("event_second", &event_second);
  unique_trigger_time.Branch("last_pps", &last_pps);
  unique_trigger_time.Branch("llast_pps", &llast_pps);
  unique_trigger_time.Branch("delta", &delta);

  // traverse through the temporary (intermediate) header file to fill TTree above (for postprocessing)
  auto fill_tree_with = 
    [&encounters, &unique_trigger_time, &event_second, &last_pps, &llast_pps, &delta]
    (UInt_t evtsec, UInt_t lpps, UInt_t llpps)
    {
      bool not_found = encounters.find(evtsec) == encounters.end();
      if (not_found) {
        encounters.insert(evtsec);
        event_second = evtsec;
        last_pps = lpps;
        llast_pps = llpps;
        delta = rollover_difference_32bit(lpps, llpps);
        unique_trigger_time.Fill();
      }
    };
  tmp_header_rdf.Foreach(fill_tree_with, {"triggerTime","lastPPS","lastLastPPS"});

  RDataFrame unique_rdf(unique_trigger_time);
  unique_rdf.Display({"event_second", "last_pps", "llast_pps", "delta"}, 100)->Print();

  
  exit(0);
}
