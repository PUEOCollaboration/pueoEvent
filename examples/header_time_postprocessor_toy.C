#include "ROOT/RDataFrame.hxx"
#include "TAttMarker.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include <iostream>
#include <iomanip>
#include <map>

struct second_boundaries 
{
  UInt_t original_start = 0; // value of the sysclk counter (ie pps) at the start of the second
  UInt_t original_end = 0;
  UInt_t corrected_start = 0;
  UInt_t delta = 0; // end_pps - start_pps, with rollover taken care of
};

using TimeTable=std::map<Long64_t, second_boundaries>;

// average value of (original_end-original_start); the final two seconds should be excluded when computing this,
// because they don't have valid (start, end) pairs.
UInt_t average_delta(TimeTable& t, Long64_t exclude1, Long64_t exclude2);

bool approx_equal(UInt_t a, UInt_t b, UInt_t tolerance = 20)
{
  UInt_t diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

// first attempt at correcting the start and end of each second via a simple extrapolation
void stupid_extrapolation(TimeTable& t, UInt_t avg_delta);

// Second attempt at correcting the start and end of each second, via TGraph::Fit().
void linear_fit(TimeTable& t);

// Returns a TGraph of the pps value at the start of each `event_second`.
// The Y-values range from [0, UINT64_MAX) instead of [0, UINT32_MAX); that is, unwrapped.
// The X-values are relative to t0; that is, instead of starting from ~1.7 billion seconds, it starts from 0.
// This t0 offset is needed to perform a fit of the graph, else the result would be horrible.
// Note that the TGraph does not contain the final row of the time table (ie the final second),
// since the final second does not have a valid start pps (that'll have to be extrapolated).
// The unwrapping is kinda dumb because the function only uses a slope to determine whether a wrap-around has occured.
TGraph naive_unrawp(TimeTable& encounters);

void print(TimeTable& utcSecond_start_end_delta_start_end);
void plot (TimeTable& utcSecond_start_end_delta_start_end, TString name="pps_correction.svg");

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
  TimeTable encounters; // this map only stores unique seconds

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

  /****************** First Attempt *********************/
  // UInt_t avg_delta = average_delta(encounters, previous_second, previous_previous);
  // stupid_extrapolation(encounters, avg_delta);
  // plot(encounters, "foo.svg");

  /****************** Second Attempt *********************/
  linear_fit(encounters);
  print(encounters);
  plot(encounters);

  exit(0);
}

void linear_fit(TimeTable& encounters)
{
  TGraph gr = naive_unrawp(encounters);     // start_pps (unwrapped to 64 bit) vs. event_second 
  Long64_t t0 = encounters.begin()->first;  // first second of the run

  // use the unwrapped graph to fit a linear function
  gr.Fit("pol1", "0");
  TF1 * fun = gr.GetFunction("pol1");

  // use the fit to correct the start of each second
  for(int i=0; i<gr.GetN(); ++i)
  {
    Long64_t relative_sec = gr.GetPointX(i);
    Long64_t utc_sec = relative_sec + t0;

    auto row = encounters.find(utc_sec);

    if (row==encounters.end())
    {
      std::cerr << "something terrible has happened.\n";
      exit(1);
    } else {
      ULong64_t corrected_start_pps = (ULong64_t)fun->Eval(relative_sec) % ((ULong64_t)UINT32_MAX + 1);
      row->second.corrected_start = static_cast<UInt_t>(corrected_start_pps);
    }
  }

  // the final second is not in the graph, but it can be extrapolated
  ULong64_t final_sec = std::prev(encounters.end())->first;
  ULong64_t final_relative_sec = final_sec - t0;

  auto last_row = encounters.find(final_sec);
  if (last_row == encounters.end()) 
  {
    std::cerr << "something horrible has happened.\n";
    exit(1);
  }
  last_row->second.corrected_start = static_cast<UInt_t>(
    (ULong64_t)fun->Eval(final_relative_sec) % ((ULong64_t)UINT32_MAX + 1)
  );
}

TGraph naive_unrawp(TimeTable& encounters) 
{
  // some arbitrary negative number to determine whether a wrap-around has occured.
  // this number should be lenient enough -- that is, not exactly (-UINT32_MAX)
  // that said, how lenient is lenient seems to be somewhat arbitrary...
  const Long64_t SLOPE_TOLERANCE = - (Long64_t) UINT32_MAX / 5;

  TGraph gr(encounters.size()-1); // pre-allocate size()-1 points (ie final second doesn't have a valid start)
  Long64_t t0 = encounters.begin()->first; // all x-values are relative to this number

  int num_wraps = 0; // number of wrap-arounds
  int idx= 0;  // add the first row of the time table
  gr.SetPoint(idx, encounters.begin()->first - t0, encounters.begin()->second.original_start);

  // iterate starting from the second row to the second-to-last row,
  // since in each iteration we need to refer to the previous row;
  // last row is excluded because the final second doesn't have a valid start pps.
  for(auto it=std::next(encounters.begin()); it!=std::prev(encounters.end()); ++it)
  {
    auto pr = std::prev(it);
    Long64_t this_x = (Long64_t) it->first;
    Long64_t this_y = (Long64_t) it->second.original_start;
    Long64_t prev_y = (Long64_t) pr->second.original_start;

    if (this_y - prev_y < SLOPE_TOLERANCE) num_wraps++;

    gr.SetPoint(++idx, this_x - t0, this_y + num_wraps * (ULong64_t) UINT32_MAX);
  }

  return gr;
}

UInt_t average_delta(TimeTable& encounters, Long64_t exclude1, Long64_t exclude2)
{
  ULong64_t sum = 0;
  for (auto& e: encounters) sum += e.second.delta;
  sum -= encounters[exclude1].delta;
  sum -= encounters[exclude2].delta;
  sum /= encounters.size()-2;
  return sum;
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

void print(TimeTable& encounters)
{

  std::cout << "-------------------------------------------------------------------------------\n"
            << "  seconds      |   original                        |    corrected                 \n"
            << "  since epoch  |   start   |    end    |  delta    |    start                     \n"
            << "-------------------------------------------------------------------------------\n";
  for (auto& e: encounters)
  {
    std::cout << std::setw(15) << std::left << e.first
              << " " << std::setw(11) << e.second.original_start
              << " " << std::setw(11) << e.second.original_end
              << " " << std::setw(11) << e.second.delta
              << " " << std::setw(11) << e.second.corrected_start << "\n";
  }
}

void plot(TimeTable& encounters, TString name)
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
