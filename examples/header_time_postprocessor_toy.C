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
  bool   missing = false;        // set to true if the second is a missing second
  bool   invalid_delta = false;  // set to true if relative_delta cannot be computed
};

using TimeTable=std::map<Long64_t, event_second_start_end>;

// The "main" function. Returns the run number or error code
int analyze(const TString header_file_path);

// Prepares two printable TimeTables, see #print().
// @param[out] run Run number
// @retval Success (0), Error (ERR_TimeTableTooShort), or Warning (ERR_MissingSecond)
int prepare_table(const TString & header_file_name, int * run, TimeTable * time_table, TimeTable * invalid_seconds);

// Computes `avg_relative_delta` for each second using neighboring seconds. 
// @retval Success (0) or error (ERR_TimeTableTooShort) if the TimeTable is too short compared to `half_width`.
// @todo figure out a reasonable default `half_width`
int simple_moving_average(TimeTable* time_table, std::size_t half_width = 8);

// Finds the mid-point of a stable period; for every second in this period, delta ≈ avg_relative_delta.
// @param[out] `anchor_point` (which is time_table.begin() if a stable region couldn't be found).
// @retval Success (0), warning (ERR_NoStableRegion), or error (ERR_EmptyTable).
int find_stable_region_mid_point(TimeTable* time_table, TimeTable::iterator* anchor_point);

// Inserts invalid seconds back to time_table.
// The pps delta of an invalid second is simply identical to its valid neighbors.
void insert_invalid_seconds_back(TimeTable* time_table, TimeTable* invalid_seconds);

// Extrapolates from `anchor_point` to compute the `corrected_pps` for every second using `avg_relative_delta`.
// Relies on `simple_moving_average()` to have computed `avg_relative_delta`.
void stupid_extrapolation(TimeTable* time_table, const TimeTable::iterator& anchor_point);

void print(const TimeTable* time_table, std::ostream& stream = std::cout);
void plot (TimeTable& time_table, TString name="pps_correction.svg");

enum err_code
{
  ERR_TimeTableTooShort = 1<<0,
  ERR_MissingSecond     = 1<<1, // recoverable
  ERR_LargeDelta        = 1<<2, // recoverable
  ERR_EmptyTable        = 1<<3,
  ERR_NoStableRegion    = 1<<4 // recoverable
};

int header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  // int run=1311;
  // analyze(Form("/work/headers/run%d/headFile%d.root", run, run));

  fs::recursive_directory_iterator run_dir("/work/headers/");
  const std::regex pattern(R"(headFile(\d+))");
  std::smatch run_match;
  for(auto const& entry: run_dir)
  {
    if (!entry.is_regular_file()) continue;

    std::string name = entry.path().stem().string(); 
    // skip other root files in the run folder, as well as any runs before 782 (1st run with amp on)
    if (!std::regex_match(name, run_match, pattern)) continue;
    else if (std::atoi(run_match[1].str().c_str()) < 782) continue;

    std::cout << entry.path() << "\n";
    analyze(entry.path().c_str());
  }

  return 0;
}

int analyze(const TString header_file_path)
{
  TimeTable time_table;
  TimeTable invalid_seconds;
  int run;

  int err = prepare_table(header_file_path, &run, &time_table, &invalid_seconds);
  if (err&ERR_TimeTableTooShort)
  {
    print(&time_table, std::cerr);
    fprintf(stderr, "\e[1;31mFatal Error: time table too short (run %d).\n\n\n\e[0m", run);
    return ERR_TimeTableTooShort;
  }
  if (err & ERR_MissingSecond)
    fprintf(stderr, "\e[1;33mWarning: missing seconds inserted (run %d).\n\n\n\e[0m", run);
  if (err & ERR_LargeDelta)
    fprintf(stderr, "\e[1;33mWarning: large pps delta detected (run %d).\n\n\n\e[0m", run);

  if(simple_moving_average(&time_table))
  {
    print(&time_table, std::cerr);
    fprintf(stderr, "\e[1;31mFatal Error: can't compute average delta; "
            "time table too short (run %d).\n\n\n\e[0m", run);
    return ERR_TimeTableTooShort;
  }

  TimeTable::iterator anchor_point;

  switch(find_stable_region_mid_point(&time_table, &anchor_point))
  {
    case ERR_EmptyTable: 
      {
        // Redudant check. Ideally we should have errored out long before we reach this function call.
        print(&time_table, std::cerr);
        fprintf(stderr, "\e[1;31mFatal Error: empty table (run %d).\n\n\n\e[0m", run);
        return ERR_EmptyTable;
      }
    case ERR_NoStableRegion:
      {
        print(&time_table, std::cerr);
        fprintf(
          stderr,
          "\e[31;1mWarning: couldn't find a stable region where `next_pps` - `this_pps` ≈ 125 million.\n"
          "Will use the first second as the anchor point (run %d).\n\n\n\e[0m", run
        );
        break;
      }
    default:
      break;
  }

  insert_invalid_seconds_back(&time_table, &invalid_seconds);
  stupid_extrapolation(&time_table, anchor_point);
  // print(&time_table, std::cerr);
  // plot(time_table);

  return 0;
}

