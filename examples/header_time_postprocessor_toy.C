#include "ROOT/RDataFrame.hxx"
#include "TF1.h"
#include "TAttMarker.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TLegend.h"
#include <iostream>
#include <iomanip>
#include <map>

// Nominally, delta = (end_pps - start_pps) should yield approximately 125M.
// The relative_delta means delta - 125M
struct second_boundaries 
{
  UInt_t original_start = 0; // value of the sysclk counter (ie pps) at the start of the second
  UInt_t original_end = 0;
  double corrected_start = 0;
  int    relative_delta = 0; // 125M - (end_pps - start_pps), with rollover taken care of
  double avg_relative_delta = 0; // average delta (moving average)
  bool   print_bold_green = false;
};

using TimeTable=std::map<Long64_t, second_boundaries>;

TimeTable prep (TString header_file_name);
void print(TimeTable& time_table);
void plot (TimeTable& time_table, TString name="pps_correction.svg");

// An attempt at correcting the start and end of each second, via calculating the avg delta and extrapolations.
void stupid_extrapolation(TimeTable& time_table, std::size_t stable_period);

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

// Another attempt at correcting the start and end of each second, via TGraph::Fit().
void linear_fit(TimeTable& t);

/* Returns a TGraph of the pps value at the start of each `event_second`.
The Y-values range from [0, UINT64_MAX) instead of [0, UINT32_MAX); that is, unwrapped.
The X-values are relative to t0; that is, instead of starting from ~1.7 billion seconds, it starts from 0.
This t0 offset is needed to perform a fit of the graph, else the result would be horrible.
Note that the TGraph does not contain the final row of the time table (ie the final second),
since the final second does not have a valid start pps (that'll have to be extrapolated).
The unwrapping is kinda dumb because the function only uses a slope to determine whether a wrap-around has occured.
*/
TGraph naive_unrawp(TimeTable& time_table);

// Some assumptions about the data are made:
// (a)  The column `event_second` (aka `triggerTime`) is monotonically increasing, ie "sorted"
// (b)  0 <= event_second[x] - event_second[x-1] <= 1
void header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  TimeTable time_table = prep("/work/real_run_1324_header.root");
  // TimeTable time_table = prep("/usr/pueoBuilder/install/bin/real_R0813_head.root");
  // TimeTable time_table = prep("/usr/pueoBuilder/install/bin/bfmr_r739_head.root");

  /****************** First Attempt *********************/
  simple_moving_average(time_table);
  std::size_t stable_period = time_table.size() / 3;
  stupid_extrapolation(time_table, stable_period);
  print(time_table);
  plot(time_table);

  /****************** Second Attempt *********************/
  // linear_fit(time_table);
  // print(time_table);
  // plot(time_table, "v2_correction.svg");


  exit(0);
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

void simple_moving_average(TimeTable& time_table, int half_width, bool ignore_last_two_row)
{
  // moving average, carried out for most rows in the table
  auto start = std::next(time_table.begin(), half_width);
  auto stop = ignore_last_two_row ? std::prev(time_table.end(), half_width+2)
                                  : std::prev(time_table.end(), half_width);
  for(auto it= start; it!=stop; ++it)
  {
    double sum = 0.; // 64 bit, in case there's an overflow, although probably unlikely

    // it for iterator, so obviously jt is jiterator ¯\_(ツ)_/¯
    for (auto jt=std::prev(it,half_width); jt!=std::next(it,half_width+1); ++jt){
      sum += jt->second.relative_delta;
    }
    it->second.avg_relative_delta = sum / (2*half_width+1);
  }

  // As for the first/last few rows in the table,
  // I could probably do something more sophisticated, but nah let's just extrapolate
  for (auto it=time_table.begin(); it!=start; ++it){
    it->second.avg_relative_delta = start->second.avg_relative_delta;
  }
  for (auto it=stop; it!=time_table.end(); ++it){
    it->second.avg_relative_delta = std::prev(stop)->second.avg_relative_delta;
  }
};

