#include "ROOT/RDataFrame.hxx"
#include "TSystem.h"
#include "TTree.h"
#include <iostream>
#include <map>

using ROOT::RDataFrame;
using ROOT::RDF::RResultPtr;

UInt_t rollover_difference_32bit(UInt_t big, UInt_t small)
{
  return big > small ? big-small : big + UINT32_MAX-small;
}

struct pps_and_their_diff 
{
  UInt_t last_pps = 0;
  UInt_t llast_pps = 0;
  UInt_t delta = 0; // last_pps - llast_pps, with rollover taken care of
};

// "promotes" the Y-value range of the graph from [0, UINT32_MAX) to [0, UINT64_MAX)
// warning: the first two seconds of the graph MUST have been removed as they contain garbage,
// and this function is not smart enough about that 
// (it only uses a slope to determine when a wrap-around has occured)
void unwrap(TGraph * gr) 
{
  // some arbitrary negative number to determine whehter a wrap-around has occured.
  // this number should be lenient enough -- that is, not exactly (-UINT32_MAX)
  // that said, how lenient is lenient seems to be somewhat arbitrary...
  const double SLOPE_TOLERANCE = -(double) UINT32_MAX / 5;

  int num_wraps = 0;
  for (int i=1; i < gr->GetN(); ++i)
  {
    double dy = (gr->GetPointY(i) + (double) UINT32_MAX * num_wraps) - gr->GetPointY(i-1);

    if (dy < SLOPE_TOLERANCE) 
    {
      num_wraps++;
    }
    gr->SetPointY(i, gr->GetPointY(i) + (double) num_wraps * UINT32_MAX);
  }
}

// Some assumptions about the data are made:
// (a)  The column `event_second` is monotonically increasing, ie "sorted"
// (b)  0 <= event_second[x] - event_second[x-1] <= 1
void header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");

  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture
  // RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/bfmr_r739_head.root");
  RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/real_R0813_head.root");

  std::map<UInt_t, pps_and_their_diff> encounters; // stores unique encounters of `event_second`
  TGraph evtsec_vs_lpps; // for interpolating/extrapolating last_pps at certain "bad" seconds
  ROOT::RVecD deltas;    // for computing the average delta

  auto search_and_fill = 
    [&encounters, &evtsec_vs_lpps, &deltas]
    (UInt_t event_second, UInt_t lpps, UInt_t llpps)
    {
      bool not_found = encounters.find(event_second) == encounters.end();
      if (not_found) {
        UInt_t diff = rollover_difference_32bit(lpps, llpps);
        encounters.emplace(
          event_second, 
          pps_and_their_diff{lpps, llpps, diff}
        );
        evtsec_vs_lpps.AddPoint(event_second, lpps);
        deltas.emplace_back(diff);
      }
    };
  tmp_header_rdf.Foreach(search_and_fill, {"triggerTime","lastPPS","lastLastPPS"});

  // remove first two seconds in the TGraph before unwrapping
  evtsec_vs_lpps.RemovePoint(0);
  evtsec_vs_lpps.RemovePoint(0);
  unwrap(&evtsec_vs_lpps);

  //  then re-evaluate with extrapolation
  UInt_t first_second = encounters.begin()->first;
  UInt_t second_second = std::next(encounters.begin(),1)->first;                 // + UINT32_MAX in case extrapolation is negative
  encounters[first_second].last_pps = (ULong64_t)(evtsec_vs_lpps.Eval(first_second) + UINT32_MAX) % UINT32_MAX;
  encounters[second_second].last_pps = (ULong64_t)(evtsec_vs_lpps.Eval(second_second) + UINT32_MAX) % UINT32_MAX;

                                   // ie. skip first two seconds when computing average delta
  deltas = ROOT::VecOps::Take(deltas, -deltas.size() + 2);
  double avg_delta = ROOT::VecOps::Mean(deltas);


  TCanvas c1("", "", 1920, 1080);
  evtsec_vs_lpps.Draw("ALP");
  evtsec_vs_lpps.SetMarkerStyle(kCircle);
  // evtsec_vs_lpps.GetXaxis()->SetRangeUser(evtsec_vs_lpps.GetPointX(0), evtsec_vs_lpps.GetPointX(0)+10);
  c1.SaveAs("bar.svg");
  exit(0);
}
