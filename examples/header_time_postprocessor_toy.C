#include "ROOT/RDataFrame.hxx"
#include "TAttMarker.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TLegend.h"
#include <iostream>
#include <iomanip>
#include <map>

struct second_boundaries 
{
  UInt_t original_start = 0; // value of the sysclk counter (ie pps) at the start of the second
  UInt_t original_end = 0;
  UInt_t corrected_start = 0;
  UInt_t corrected_end = 0;
  UInt_t delta = 0; // end_pps - start_pps, with rollover taken care of
};

// average value of (original_end-original_start); the final two seconds should be excluded when computing this,
// because they don't have valid (start, end) pairs.
UInt_t average_delta(std::map<Long64_t, second_boundaries>& encounters, Long64_t exclude1, Long64_t exclude2);

// first attempt at correcting the start and end of each second via extrapolation
void stupid_extrapolation(std::map<Long64_t, second_boundaries>& encounters, UInt_t avg_delta);

void print(std::map<Long64_t, second_boundaries>& utcSecond_start_end_delta_start_end);
void plot (std::map<Long64_t, second_boundaries>& utcSecond_start_end_delta_start_end, TString name="pps_correction.svg");
bool approx_equal(UInt_t a, UInt_t b, UInt_t tolerance = 20)
{
  UInt_t diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

// Some assumptions about the data are made:
// (a)  The column `event_second` (aka `triggerTime`) is monotonically increasing, ie "sorted"
// (b)  0 <= event_second[x] - event_second[x-1] <= 1
void header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture

  // ROOT::RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/bfmr_r739_head.root");
  ROOT::RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/real_R0813_head.root");
  std::map<Long64_t, second_boundaries> encounters; // this map only stores unique seconds

  Long64_t previous_previous = -2; // initialize to garbage
  Long64_t previous_second   = -1;
  auto search_and_fill = 
    [&encounters, &previous_second, &previous_previous]
    (UInt_t event_second, UInt_t lpps)
    {
      Long64_t evtsec = (Long64_t) event_second;
      bool new_encounter = encounters.find(evtsec) == encounters.end();
      if (new_encounter) 
      {
        encounters.emplace(evtsec, second_boundaries{});
        // the start of the previous second is the lpps of the current second
        // note that this inserts garbage into `encounters` during the first two iterations;
        encounters[previous_second].original_start = lpps;
        encounters[previous_previous].original_end = lpps;
        encounters[previous_previous].delta = lpps - encounters[previous_previous].original_start;
        // note: for unsigned integers, wrap-around subtraction is automaticlly taken care of
        
        previous_previous = previous_second;
        previous_second = evtsec;
      }
    };
  tmp_header_rdf.Foreach(search_and_fill, {"triggerTime","lastPPS"});
  encounters.erase(-2);  // erase the garbage
  encounters.erase(-1);
  // print(encounters);

  UInt_t avg_delta = average_delta(encounters, previous_second, previous_previous);
  stupid_extrapolation(encounters, avg_delta);
  // print(encounters);
  plot(encounters);

  exit(0);
}

UInt_t average_delta(std::map<Long64_t, second_boundaries>& encounters, Long64_t exclude1, Long64_t exclude2)
{
  ULong64_t sum = 0;
  for (auto& e: encounters) sum += e.second.delta;
  sum -= encounters[exclude1].delta;
  sum -= encounters[exclude2].delta;
  sum /= encounters.size()-2;
  return sum;
}

