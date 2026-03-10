#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "TAttMarker.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TLegend.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

constexpr int NOMINAL_CLOCK_FREQ=125000000;

// Nominally, delta := (next_pps - this_pps) should yield approximately 125E6 
// (unsigned integer overflow should be taken care of when computing this difference).
// `relative_delta` is then defined as (delta - 125E6).
struct event_second_start_end 
{
  UInt_t last_pps = 0;           // value of the sysclk counter at the start of the previous second
  UInt_t this_pps = 0;           // extracted from the last_pps of the next second.
  UInt_t next_pps = 0;           // extracted from the last_pps of the second after next second.
  int    relative_delta = 0;     // (next_pps - this_pps) - 125E6; unsigned int overflow taken care of
  double avg_relative_delta = 0; // average delta (moving average)
  double corrected_pps = 0;      // corrected this_pps (correction via avg_relative_delta)
  bool   invalid = false;        // set to true if the second is a missing second (or neighbors one)
};

using TimeTable=std::map<Long64_t, event_second_start_end>;

// The "main" function. Returns the run number or error code
int analyze(TString header_file_path);

// Prepare a TimeTable (invalid rows not included). Returns the run number or error code
int prepare_table (TString& header_file_name, TimeTable& time_table, ROOT::RVecLL& invalid_seconds);

// Removes duplicate entries in `invalid_seconds` and sort ascending.
// Returns an error if there are too many consecutive invalid seconds.
int prune_and_check_invalid_seconds(ROOT::RVecLL& invalid_seconds, std::size_t TOLERANCE = 10);

// Computes `avg_relative_delta` for each second using neighboring seconds. 
// Returns error (code ERR_TimeTableTooShort) if the TimeTable is too short compared to `half_width`.
// @todo figure out a reasonable default `half_width`
int simple_moving_average(TimeTable& time_table, std::size_t half_width = 5);

// Finds the mid-point of a stable period;
// for every second in this period, delta ≈ avg_relative_delta.
// Returns iterator `anchor_point`, or time_table.begin() if a stable region couldn't be found.
TimeTable::iterator find_stable_region_mid_point(std::size_t len, TimeTable& t);

void insert_invalid_seconds_back(TimeTable& time_table, const ROOT::RVecLL& invalid_seconds);

// Calls `simple_moving_average()` to compute `avg_relative_delta` for each second,
// then extrapolates from `anchor_point` to compute the `corrected_pps` for every second using
// `avg_relative_delta` of each second.
void stupid_extrapolation(TimeTable& time_table, TimeTable::iterator anchor_point);

void print(TimeTable& time_table, std::ostream& stream = std::cout, std::size_t num_rows = -1); // -1: print all rows
void plot (TimeTable& time_table, TString name="pps_correction.svg");

#define ERR_EmptyTable -1
#define ERR_TooManyConsecutiveInvalid -2
#define ERR_TimeTableTooShort -3

int header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  fs::recursive_directory_iterator run_dir("/work/headers/");
  const std::regex pattern(R"(headFile(\d+))");
  std::smatch run_match;

  for(auto const& entry: run_dir)
  {
    if (!entry.is_regular_file()) continue;

    std::string name = entry.path().stem().string(); 
    // skip other root files in the run folder
    if (!std::regex_match(name, run_match, pattern)) continue;

    // auto r = std::atoi(run_match[1].str().c_str());
    // if (r == 1365 || r==1286 || r==1180 || r==1315 || r==1138 || r==1108 ||
    //     r==1029 || r==1020 || r==1222 || r==1345 || r==1122) 
    // {
    //   continue;
    // }
    std::cout << entry.path() << "\n";
    int actual_run = analyze(entry.path().c_str());
    int attempt_run = std::atoi(run_match[1].str().c_str());

    if (actual_run < 0)
    {
      std::cerr << "Error occurred during run " << attempt_run << " (code: " << actual_run << ")\n";
    }
    else if (actual_run >= 0 && actual_run != attempt_run)
    {
      fprintf(stderr, "run number mismatch (attempt: %d, result: %d)", attempt_run, actual_run);
    }

  }
  return 0;
}

