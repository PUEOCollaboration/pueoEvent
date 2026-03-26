// @file timemark_investigation.C
// This is not an example, but I need somewhere to store this script so here it is.
//

#include "TSystem.h"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"

void timemark_investigation()
{
  gSystem->Load("libpueoEvent.so");
  ROOT::RDataFrame rdf("timemarkTree", "/work/all_timemarks.root");

  rdf.Filter("rising.utc_secs >= 1767235954-5 && rising.utc_secs <= 1767235954+5")
     .Display({"readout_time.utc_secs","rising.utc_secs", "rising.utc_nsecs"}, 100)->Print();
  
}