void stupid_extrapolation(std::map<Long64_t, second_boundaries>& encounters, UInt_t avg_delta)
{
  // find the mid-point of a "stable region" where the delta's are all approximately avg_delta
  std::size_t stable_period = encounters.size() / 3;
  auto mid_point = encounters.begin();
  std::vector<ULong64_t> stable_seconds;
  stable_seconds.reserve(stable_period);

  for (auto &e: encounters) 
  {
    if (stable_seconds.size() == stable_period) break; 

    if (approx_equal(e.second.delta , avg_delta)) stable_seconds.emplace_back(e.first);
    else stable_seconds.clear();
  }

  if (stable_seconds.size() != stable_period) 
  {
    print(encounters);
    fprintf(
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `end_pps` - `start_pps` are all approximately %u clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      stable_period, avg_delta, mid_point->first
    );
  } else {
    mid_point = encounters.find(stable_seconds.at(stable_period/2));
  }

  for (auto it = encounters.begin(); it!=mid_point; ++it)
  {
    Long64_t diff_sec = it->first - mid_point->first;
    it->second.corrected_start = (mid_point->second.original_start) + diff_sec * avg_delta;
    it->second.corrected_end = (mid_point->second.original_end) + diff_sec * avg_delta;
  }

  mid_point->second.corrected_start = mid_point->second.original_start;
  mid_point->second.corrected_end = mid_point->second.original_end;

  for (auto it = std::next(mid_point); it!=encounters.end(); ++it)
  {
    Long64_t diff_sec = it->first - mid_point->first;
    it->second.corrected_start = (mid_point->second.original_start) + diff_sec * avg_delta;
    it->second.corrected_end = (mid_point->second.original_end) + diff_sec * avg_delta;
  }
}

void print(std::map<Long64_t, second_boundaries>& encounters)
{

  std::cout << "-------------------------------------------------------------------------------\n"
            << "            |   original                        |    corrected                 \n"
            << "   second   |   start   |    end    |  delta    |    start   |    end    " << "\n"
            << "-------------------------------------------------------------------------------\n";
  for (auto& e: encounters)
  {
    std::cout << std::setw(11) << e.first
              << " " << std::setw(11) << e.second.original_start
              << " " << std::setw(11) << e.second.original_end
              << " " << std::setw(11) << e.second.delta
              << " " << std::setw(11) << e.second.corrected_start
              << " " << std::setw(11) << e.second.corrected_end << "\n";
  }
}

void plot(std::map<Long64_t, second_boundaries>& encounters, TString name)
{
  TGraph original(encounters.size());
  TGraph corrected(encounters.size());
  TGraph diff(encounters.size());

  std::size_t counter=0;
  for (auto& e: encounters){

    UInt_t o = e.second.original_start;
    original.SetPoint(counter, e.first,o);
    UInt_t c = e.second.corrected_start;
    corrected.SetPoint(counter, e.first, c);
    int d = o < c ? c - o : o - c;
    diff.SetPoint(counter, e.first, d);
    counter++;
  }
  diff.RemovePoint(original.GetN()-1); // last second's start doesn't exist before correction

  TCanvas c1(name, name, 1920 * 1.5, 1080 * 2);
  c1.Divide(1,2);
  c1.cd(1);
  original.Draw("ALP");
  original.SetMarkerStyle(kFullCrossX);
  original.SetMarkerSize(3);
  original.SetTitle("`Event_Second` Boundaries");
  original.GetYaxis()->SetTitle("PPS [sysclk count]");
  original.GetYaxis()->CenterTitle();
  original.GetXaxis()->SetTitle("Event Second [seconds since Unix epoch]");
  original.GetXaxis()->CenterTitle();
  original.GetXaxis()->SetLabelOffset(0.1);
  corrected.Draw("P");
  corrected.SetMarkerStyle(kCircle);
  corrected.SetMarkerColor(kRed);
  corrected.SetMarkerSize(2);

  double y0 = UINT32_MAX;
  TLine line(original.GetPointX(0), y0, original.GetPointX(original.GetN()-1), y0);
  line.SetLineStyle(2);   // dashed
  line.Draw();

  TLegend leg(0.1, 0.8, 0.25, 0.9); // (x1,y1,x2,y2) in NDC
  leg.AddEntry(&original, "Original", "p");
  leg.AddEntry(&corrected, "Corrected", "p");
  leg.AddEntry(&line, "UINT 32Bit MAX", "l");
  leg.Draw();
  
  c1.cd(2);
  diff.Draw("ALP");
  diff.SetTitle("");
  diff.SetMarkerStyle(kCircle);
  diff.GetXaxis()->SetLabelSize(0);
  diff.GetYaxis()->SetTitle("abs(original-corrected) [sysclk counts]");
  diff.GetYaxis()->CenterTitle();

  c1.SaveAs(name);
}
