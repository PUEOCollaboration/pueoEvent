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

/* Nominally, delta = (end_pps - start_pps) should yield approximately 125E6 
 * (unsigned integer overflow should be taken care of when computing the difference).
 * `relative_delta` is then defined as (delta - 125E6). */
struct second_boundaries 
{
  UInt_t original_start = 0;     // value of sysclk counter at start of the second (start_pps)
  UInt_t original_end = 0;       // end_pps
  int    relative_delta = 0;     // (end_pps - start_pps) - 125E6; unsigned int overflow taken care of
  double avg_relative_delta = 0; // average delta (moving average)
  double corrected_start = 0;    // to be corrected using avg_relative_delta
  bool   print_bold_green = false;
};

using TimeTable=std::map<Long64_t, second_boundaries>;

TimeTable prep (TString header_file_name);
void print(TimeTable& time_table, std::size_t num_rows = 100);
void plot (TimeTable& time_table, TString name="pps_correction.svg");

// Finds the mid-point of a "stable region" where, for every second, its delta is approximately avg_relative_delta.
// Returns time_table.begin() if a stable region couldn't be found
TimeTable::iterator find_stable_region_mid_point(
  std::size_t stable_period, TimeTable& time_table, bool ignore_last_two_row = true);

// computes `corrected_start` for each second by calling `simple_moving_average()`
void stupid_extrapolation(TimeTable& time_table, TimeTable::iterator anchor_point);

/* Computes `avg_relative_delta` for each second using neighboring seconds. CRASHES if the run has too few seconds.
 * @param half_width Half width of the window when computing the moving average.
 * @note  The deltas are nominally 125 MHz, but sometimes shit can glitch.
 *        That is, for some `event_second`, the delta would overshoot,
 *        and at a later `event_second`, its delta would undershoot.
 *        In other words, the former `event_second` is too long and the latter is too short.
 *        The opposite could also happen.
 *        The size of `half_width` depends on the magnitude of the over/undershoot that we would expect.
 *        e.g. If we can expect an error of size 100, then half_width of 5 seconds is probably fine,
 *        but if the error is of size 10000, the window needs to be larger so that the error 
 *        distributed into each bin is small enough.
 *        However, obviously this window shouldn't be too large, else there's no point to performing
 *        a moving average. */
void simple_moving_average(TimeTable& time_table, std::size_t half_width = 5, bool ignore_last_two_row = true);

/* @brief The "main" function.
 * @note  Some assumptions about the data are made:
 *        (a)  The column `event_second` (aka `triggerTime`) is monotonically increasing, ie "sorted"
 *        (b)  0 <= event_second[x] - event_second[x-1] <= 1 */
void header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  // TimeTable time_table = prep("/work/R1392_header.root");
  // TimeTable time_table = prep("/work/real_run_1324_header.root");
  TimeTable time_table = prep("/usr/pueoBuilder/install/bin/bfmr_r739_head.root");

  simple_moving_average(time_table);
  std::size_t stable_period = time_table.size() / 3;
  TimeTable::iterator mid_point = find_stable_region_mid_point(stable_period, time_table);
  // stupid_extrapolation(time_table, mid_point);
  // print(time_table);
  // fprintf(stdout, "There are %lld seconds in this run.\n", 
  //         std::prev(time_table.end())->first - time_table.begin()->first);
  //
  // plot(time_table);

  exit(0);
}