int analyze(TString header_file_path)
{
  TimeTable time_table;
  ROOT::RVecLL invalid_seconds;
  int run = prepare_table(header_file_path, time_table, invalid_seconds);
  if (run == ERR_EmptyTable) return run;

  if (prune_and_check_invalid_seconds(invalid_seconds) == ERR_TooManyConsecutiveInvalid)
    return ERR_TooManyConsecutiveInvalid;

  if(simple_moving_average(time_table))
    return ERR_TimeTableTooShort;

  // std::size_t stable_period = time_table.size() / 5;
  // TimeTable::iterator mid_point = find_stable_region_mid_point(stable_period, time_table);
  //
  // insert_invalid_seconds_back(time_table, invalid_seconds);
  // stupid_extrapolation(time_table, mid_point);
  //
  // print(time_table);
  // fprintf(stdout, "Run %d duration: %lld seconds.\n", 
  //         run, std::prev(time_table.end())->first - time_table.begin()->first);
  // plot(time_table);

  return run;
}

int prepare_table(TString& header_file_name, TimeTable& time_table, ROOT::RVecLL& invalid_seconds)
{
  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture
  ROOT::RDataFrame tmp_header_rdf("headerTree", header_file_name);
  auto run = tmp_header_rdf.Take<Int_t>("run"); // will return the first element in this vector

  // start by filling a table of event_second vs lpps
  auto search_and_fill = 
    [&time_table]
    (UInt_t event_second, UInt_t lpps)
    {
      Long64_t evtsec = (Long64_t) event_second;
      bool new_encounter = time_table.find(evtsec) == time_table.end();
      if (new_encounter) 
      {
        time_table.emplace(evtsec, event_second_start_end{.last_pps=lpps});
      }
    };
  tmp_header_rdf.Foreach(search_and_fill, {"triggerTime","lastPPS"});

  if (time_table.size() < 3) 
  { // It only makes sense if we have at least one valid second,
    // and since the final two seconds of any run are invalid (can't compute their pps delta),
    // really we need at least 3 seconds in a run.
    print(time_table, std::cerr);
    fprintf(stderr, 
            "\033[1;31mFatal Error at %s.\n\tReason: time table too short (run %d).\n\033[0m",
            __PRETTY_FUNCTION__, run->front());
    return ERR_EmptyTable;
  }  

  if (time_table.size() < 20) // Mar 10 2026: I manually figured out the shortest run is 889 at 20 seconds
  {
    print(time_table, std::cerr);
    fprintf(stderr, "\033[1;33mWarning at %s.\n\tReason: small table (run %d).\n\033[0m",
            __PRETTY_FUNCTION__, run->front());
  }  

  // Next, compute this_pps, next_pps, and delta of each second, using last_pps.
  for (TimeTable::iterator current=time_table.begin(); current!=std::prev(time_table.end(),2);)
  {
    TimeTable::iterator next_next = std::next(current, 2);
    TimeTable::iterator next = std::next(current, 1);

    // ie, check if the next two seconds actually exist in the table
    if(next->first != current->first+1)
    {
      invalid_seconds.emplace_back(current->first);
      invalid_seconds.emplace_back(current->first+1);
      current = time_table.erase(current);
    }
    else if (next_next->first != current->first+2)
    {
      invalid_seconds.emplace_back(current->first);
      invalid_seconds.emplace_back(current->first+2);
      current = time_table.erase(current);
    }
    else // success case
    {
      // the last_pps of next second is the current second's this_pps
      UInt_t tpps = next->second.last_pps;
      current->second.this_pps = tpps;
      UInt_t npps = next_next->second.last_pps;
      current->second.next_pps =  npps;
      // Explicitly use UInt_t so that wrap-around subtraction is automatic
      UInt_t delta =  npps - tpps;
      // And then convert to int so that values below nominal show up as negative
      current->second.relative_delta = static_cast<int>(delta) - NOMINAL_CLOCK_FREQ;
      ++current;
    }  
  }
  // The final 2 seconds will never be valid
  invalid_seconds.emplace_back((--time_table.end())->first);
  time_table.erase(--time_table.end());
  invalid_seconds.emplace_back((--time_table.end())->first);
  time_table.erase(--time_table.end());

  return run->front();
}

