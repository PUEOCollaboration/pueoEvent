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
  UInt_t delta = 0; // end_pps - start_pps, with rollover taken care of
  UInt_t avg_delta = 0; // average delta (moving average)
};

using TimeTable=std::map<Long64_t, second_boundaries>;

TimeTable prep (TString header_file_name);
void print(TimeTable& time_table);
void plot (TimeTable& time_table, TString name="pps_correction.svg");

/* @param half_width Half width of the window when computing the moving average.
   @note  The deltas are nominally 125 MHz, but sometimes shit can glitch.
          That is, for some `event_second`, the delta would overshoot,
          and at a later `event_second`, its delta would undershoot.
          In other words, the former `event_second` is too long and the latter is too short.
          The opposite could also happen.
          The size of `half_width` depends on the magnitude of the over/undershoot that we would expect.
          e.g. If we can expect an error of size 100, then half_width of 5 seconds is probably fine,
          but if the error is of size 10000, the window needs to be larger so that the error 
          is distributed into each bin.
          However, obviously this window shouldn't be too large, else there's no point to performing
          a moving average. */
void simple_moving_average(TimeTable& time_table, int half_width = 5, bool ignore_last_two_row = true);

bool approx_equal(UInt_t a, UInt_t b, UInt_t tolerance = 20)
{
  UInt_t diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

void stupid_extrapolation(TimeTable& encounters, UInt_t avg_delta)
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
  }

  mid_point->second.corrected_start = mid_point->second.original_start;

  for (auto it = std::next(mid_point); it!=encounters.end(); ++it)
  {
    Long64_t diff_sec = it->first - mid_point->first;
    it->second.corrected_start = (mid_point->second.original_start) + diff_sec * avg_delta;
  }
}

// Some assumptions about the data are made:
// (a)  The column `event_second` (aka `triggerTime`) is monotonically increasing, ie "sorted"
// (b)  0 <= event_second[x] - event_second[x-1] <= 1
void header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  TimeTable time_table = prep("/usr/pueoBuilder/install/bin/real_R0813_head.root");
  // TimeTable time_table = prep("/usr/pueoBuilder/install/bin/bfmr_r739_head.root");

  simple_moving_average(time_table);
  print(time_table);

  exit(0);
}

TimeTable prep(TString header_file_name)
{
  TimeTable encounters; // this table only store unique `event_second`s

  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture
  ROOT::RDataFrame tmp_header_rdf("headerTree", header_file_name);

  Long64_t previous_previous = -2; // initialize first row of the table to garbage
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
        // note that this inserts garbage into `encounters` during the first two iterations, 
        // since the first second doesn't have a valid lpps.
        // This is okay since the first two rows are set to garbage anyways.
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
  return encounters;
}

void print(TimeTable& encounters)
{

  std::cout << "------------------------------------------------------------------------------\n"
            << " seconds     | original  | original  | original  | moving    |  corrected  \n"
            << " since epoch | start     | end       | delta     | avg delta |  start      \n"
            << "------------------------------------------------------------------------------\n";
  for (auto& e: encounters)
  {
    std::cout << " " << std::setw(13) << std::left << e.first
              << " " << std::setw(11) << e.second.original_start
              << " " << std::setw(11) << e.second.original_end
              << " " << std::setw(11) << e.second.delta
              << " " << std::setw(14) << e.second.avg_delta
              << " " << std::setw(11) << e.second.corrected_start << "\n";
  }
  std::cout << "------------------------------------------------------------------------------\n";
}

void plot(TimeTable& encounters, TString name)
{
  TGraph original(encounters.size());
  TGraph corrected(encounters.size());
  TGraph diff(encounters.size());

  std::size_t counter=0;
  for (auto& e: encounters){

    Long64_t o = e.second.original_start;
    original.SetPoint(counter, e.first,o);
    Long64_t c = e.second.corrected_start;
    corrected.SetPoint(counter, e.first, c);
    Long64_t d = o-c;
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

void simple_moving_average(TimeTable& time_table, int half_width, bool ignore_last_two_row)
{
  // moving average, carried out for most rows in the table
  auto start = std::next(time_table.begin(), half_width);
  auto stop = ignore_last_two_row ? std::prev(time_table.end(), half_width+2)
                                  : std::prev(time_table.end(), half_width);
  for(auto it= start; it!=stop; ++it)
  {
    ULong64_t sum = 0; // 64 bit, in case there's an overflow, although probably unlikely

    // it for iterator, so obviously jt is jiterator ¯\_(ツ)_/¯
    for (auto jt=std::prev(it,half_width); jt!=std::next(it,half_width+1); ++jt){
      sum += jt->second.delta;
    }
    it->second.avg_delta = sum / (2*half_width+1);
  }

  // As for the first/last few rows in the table,
  // I could probably do something more sophisticated, but nah let's just extrapolate
  for (auto it=time_table.begin(); it!=start; ++it){
    it->second.avg_delta = start->second.avg_delta;
  }
  for (auto it=stop; it!=time_table.end(); ++it){
    it->second.avg_delta = std::prev(stop)->second.avg_delta;
  }
};