int prepare_table(const TString& header_file_name, int* run, TimeTable* time_table, TimeTable* invalid_seconds)
{
  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture
  ROOT::RDataFrame tmp_header_rdf("headerTree", header_file_name);
  auto rvec = tmp_header_rdf.Take<Int_t>("run"); // will return the first element in this vector

  // start by filling a table of event_second vs lpps
  auto search_and_fill = 
    [&time_table]
    (UInt_t event_second, UInt_t lpps)
    {
      Long64_t evtsec = (Long64_t) event_second;
      bool new_encounter = time_table->find(evtsec) == time_table->end();
      if (new_encounter) 
      {
        time_table->emplace(evtsec, event_second_start_end{.last_pps=lpps});
      }
    };
  tmp_header_rdf.Foreach(search_and_fill, {"triggerTime","lastPPS"});
  *run = rvec->front();

  // It only makes sense if we have at least one valid second (ie 3 consecutive `event_second`s).
  // Note: the final two seconds of any run are invalid (can't compute their pps delta),
  if (time_table->size() < 3) return ERR_TimeTableTooShort;

  // Mar 10 2026: I manually figured out the shortest run is 889 at 20 seconds, 
  // so this warning would actually never be triggered :)
  if (time_table->size() < 20)
  {                        
    print(time_table, std::cerr);
    fprintf(stderr, "\033[1;33mWarning: small table (run %d).\n\033[0m", *run);
  }
  
  int warning_code = 0;

  // loop from first second to last second, if a second is missing, add it as invalid
  for (TimeTable::iterator current=time_table->begin(); current!=std::prev(time_table->end(),2);++current)
  {
    TimeTable::iterator next_next = std::next(current, 2);
    TimeTable::iterator next = std::next(current, 1);
    Long64_t actual_next_second = current->first+1;
    Long64_t actual_next_next = current->first+2;
    // if the next (or next next) second does not exist, then the current second's delta pps cannot be computed
    if (actual_next_second != next->first)
    {
      (*time_table)[actual_next_second].missing = true;
      current->second.invalid_delta = true;
      warning_code |= ERR_MissingSecond;
    }
    if (actual_next_next != next_next->first)
    {
      (*time_table)[actual_next_next].missing = true;
      current->second.invalid_delta = true;
      (*time_table)[actual_next_second].invalid_delta = true;
      warning_code |= ERR_MissingSecond;
    }
  }

  // Next, compute this_pps, next_pps, and delta := (next_pps - this_pps) of each valid second;
  // REMOVE entries with invalid deltas, since we're gonna perform an average later.
  for (TimeTable::iterator current=time_table->begin(); current!=std::prev(time_table->end(),2);)
  {
    TimeTable::iterator next_next = std::next(current, 2);
    TimeTable::iterator next = std::next(current, 1);
    if (next->second.missing || next_next->second.missing)
    {
      // move entries with invalid deltas to a separate table
      invalid_seconds->insert(time_table->extract(current++));
    }
    else
    {
      // the last_pps of next second is the current second's this_pps
      UInt_t tpps = next->second.last_pps;
      current->second.this_pps = tpps;
      UInt_t npps = next_next->second.last_pps;
      current->second.next_pps =  npps;
      // Explicitly use UInt_t so that wrap-around subtraction is automatic
      UInt_t delta =  npps - tpps;
      // And then convert to int so that values below nominal show up as negative
      int rel_delta = static_cast<int>(delta) - NOMINAL_CLOCK_FREQ;
      current->second.relative_delta = rel_delta;

      if (TMath::Abs(rel_delta) > 100) warning_code |= ERR_LargeDelta;
      ++current;
    }  
  }
  // The final 2 seconds will never have valid deltas since the next two seconds don't exist for them.
  for(TimeTable::iterator it=std::prev(time_table->end(),2); it!=time_table->end();)
  {
    it->second.invalid_delta=true;
    invalid_seconds->insert(time_table->extract(it++));
  }


  return warning_code;
}

int simple_moving_average(TimeTable* time_table, std::size_t half_width)
{
  if (half_width == 0) half_width=5;

  if (2 * half_width + 1 > time_table->size()) return ERR_TimeTableTooShort;

  TimeTable::iterator start = std::next(time_table->begin(), half_width);
  TimeTable::iterator stop =  std::prev(time_table->end(), half_width);

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
  for (auto it=time_table->begin(); it!=start; ++it){
    it->second.avg_relative_delta = start->second.avg_relative_delta;
  }
  for (auto it=stop; it!=time_table->end(); ++it){
    it->second.avg_relative_delta = std::prev(stop)->second.avg_relative_delta;
  }

  return 0;
};