void simple_moving_average(TimeTable& time_table, std::size_t half_width, bool ignore_last_two_row)
{
  if (half_width == 0) 
  {
    std::cerr << __PRETTY_FUNCTION__ << "\n\t bad value supplied to parameter `half_width` (" 
              << half_width << "), resetting it to 5 [seconds].\n";
    half_width=5;
  }
  std::size_t valid_table_size = ignore_last_two_row ? time_table.size() - 2 : time_table.size();

  if (valid_table_size < 5)  // fuck it, if the run has less than 5 seconds, crash the program
  {
    std::cerr << __PRETTY_FUNCTION__ << "\n\tFatal Error: parameter `time_table` too short ("
              << valid_table_size    << " rows).";
    exit(1);
  }

  if (2 * half_width + 1 > valid_table_size)
  {
    std::cerr << __PRETTY_FUNCTION__ << "\n\tFatal Error: parameter `time_table` too short ("
              << valid_table_size    << " rows).";
    exit(1);
  }

  TimeTable::iterator start = std::next(time_table.begin(), half_width);
  TimeTable::iterator stop = ignore_last_two_row ? std::prev(time_table.end(), half_width+2)
                                                 : std::prev(time_table.end(), half_width);

  // first, perform moving average for the "meat" of the table (ie exclude first/last few rows)
  for(TimeTable::iterator it = start; it!=stop; ++it)
  {
    double sum = 0.;
    // it stands for iterator, so obviously jt is jiterator ¯\_(ツ)_/¯
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

template<typename T> bool approx_equal(T a, T b, T tolerance = 20)
{
  T diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

TimeTable::iterator find_stable_region_mid_point(
  std::size_t requested_stable_period, TimeTable& time_table, bool ignore_last_two_row) 
{
  TimeTable::iterator mid_point = time_table.begin();
  TimeTable::iterator stop = ignore_last_two_row ? std::prev(time_table.end(), 2) : time_table.end();

  std::vector<ULong64_t> stable_seconds;
  stable_seconds.reserve(requested_stable_period);

  for (auto it=time_table.begin(); it!=stop; ++it) 
  {
    if (stable_seconds.size() == requested_stable_period) break; 

    if (approx_equal<double>(it->second.relative_delta , it->second.avg_relative_delta)) 
      stable_seconds.emplace_back(it->first);
    else stable_seconds.clear();
  }

  if (stable_seconds.size() != requested_stable_period) 
  {
    print(time_table);
    fprintf
    (
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `end_pps` - `start_pps` are all approximately 125 million clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      requested_stable_period, time_table.begin()->first
    );
  } else {
    mid_point = time_table.find(stable_seconds.at(requested_stable_period/2));
  }

  if (mid_point == time_table.end()) // check again, in case the find() above returns a invalid result
  {
    fprintf
    (
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `end_pps` - `start_pps` are all approximately 125 million clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      requested_stable_period, time_table.begin()->first
    );
    mid_point = time_table.begin();
  }

  return mid_point;
}

void stupid_extrapolation(TimeTable& time_table, TimeTable::iterator anchor_point)
{
  anchor_point->second.corrected_start = anchor_point->second.original_start;
  anchor_point->second.print_bold_green = true;

  // check required, since std::prev(map.begin()) is undefined behavior
  if (anchor_point!=time_table.begin())
  {
    // backward extrapolation from the anchor point
    for (auto it = std::prev(anchor_point); it!=time_table.begin(); --it)
    {
      auto future = std::next(it);
      it->second.corrected_start = 
        std::fmod(
          future->second.corrected_start - (125000000 + it->second.avg_relative_delta) +
          static_cast<double>(UINT32_MAX) + 1,
          static_cast<double>(UINT32_MAX) + 1
        );
    }
    // handle first row explicitly
    TimeTable::iterator beg = time_table.begin();
    TimeTable::iterator fut = std::next(time_table.begin());
    beg->second.corrected_start = 
      std::fmod(
        fut->second.corrected_start - (125000000 + beg->second.avg_relative_delta) +
        static_cast<double>(UINT32_MAX) + 1,
        static_cast<double>(UINT32_MAX) + 1
      );
  }
  // forward extrapolation
  for (auto it = std::next(anchor_point); it!=time_table.end(); ++it)
  {
    auto past = std::prev(it);
    it->second.corrected_start = 
      std::fmod(
        past->second.corrected_start + (125000000 + past->second.avg_relative_delta)+
        static_cast<double>(UINT32_MAX) + 1,
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
        // note: use UInt_t so that wrap-around subtraction is automatic
        UInt_t delta = encounters[previous_previous].original_end - encounters[previous_previous].original_start;
        // and then convert to int so that values below nominal show up as negative
        encounters[previous_previous].relative_delta =  static_cast<int>(delta) - 125000000;
        
        previous_previous = previous_second;
        previous_second = evtsec;
      }
    };
  tmp_header_rdf.Foreach(search_and_fill, {"triggerTime","lastPPS"});
  encounters.erase(-2);  // erase the garbage
  encounters.erase(-1);
  return encounters;
}

void print(TimeTable& time_table, std::size_t num_rows)
{

  std::cout << "\nRelative delta is defined to be 125000000 - (end_pps - start_pps)\n\n";

  std::cout << "--------------------------------------------------------------------\n"
            << " seconds     | start pps | end pps   | rel   | avg rel | corrected  \n"
            << " since epoch |           |           | delta | delta   | start pps  \n"
            << " Long64_t    | UInt_t    | UInt_t    | int   | double  | double     \n"
            << "--------------------------------------------------------------------\n";
  for (auto it=time_table.begin(); it!=std::next(time_table.begin(), num_rows); ++it)
  {
    const char * space = it->second.print_bold_green ? "\033[1;32m " : "\033[0m ";
    std::cout << space << std::setw(13) << std::left << it->first
              << space << std::setw(11) << it->second.original_start
              << space << std::setw(11) << it->second.original_end
              << space << std::setw(7)  << it->second.relative_delta
              << space << std::setw(6)  << std::fixed << std::setprecision(2) << it->second.avg_relative_delta
              << space << std::setw(15) << std::right << std::fixed << std::setprecision(2) << it->second.corrected_start << "\n";
  }

  if (num_rows < time_table.size()){
    std::cout << "...\n";
    std::cout << "...\n";
    std::cout << "...\n";
  }
  std::cout << "------------------------------------------------------------------------------\n";
}

void plot(TimeTable& encounters, TString name)
{
  Long64_t t0 = encounters.begin()->first;

  TGraph original(encounters.size());
  TGraph corrected(encounters.size());
  TGraph diff(encounters.size());

  TGraph original_delta(encounters.size());
  TGraph avg_delta(encounters.size());

  std::size_t counter=0;
  for (auto& e: encounters){

    Long64_t o = e.second.original_start;
    original.SetPoint(counter, e.first-t0,o);
    double c = e.second.corrected_start;
    corrected.SetPoint(counter, e.first-t0, c);
    double d = o-c;
    diff.SetPoint(counter, e.first-t0, d);

    original_delta.SetPoint(counter, e.first-t0, e.second.relative_delta);
    avg_delta.SetPoint(counter, e.first-t0, e.second.avg_relative_delta);

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
  // original.GetXaxis()->SetRange(0, 1000);
  corrected.Draw("P");
  corrected.SetMarkerStyle(kCircle);
  corrected.SetMarkerColor(kRed);
  corrected.SetMarkerSize(2);

  double y0 = UINT32_MAX;
  TLine line(original.GetPointX(0), y0, original.GetPointX(original.GetN()-1), y0);
  line.SetLineStyle(2);   // dashed
  line.Draw();

  TLegend leg(0.6, 0, 0.9, 0.1); // (x1,y1,x2,y2) in NDC
  leg.AddEntry(&original, "Original", "p");
  leg.AddEntry(&corrected, "Corrected", "p");
  leg.AddEntry(&line, "UINT 32Bit MAX", "l");
  leg.Draw();

  c1.cd(2);
  original_delta.Draw("ALP");
  original_delta.SetMarkerStyle(kFullCrossX);
  original_delta.SetMarkerSize(3);
  original_delta.SetTitle("#Delta #equiv end_pps - start_pps ≈ 125,000,000");
  original_delta.GetYaxis()->SetTitle("#Delta - 125E6 [sysclk counts]");
  original_delta.GetYaxis()->CenterTitle();
  original_delta.GetYaxis()->SetTitleSize(0.1);
  original_delta.GetYaxis()->SetTitleOffset(0.3);
  original_delta.GetXaxis()->SetLabelSize(0);
  // original_delta.GetXaxis()->SetRange(0, 1000);

  avg_delta.Draw("P");
  avg_delta.SetMarkerStyle(kCircle);
  avg_delta.SetMarkerColor(kRed);

  TLegend leg3(0.6, 0.0, 0.9, 0.1); // (x1,y1,x2,y2) in NDC
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
  diff.GetXaxis()->SetTitle(Form("Event Second [seconds since t0 (%lld)]", t0));
  diff.GetXaxis()->SetTitleOffset(2.2);
  diff.GetXaxis()->CenterTitle();
  diff.GetXaxis()->SetLabelOffset(0.05);
  // diff.GetXaxis()->SetRange(0, 1000);

  c1.SaveAs(name);
}
