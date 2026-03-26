// @file timemark_investigation.C
// This is not an example, but I need somewhere to store this script so here it is.
//
// Below is an example entry of the timemarkTree stored in a converted timemark ROOT file
//       timemark        = (pueo::Timemark*)0xaaaacd34e480
//       readout_time.utc_secs = 1764969939
//       readout_time.utc_nsecs = 507088232
//       rising.utc_secs = 1764969903
//       rising.utc_nsecs = 821490099
//       falling.utc_secs = 1764969903
//       falling.utc_nsecs = 821490107
//       rise_count      = 12912
//       channel         = 0
//       flags           = 205
// readout_time.utc_secs differs significantly from rising.utc_secs which might cause problems
// for global timing calibration, see pull request 10's comment on event_second correction
// (https://github.com/PUEOCollaboration/pueoEvent/pull/10#issuecomment-4136908462)
//
// This script will try to investigate this difference.

#include "TSystem.h"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"

void timemark_investigation()
{
  gSystem->Load("libpueoEvent.so");
  ROOT::RDataFrame rdf("timemarkTree", "/work/all_timemarks.root");
  
  auto diff_bw = 
    [](ULong64_t readout_sec, ULong64_t turf_sec)
    {
      return static_cast<Long64_t>(readout_sec) - static_cast<Long64_t>(turf_sec);
    };
  auto diff_df = rdf.Define("difference", diff_bw, {"readout_time.utc_secs", "rising.utc_secs"});

  auto gr = diff_df.Graph<ULong64_t, Long64_t>("rising.utc_secs", "difference");
  TCanvas c1("","", 1920, 1080);  
  gr->SetTitle("readout_time.utc_secs - rising.utc_secs");
  gr->GetYaxis()->CenterTitle();
  gr->GetYaxis()->SetTitle("Difference [sec]");
  gr->GetXaxis()->SetTitle("Date");
  gr->GetXaxis()->SetTimeDisplay(1);
  gr->GetXaxis()->SetTimeFormat("%m/%d/%Y%F1970-01-01 00:00:00");
  gr->Draw("ALP");
  c1.SaveAs("timemark_investigation.png");
}