int prune_and_check_invalid_seconds(ROOT::RVecLL& invalid_seconds, std::size_t TOLERANCE)
{
  std::sort(invalid_seconds.begin(), invalid_seconds.end());
  auto dup = std::unique(invalid_seconds.begin(), invalid_seconds.end());
  invalid_seconds.erase(dup, invalid_seconds.end());

  if (TOLERANCE < 2) // since the final two seconds are always invalid, TOLERANCE should be lenient enough
  {
    fprintf(stderr, "%s\n\tBad TOLERANCE: %lu, resetting to 10\n", __PRETTY_FUNCTION__, TOLERANCE);
    TOLERANCE = 10;
  }
  if (invalid_seconds.size() <= TOLERANCE) return 0;

  std::size_t invalid_chunk_size = 1;
  for (std::size_t i=0; i+1<invalid_seconds.size(); ++i)
  {
    if (invalid_seconds[i+1] != invalid_seconds[i]+1) invalid_chunk_size = 1;
    else 
    {
      invalid_chunk_size++;
      if (invalid_chunk_size > TOLERANCE) return ERR_TooManyConsecutiveInvalid;
    }
  }
  return 0;
}

int simple_moving_average(TimeTable& time_table, std::size_t half_width)
{
  if (half_width == 0) 
  {
    fprintf(stderr,
      "Error at %s:\n\t bad value supplied to parameter `half_width` (%lu), resetting it to 5 [seconds].\n",
      __PRETTY_FUNCTION__ , half_width
    );
    half_width=5;
  }

  if (2 * half_width + 1 > time_table.size())
  {
    fprintf(stderr,
      "\033[1;31mFatal Error at: %s\n\tReason: time_table too short (only %lu valid rows).\033[0m\n",
      __PRETTY_FUNCTION__ , time_table.size()
    );
    print(time_table, std::cerr);
    return ERR_TimeTableTooShort;
  }

  TimeTable::iterator start = std::next(time_table.begin(), half_width);
  TimeTable::iterator stop =  std::prev(time_table.end(), half_width);

  // first, perform moving average for the "meat" of the table (ie exclude first/last few rows)
  for(TimeTable::iterator it = start; it!=stop; ++it)
  {
    double sum = 0.;
    // it stands for iterator, so obviously jt is jiterator ¯\_(ツ)_/¯
    for (auto jt=std::prev(it,half_width); jt!=std::next(it,half_width+1); ++jt){
      sum += jt->second.relative_delta;
    }
    it->second.avg_relative_delta = sum / ( 2. * half_width + 1.);
  }

  // As for the first/last few rows in the table,
  // I could probably do something more sophisticated, but nah let's just extrapolate
  for (auto it=time_table.begin(); it!=start; ++it){
    it->second.avg_relative_delta = start->second.avg_relative_delta;
  }
  for (auto it=stop; it!=time_table.end(); ++it){
    it->second.avg_relative_delta = std::prev(stop)->second.avg_relative_delta;
  }

  return 0;
};

template<typename T> bool approx_equal(T a, T b, T tolerance = 20)
{
  T diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

TimeTable::iterator find_stable_region_mid_point(std::size_t requested_stable_period, TimeTable& time_table) 
{
  if (requested_stable_period > time_table.size())
  {
    std::cerr << __PRETTY_FUNCTION__ << ": requestd size too large. "
    "Resetting `requested_stable_period` to 10.\n";
    requested_stable_period = 10;
  }

  TimeTable::iterator mid_point = time_table.begin();
  TimeTable::iterator stop = time_table.end();

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
    print(time_table, std::cerr);
    fprintf
    (
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `next_pps` - `this_pps` are all approximately 125 million clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      requested_stable_period, time_table.begin()->first
    );
  } else {
    mid_point = time_table.find(stable_seconds.at(requested_stable_period/2));
  }

  if (mid_point == time_table.end()) // check again, in case the find() above returns an invalid result
  {
    fprintf
    (
      stderr,
      "\e[31;1mCouldn't find a stable region (required: %lu stable seconds) "
      "where `next_pps` - `this_pps` are all approximately 125 million clock counts.\n"
      "Falling back to the assumption that the first second (%llu) is a \"good second\"\n\e[31;0m",
      requested_stable_period, time_table.begin()->first
    );
    mid_point = time_table.begin();
  }

  return mid_point;
}

void insert_invalid_seconds_back(TimeTable& time_table, const ROOT::RVecLL& invalid_seconds)
{
  for(auto& sec: invalid_seconds)
  {
    time_table[sec].invalid = true;
    TimeTable::iterator past   = time_table.find(sec-1);
    TimeTable::iterator future = time_table.find(sec+1);
    if (past != time_table.end())
    {
      time_table[sec].avg_relative_delta = past->second.avg_relative_delta;
    } 
    else if (future != time_table.end())
    {
      time_table[sec].avg_relative_delta = future->second.avg_relative_delta;
    } 
  }
}