template<typename T> bool approx_equal(T a, T b, T tolerance = 20)
{
  T diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

int find_stable_region_mid_point(TimeTable * time_table, TimeTable::iterator* anchor_point) 
{
  if (time_table->empty()) return ERR_EmptyTable;

  // some arbitrarily decided values for the length of this "stable period".
  std::size_t stable_length = time_table->size() < 50 ? 10 : time_table->size() / 5;

  *anchor_point = time_table->begin();
  TimeTable::iterator stop = time_table->end();

  std::vector<TimeTable::iterator> stable_seconds;
  stable_seconds.reserve(stable_length);

  for (auto it=time_table->begin(); it!=stop; ++it) 
  {
    if (stable_seconds.size() == stable_length) break; 

    if (approx_equal<double>(it->second.relative_delta , it->second.avg_relative_delta)) 
      stable_seconds.emplace_back(it);
    else stable_seconds.clear();
  }

  if (stable_seconds.size() != stable_length) return ERR_NoStableRegion;
  else 
  {
    *anchor_point = stable_seconds.at(stable_length/2);
    return 0;
  }
}

void insert_invalid_seconds_back(TimeTable* time_table, TimeTable* invalid_seconds)
{
  for(TimeTable::iterator it=invalid_seconds->begin(); it!=invalid_seconds->end();)
  {
    auto insert = time_table->insert(invalid_seconds->extract(it++));
    TimeTable::iterator current = insert.position; // position of the newly inserted element in time_table
    TimeTable::iterator past = std::prev(current);
    TimeTable::iterator future = std::next(current);
    if (future != time_table->end())
    {
      double avg = (past->second.avg_relative_delta + future->second.avg_relative_delta) / 2;
      current->second.avg_relative_delta = avg;
    } else {
      current->second.avg_relative_delta = past->second.avg_relative_delta;
    }
  }
}

void stupid_extrapolation(TimeTable* time_table, const TimeTable::iterator & anchor_point)
{
  anchor_point->second.corrected_pps = anchor_point->second.this_pps;

  // backward extrapolation from the anchor point
  for (auto rit = std::make_reverse_iterator(anchor_point); rit!=time_table->rend(); ++rit)
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
  for (auto it = std::next(anchor_point); it!=time_table->end(); ++it)
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

void print(const TimeTable* time_table, std::ostream& stream)
{

  stream << "\nColor: \e[1;34mMissing \e[1;33m Invalid Delta \e[1;31m Missing and Invalid Delta\e[0m\n";
  stream << "Relative delta is defined to be (next_pps - this_pps) - 125000000\n";
  stream << "---------------------------------------------------------------------------------------\n"
         << " seconds     | last pps  | this_pps  | next_pps  | relative | avg. rel. | corrected  \n"
         << " since epoch |           |           |           | delta    | delta     | this_pps   \n"
         << " Long64_t    | UInt_t    | UInt_t    | UInt_t    | int      | double    | double     \n"
         << "---------------------------------------------------------------------------------------\n";

  TString color;
  for (auto it=time_table->begin(); it!=time_table->end(); ++it)
  {
    if (it->second.missing && it->second.invalid_delta) color = "\033[1;31m ";  // red
    else if (it->second.missing) color = "\033[1;34m ";  // blue
    else if (it->second.invalid_delta) color = "\033[1;33m "; // yellow
    else color = "\033[0m ";

    stream << color << std::setw(13) << std::left << it->first
           << color << std::setw(11) << it->second.last_pps// << "\033[0m\n";
           << color << std::setw(11) << it->second.this_pps
           << color << std::setw(11) << it->second.next_pps
           << color << std::setw(10) << it->second.relative_delta
           // << color << std::setw(10) << std::boolalpha << it->second.missing
           // << color << std::setw(10) << std::boolalpha << it->second.invalid_delta <<  "\033[0m\n";
           << color << std::setw(9)  << std::fixed << std::setprecision(2) << it->second.avg_relative_delta
           << color << std::setw(15) << std::right << std::fixed << std::setprecision(2) << it->second.corrected_pps << "\n";
  }

  stream << "\033[0m---------------------------------------------------------------------------------------\n";
}

void plot(TimeTable& time_table, TString name)
{
  Long64_t t0 = time_table.begin()->first;
  int xlow = 0;
  int xhigh = 0;

  TGraph original_this_pps(time_table.size());
  TGraph corrected_this_pps(time_table.size());
  TGraph this_pps_residual(time_table.size());

  TGraph original_delta(time_table.size());
  TGraph avg_delta(time_table.size());

  std::size_t counter=0;
  for (auto& row: time_table){

    Long64_t original = row.second.this_pps;
    original_this_pps.SetPoint(counter, row.first-t0, original);

    double corrected = row.second.corrected_pps;
    corrected_this_pps.SetPoint(counter, row.first-t0, corrected);

    double residual = original-corrected;
    this_pps_residual.SetPoint(counter, row.first-t0, residual);

    original_delta.SetPoint(counter, row.first-t0, row.second.relative_delta);
    avg_delta.SetPoint(counter, row.first-t0, row.second.avg_relative_delta);

    counter++;
  }

  original_this_pps.RemovePoint(original_this_pps.GetN()-1); // last second doesn't have a valid this_pps
  original_this_pps.RemovePoint(original_this_pps.GetN()-1); // 2nd-to-last second doesn't have a valid this_pps
  this_pps_residual.RemovePoint(this_pps_residual.GetN()-1); // for comparison to make sense,
  this_pps_residual.RemovePoint(this_pps_residual.GetN()-1);

  original_delta.RemovePoint(original_delta.GetN()-1); // final two seconds don't have valid deltas
  original_delta.RemovePoint(original_delta.GetN()-1);

  TCanvas c1(name, name, 1920 * 1.5, 1080 * 2);
  c1.Divide(1,3);
  c1.cd(1);
  original_this_pps.Draw("ALP");
  original_this_pps.SetMarkerStyle(kFullCrossX);
  original_this_pps.SetMarkerSize(3);
  original_this_pps.SetTitle("System Clock Value (uint32_t) at Start of Each Second (aka \"this_pps\")");
  original_this_pps.GetXaxis()->SetLabelSize(0);
  original_this_pps.GetYaxis()->SetTitle("[sysclk counts]");
  original_this_pps.GetYaxis()->SetTitleSize(0.1);
  original_this_pps.GetYaxis()->SetTitleOffset(0.3);
  original_this_pps.GetYaxis()->CenterTitle();
  if (xlow && xhigh)
    original_this_pps.GetXaxis()->SetRangeUser(xlow, xhigh);
  corrected_this_pps.Draw("P");
  corrected_this_pps.SetMarkerStyle(kCircle);
  corrected_this_pps.SetMarkerColor(kRed);
  corrected_this_pps.SetMarkerSize(2);

  double y0 = UINT32_MAX;
  TLine line(original_this_pps.GetPointX(0), y0, original_this_pps.GetPointX(original_this_pps.GetN()-1), y0);
  line.SetLineStyle(2);   // dashed
  line.Draw();

  TLegend leg(0.6, 0, 0.9, 0.1); // (x1,y1,x2,y2) in NDC
  leg.AddEntry(&original_this_pps, "Original", "p");
  leg.AddEntry(&corrected_this_pps, "Corrected", "p");
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
  if (xlow && xhigh)
    original_delta.GetXaxis()->SetRangeUser(xlow, xhigh);
  // original_delta.GetYaxis()->SetRangeUser(30, 40);

  avg_delta.Draw("P");
  avg_delta.SetMarkerStyle(kCircle);
  avg_delta.SetMarkerColor(kRed);

  TLegend leg3(0.6, 0.0, 0.9, 0.1); // (x1,y1,x2,y2) in NDC
  leg3.AddEntry(&original_delta, "original (discrete) delta (uint32_t)", "p");
  leg3.AddEntry(&avg_delta, "smoothed (moving averaged) delta (double_t)", "p");
  leg3.Draw();

  c1.cd(3);
  this_pps_residual.Draw("ALP");
  this_pps_residual.SetTitle("(original this_pps) - (corrected_pps)");
  this_pps_residual.SetMarkerStyle(kCircle);
  this_pps_residual.GetYaxis()->SetTitle("[sysclk counts]");
  this_pps_residual.GetYaxis()->CenterTitle();
  this_pps_residual.GetYaxis()->SetTitleSize(0.1);
  this_pps_residual.GetYaxis()->SetTitleOffset(0.3);
  this_pps_residual.GetXaxis()->SetTitle(Form("Event Second [seconds since t0 (%lld)]", t0));
  this_pps_residual.GetXaxis()->SetTitleOffset(2.2);
  this_pps_residual.GetXaxis()->CenterTitle();
  this_pps_residual.GetXaxis()->SetLabelOffset(0.05);
  if (xlow && xhigh)
    this_pps_residual.GetXaxis()->SetRangeUser(xlow, xhigh);
  this_pps_residual.GetYaxis()->SetRangeUser(-10, 10);

  c1.SaveAs(name);
}
