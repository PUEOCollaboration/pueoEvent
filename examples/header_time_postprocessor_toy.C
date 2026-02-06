#include "ROOT/RDataFrame.hxx"
#include "TSystem.h"
#include "TTree.h"
#include <unordered_set>

using ROOT::RDataFrame;
using ROOT::RDF::RResultPtr;

// Some assumptions about the data are made:
// (a)  The column `event_second` is monotonically increasing, ie "sorted"
// (b)  0 <= event_second[x] - event_second[x-1] <= 1
void header_time_postprocessor_toy(){

  gSystem->Load("libpueoEvent.so");

  // ROOT::DisableImplicitMT(); // default already disables this so no need
  RDataFrame rdf("header", "/usr/pueoBuilder/install/bin/bfmr_r739_head.root");

  std::unordered_set<UInt_t> encounters;
  UInt_t event_second, last_pps, llast_pps;

  // this tree only stores unique encounters
  TTree unique_trigger_time("", "");
  unique_trigger_time.Branch("event_second", &event_second);
  unique_trigger_time.Branch("last_pps", &last_pps);
  unique_trigger_time.Branch("llast_pps", &llast_pps);

  auto fill_tree_with = 
    [&encounters, &unique_trigger_time, &event_second, &last_pps, &llast_pps]
    (UInt_t evtsec, UInt_t lpps, UInt_t llpps)
    {
      bool not_found = encounters.find(evtsec) == encounters.end();
      if (not_found) {
        encounters.insert(evtsec);
        event_second = evtsec;
        last_pps = lpps;
        llast_pps = llpps;
        unique_trigger_time.Fill();
      }
    };

  rdf.Foreach(fill_tree_with, {"triggerTime","lastPPS","lastLastPPS"});
  
  RDataFrame new_rdf(unique_trigger_time);
  auto is_diff_between = []
    (UInt_t lpps, UInt_t llpps)
    {
      return lpps > llpps ? lpps-llpps : lpps + UINT32_MAX-llpps; // ie deal with rollover
    };
  auto diff_rdf = new_rdf.Define("delta", is_diff_between, {"last_pps", "llast_pps"});

  diff_rdf.Display({"event_second", "last_pps", "llast_pps", "delta"}, 100)->Print();

  
  exit(0);
}