void stupid_extrapolation(TimeTable& time_table, TimeTable::iterator anchor_point)
{
  anchor_point->second.corrected_pps = anchor_point->second.this_pps;

  // backward extrapolation from the anchor point
  for (auto rit = std::make_reverse_iterator(anchor_point); rit!=time_table.rend(); ++rit)
  {
    auto future = std::prev(rit);
    rit->second.corrected_pps = 
      std::fmod(
        future->second.corrected_pps - (NOMINAL_CLOCK_FREQ + rit->second.avg_relative_delta) +
        static_cast<double>(UINT32_MAX) + 1,
        static_cast<double>(UINT32_MAX) + 1
      );
  }

  // forward extrapolation
  for (auto it = std::next(anchor_point); it!=time_table.end(); ++it)
  {
    auto past = std::prev(it);
    it->second.corrected_pps = 
      std::fmod(
        past->second.corrected_pps + (NOMINAL_CLOCK_FREQ + past->second.avg_relative_delta)+
        static_cast<double>(UINT32_MAX) + 1,
        static_cast<double>(UINT32_MAX) + 1
      );
  }
}

void print(TimeTable& time_table, std::ostream& stream, std::size_t num_rows)
{
  TimeTable::iterator stop = (num_rows < time_table.size()) ? std::next(time_table.begin(), num_rows) : time_table.end();

  stream << "\nRelative delta is defined to be (next_pps - this_pps) - 125000000\n\n";

  stream << "---------------------------------------------------------------------------------------\n"
         << " seconds     | last pps  | this_pps  | next_pps  | relative | avg. rel. | corrected  \n"
         << " since epoch |           |           |           | delta    | delta     | this_pps   \n"
         << " Long64_t    | UInt_t    | UInt_t    | UInt_t    | int      | double    | double     \n"
         << "---------------------------------------------------------------------------------------\n";

  TString color;
  for (auto it=time_table.begin(); it!=stop; ++it)
  {
    auto color = it->second.invalid ? "\033[1;31m " : "\033[0m ";

    stream << color << std::setw(13) << std::left << it->first
           << color << std::setw(11) << it->second.last_pps
           << color << std::setw(11) << it->second.this_pps
           << color << std::setw(11) << it->second.next_pps
           << color << std::setw(10)  << it->second.relative_delta
           << color << std::setw(9)  << std::fixed << std::setprecision(2) << it->second.avg_relative_delta
           << color << std::setw(15) << std::right << std::fixed << std::setprecision(2) << it->second.corrected_pps << "\033[0m\n";
  }

  if (num_rows < time_table.size()){
    stream << " ...\n";
    stream << " ...\n";
    stream << " ...\n";
  }
  stream << "\033[0m---------------------------------------------------------------------------------------\n";
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

    Long64_t o = e.second.this_pps;
    original.SetPoint(counter, e.first-t0,o);
    double c = e.second.corrected_pps;
    corrected.SetPoint(counter, e.first-t0, c);
    double d = o-c;
    diff.SetPoint(counter, e.first-t0, d);

    original_delta.SetPoint(counter, e.first-t0, e.second.relative_delta);
    avg_delta.SetPoint(counter, e.first-t0, e.second.avg_relative_delta);

    counter++;
  }

  original.RemovePoint(original.GetN()-1); // final second doesn't have a valid this_pps
  diff.RemovePoint(diff.GetN()-1);         // final second doesn't have a valid this_pps
  original_delta.RemovePoint(original_delta.GetN()-1); // final two seconds don't have valid deltas
  original_delta.RemovePoint(original_delta.GetN()-1); // final two seconds don't have valid deltas

  TCanvas c1(name, name, 1920 * 1.5, 1080 * 2);
  c1.Divide(1,3);
  c1.cd(1);
  original.Draw("ALP");
  original.SetMarkerStyle(kFullCrossX);
  original.SetMarkerSize(3);
  original.SetTitle("System Clock Value (uint32_t) at Start of Each Second (aka \"this_pps\")");
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
  original_delta.SetTitle("#Delta #equiv next_pps - this_pps ≈ 125,000,000");
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
  diff.SetTitle("(original this_pps) - (corrected_pps)");
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
