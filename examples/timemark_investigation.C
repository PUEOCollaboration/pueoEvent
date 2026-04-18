// @file timemark_investigation.C
// This is not an example, but I need somewhere to store this script so here it is.
//

#include "TTimeStamp.h"
#include "TSystem.h"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"

void timemark_investigation()
{
  gSystem->Load("libpueoEvent.so");
  ROOT::RDataFrame rdf("timemarkTree", "/work/all_timemarks.root");

  rdf.Filter("readout_time.fSec - 1766189366 > 0 && readout_time.fSec - 1766189366 < 100000")
     .Display({"readout_time.fSec","rising.fSec", "rising.fNanoSec"}, 100)->Print();

  auto diff_bw = [](TTimeStamp& readout_sec, TTimeStamp& rising)
    {return readout_sec.GetSec() - rising.GetSec();};
  auto diff_df = rdf.Define("difference", diff_bw, {"readout_time", "rising"})
                    .Define("x_axis", [](TTimeStamp& rising){return rising.GetSec();}, {"rising"});

  auto gr = diff_df.Graph<time_t, time_t>("x_axis", "difference");
  TCanvas c1("","", 1920, 1080);  
  gr->SetTitle("timemark_t.readout_time.utc_secs - timemark_t.rising.utc_secs");
  gr->GetYaxis()->CenterTitle();
  gr->GetYaxis()->SetTitle("Difference [sec]");
  gr->GetXaxis()->SetTitle("Date");
  // gr->GetXaxis()->SetRangeUser(1766.5e6, 1767e6); // seems like there's a clock drift during Christmas
  gr->GetXaxis()->SetRangeUser(1766189366-2000, 1766189366+2000); // seems like some days we were'nt timestamping
  // gr->GetXaxis()->SetRangeUser(1766217891-2000, 1766217891+2000);
  gr->GetXaxis()->SetTimeDisplay(1);
  gr->GetXaxis()->SetTimeFormat("%m/%d/%Y%F1970-01-01 00:00:00");
  gr->Draw("ALP");
  gr->SetMarkerStyle(kCircle);
  gr->SetMarkerSize(3);
  c1.SaveAs("timemark_investigation_2.png");
  
}