void stupid_extrapolation(TimeTable& time_table, std::size_t stable_period)
{
  // find the mid-point of a "stable region" where, for every second, its delta is approximately avg_relative_delta
  auto mid_point = time_table.begin();
  std::vector<ULong64_t> stable_seconds;
  stable_seconds.reserve(stable_period);

  for (auto &e: time_table) 
  {
    if (stable_seconds.size() == stable_period) break; 

    if (approx_equal(e.second.relative_delta , e.second.avg_relative_delta)) stable_seconds.emplace_back(e.first);
    else stable_seconds.clear();
  }

  if (stable_seconds.size() != stable_period) 
  {
    print(time_table);
    fprintf(
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `end_pps` - `start_pps` are all approximately 125 million clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      stable_period, mid_point->first
    );
  } else {
    mid_point = time_table.find(stable_seconds.at(stable_period/2));
  }
  // check again, for safety
  if (mid_point == time_table.end()) 
  {
    fprintf(
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `end_pps` - `start_pps` are all approximately 125 million clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      stable_period, mid_point->first
    );
    mid_point = time_table.begin();
  }

  std::cout << "found mid_point: " << mid_point->first << '\n';

  mid_point->second.corrected_start = mid_point->second.original_start;
  mid_point->second.print_bold_green = true;

  for (auto it = std::prev(mid_point); it!=std::prev(time_table.begin()); --it){
    auto future = std::next(it);
    it->second.corrected_start = 
      std::fmod(
        future->second.corrected_start - (125000000 + future->second.avg_relative_delta)
        +static_cast<double>(UINT32_MAX) + 1 
        ,
        static_cast<double>(UINT32_MAX) + 1
      );
  }

  for (auto it = std::next(mid_point); it!=time_table.end(); ++it){
    auto past = std::prev(it);
    it->second.corrected_start = 
      std::fmod(
        past->second.corrected_start + (125000000 + past->second.avg_relative_delta)
        +static_cast<double>(UINT32_MAX) + 1 
        ,
        static_cast<double>(UINT32_MAX) + 1
      );
  }
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
        encounters[previous_previous].relative_delta = (lpps - encounters[previous_previous].original_start) - 125000000;
        // note: for unsigned integers, wrap-around subtraction 
        //   `lpps - encounters[previous_previous].original_start`
        // is automaticlly taken care of.
        
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


  std::cout << "\nRelative delta is defined to be 125000000 - (end_pps - start_pps)\n\n";

  std::cout << "--------------------------------------------------------------------\n"
            << " seconds     | start pps | end pps   | rel   | avg rel | corrected  \n"
            << " since epoch |           |           | delta | delta   | start pps  \n"
            << " Long64_t    | UInt_t    | UInt_t    | int   | double  | double     \n"
            << "--------------------------------------------------------------------\n";
  for (auto& e: encounters)
  {
    const char * space = e.second.print_bold_green ? "\033[1;32m " : "\033[0m ";
    std::cout << space << std::setw(13) << std::left << e.first
              << space << std::setw(11) << e.second.original_start
              << space << std::setw(11) << e.second.original_end
              << space << std::setw(7)  << e.second.relative_delta
              << space << std::setw(6)  << std::fixed << std::setprecision(2) << e.second.avg_relative_delta
              << space << std::setw(15) << std::right << std::fixed << std::setprecision(2) << e.second.corrected_start << "\n";
  }
  std::cout << "------------------------------------------------------------------------------\n";
}

void plot(TimeTable& encounters, TString name)
{
  TGraph original(encounters.size());
  TGraph corrected(encounters.size());
  TGraph diff(encounters.size());

  TGraph original_delta(encounters.size());
  TGraph avg_delta(encounters.size());

  std::size_t counter=0;
  for (auto& e: encounters){

    Long64_t o = e.second.original_start;
    original.SetPoint(counter, e.first,o);
    double c = e.second.corrected_start;
    corrected.SetPoint(counter, e.first, c);
    double d = o-c;
    diff.SetPoint(counter, e.first, d);

    original_delta.SetPoint(counter, e.first, e.second.relative_delta);
    avg_delta.SetPoint(counter, e.first, e.second.avg_relative_delta);

    counter++;
  }
  original.RemovePoint(original.GetN()-1); // final second doesn't have a valid original_start
  diff.RemovePoint(diff.GetN()-1);         // final second doesn't have a valid original_start
  original_delta.RemovePoint(original_delta.GetN()-1); // final two seconds don't have valid deltas
  original_delta.RemovePoint(original_delta.GetN()-1); // final two seconds don't have valid deltas

  TCanvas c1(name, name, 1920 * 1.5, 1080 * 2);
  c1.Divide(1,3);
  c1.cd(1);
  original.Draw("ALP");
  original.SetMarkerStyle(kFullCrossX);
  original.SetMarkerSize(3);
  original.SetTitle("System Clock Value (uint32_t) at Start of Each Second (aka \"start pps\")");
  original.GetXaxis()->SetLabelSize(0);
  original.GetYaxis()->SetTitle("[sysclk counts]");
  original.GetYaxis()->SetTitleSize(0.1);
  original.GetYaxis()->SetTitleOffset(0.3);
  original.GetYaxis()->CenterTitle();
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
  original_delta.Draw("ALP");
  original_delta.SetMarkerStyle(kFullCrossX);
  original_delta.SetMarkerSize(3);
  original_delta.SetTitle("#Delta #equiv end_pps - start_pps ≈ 125,000,000");
  original_delta.GetYaxis()->SetTitle("#Delta - 125 MHz [sysclk counts]");
  original_delta.GetYaxis()->CenterTitle();
  original_delta.GetYaxis()->SetTitleSize(0.1);
  original_delta.GetYaxis()->SetTitleOffset(0.3);
  original_delta.GetXaxis()->SetLabelSize(0);

  avg_delta.Draw("P");
  avg_delta.SetMarkerStyle(kCircle);
  avg_delta.SetMarkerColor(kRed);

  TLegend leg3(0.6, 0.1, 0.9, 0.3); // (x1,y1,x2,y2) in NDC
  leg3.AddEntry(&original_delta, "original (discrete) delta (uint32_t)", "p");
  leg3.AddEntry(&corrected, "smoothed (moving averaged) delta (double_t)", "p");
  leg3.Draw();

  c1.cd(3);
  diff.Draw("ALP");
  diff.SetTitle("(original start pps) - (corrected start pps)");
  diff.SetMarkerStyle(kCircle);
  diff.GetYaxis()->SetTitle("[sysclk counts]");
  diff.GetYaxis()->CenterTitle();
  diff.GetYaxis()->SetTitleSize(0.1);
  diff.GetYaxis()->SetTitleOffset(0.3);
  diff.GetXaxis()->SetTitle("Event Second [seconds since Unix epoch]");
  diff.GetXaxis()->SetTitleOffset(2.2);
  diff.GetXaxis()->CenterTitle();
  diff.GetXaxis()->SetLabelOffset(0.05);


  c1.SaveAs(name);
}
