#include "pueo/RawHeader.h"
#include "pueo/Timemark.h"
#include "ROOT/RDataFrame.hxx"
#include "TTimeStamp.h"
#include "TAttMarker.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TLegend.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <map>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

constexpr int32_t NOMINAL_CLOCK_FREQ=125000000;
constexpr int32_t PUEO_FIRST_AMP_RUN=783; // will ignore all runs prior to this one
constexpr int32_t PUEO_LAUNCH_SECOND=1766163240;

// Nominally, delta := (next_pps - this_pps) should yield approximately 125E6 
// (unsigned integer overflow should be taken care of when computing this difference).
// `relative_delta` is then defined as (delta - 125E6).
struct event_second_start_end 
{
  int32_t  corrected_sec = 0;      // Corrected event_second
  int32_t  readout_time_sec = 0;   // CPU readout time, not necessarily the same as event_second
  uint32_t this_pps = 0;           // value of the sysclk counter of the current second
  uint32_t next_pps = 0;           // value of the sysclk counter of the next second
  int32_t  relative_delta = 0;     // (next_pps - this_pps) - 125E6; unsigned int overflow taken care of
  double   avg_relative_delta = 0; // average delta (moving average)
  double   corrected_pps = 0;      // corrected this_pps (correction via avg_relative_delta)
  bool     missing = false;        // set to true if the second is a missing second
  bool     invalid_delta = false;  // set to true if relative_delta cannot be computed
};

using TimeTable=std::map<int32_t, event_second_start_end>;

// The "main" function. Returns the run number or error code
int32_t analyze(const char * header_file_path);

// Prepares two printable TimeTables, see #print().
// @param[out] run Run number
// @retval Success (0), Error (eg. ERR_TimeTableTooShort), or Warning (eg. ERR_MissingSecond)
int32_t prepare_table(ROOT::RDF::RNode header_rdf, uint32_t * run, TimeTable * time_table, TimeTable * invalid_seconds);

// Checks if the first column of the table (`event_second`) starts out wrong.
// Rumor has it that some runs started erroneously from year 1970 (Unix epoch).
int32_t check_event_seconds(const TimeTable * time_table);

// Computes `avg_relative_delta` for each second using neighboring seconds. 
// @retval Success (0) or error (ERR_TimeTableTooShort) if the TimeTable is too short compared to `half_width`.
// @todo figure out a reasonable default `half_width`
int32_t simple_moving_average(TimeTable* time_table, std::size_t half_width = 8);

// Finds the mid-point of a stable period; for every second in this period, delta ≈ avg_relative_delta.
// @param[out] `anchor_point` (which is time_table.begin() if a stable region couldn't be found).
// @retval Success (0), warning (ERR_NoStableRegion), or error (ERR_EmptyTable).
int32_t find_stable_region_mid_point(TimeTable* time_table, TimeTable::iterator* anchor_point);

// Inserts invalid seconds back to time_table.
// The pps delta of an invalid second is simply identical to its valid neighbors.
void insert_invalid_seconds_back(TimeTable* time_table, TimeTable* invalid_seconds);

// Extrapolates from `anchor_point` to compute the `corrected_pps` for every second using `avg_relative_delta`.
// Relies on `simple_moving_average()` to have computed `avg_relative_delta`.
void stupid_extrapolation(TimeTable* time_table, const TimeTable::iterator& anchor_point);

void print(const TimeTable* time_table, std::ostream& stream = std::cout);
void plot (TimeTable& time_table, TString name="pps_correction.svg");

int32_t correct_one_event_second(TimeTable* pps_corrected_time_table, TimeTable::iterator* row,
                                 ROOT::RDF::RNode header_rdf, ROOT::RDF::RNode timemark_rdf);

// second_correction HAS TO BE LATER than first_correction
int32_t correct_all_event_seconds(TimeTable* time_table_twice_corrected, TimeTable::iterator first_correction,
                                  TimeTable::iterator second_correction)
{

  // the difference between event_second (the key of the map)
  auto key_diff = second_correction->first - first_correction->first;
  if (key_diff <= 0) return -1;
  // the if above guarantees second comes later than first
  // second_correction HAS TO BE later than first_correction otherwise this will be UB
  auto distance = std::distance(first_correction, second_correction);

  auto correction_diff = second_correction->second.corrected_sec - first_correction->second.corrected_sec;
  if (correction_diff!=key_diff) return -1;
  if (correction_diff!=distance) return -1;

  for (auto it=std::make_reverse_iterator(first_correction); it!=(*time_table_twice_corrected).rend(); ++it)
  {
    it->second.corrected_sec = first_correction->second.corrected_sec - (first_correction->first - it->first);
  }
  for (auto it=std::next(first_correction); it!=second_correction; ++it)
  {
    it->second.corrected_sec = first_correction->second.corrected_sec + (it->first - first_correction->first);
  }
  for (auto it=std::next(second_correction); it!=(*time_table_twice_corrected).end(); ++it)
  {
    it->second.corrected_sec = second_correction->second.corrected_sec + (it->first - second_correction->first);
  }

  return 0;
}

enum err_code
{
  ERR_MoreThanOneRun    = 1<<0, // fatal, more than one run present in headerFile<run>.root
  ERR_PreAmpRun         = 1<<1, // fatal, because we are just going to ignore these
  ERR_Year1970          = 1<<2, // should be recoverable
  ERR_TimeTableTooShort = 1<<3, // fatal, time tables with only 1 or 2 seconds can't be processed
  ERR_MissingSecond     = 1<<4, // recoverable
  ERR_LargeDelta        = 1<<5, // recoverable but won't happen, because they happen in pre-amp runs
  ERR_EmptyTable        = 1<<6, // fatal but probably won't be reached; we should have errored out already
  ERR_NoStableRegion    = 1<<7, // recoverable
  ERR_NoNearbyTimemark  = 1<<8  // cannot find a timestamped event using the event's readout time, should be recoverable
};

int32_t header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");
  int32_t run=1023;
  analyze(Form("/work/headers/run%d/headFile%d.root", run, run));

  // fs::recursive_directory_iterator run_dir("/work/headers/");
  // const std::regex pattern(R"(headFile(\d+))");
  // for(auto const& entry: run_dir)
  // {
  //   if (!entry.is_regular_file()) continue;
  //
  //   std::string name = entry.path().stem().string(); 
  //   // skip other root files in the run folder
  //   if (!std::regex_match(name, pattern)) continue;
  //
  //   std::cout << entry.path() << "\n";
  //   analyze(entry.path().c_str());
  // }
  //
  return 0;
}

int32_t analyze(const char * header_file_path)
{
  ROOT::RDataFrame header_rdf("headerTree", header_file_path);
  TimeTable time_table;
  TimeTable invalid_seconds;
  uint32_t run=0;

  int32_t err = prepare_table(header_rdf, &run, &time_table, &invalid_seconds);
  if (err&ERR_MoreThanOneRun) 
  {
    fprintf(stderr, "\e[1;31mFatal Error: cannot process %s; I can only handle a single run"
            " at a time but more than one is found.\n\e[0m", header_file_path);
    return ERR_MoreThanOneRun;
  }

  if (err&ERR_PreAmpRun ) 
  {
    fprintf(stderr, "\e[1;32mNote: run %d is a pre-amp run and will be ignored.\n\e[0m", run);
    return ERR_PreAmpRun ;
  }

  if (err&ERR_TimeTableTooShort)
  {
    print(&time_table, std::cerr);
    fprintf(stderr, "\e[1;31mFatal Error: time table too short (run %d).\n\e[0m", run);
    return ERR_TimeTableTooShort;
  }

  if (err & ERR_MissingSecond)
    fprintf(stderr, "\e[1;33mWarning: missing seconds inserted (run %d).\n\e[0m", run);

  if (err & ERR_LargeDelta)
    fprintf(stderr, "\e[1;33mWarning: large pps delta detected (run %d).\n\e[0m", run);

  if (check_event_seconds(&time_table))
    fprintf(stderr, "\e[1;31mWarning: problematic event_second (run %d).\n\e[0m", run);

  if(simple_moving_average(&time_table))
  {
    print(&time_table, std::cerr);
    fprintf(stderr, "\e[1;31mFatal Error: can't compute average delta; "
            "time table too short (run %d).\n\e[0m", run);
    return ERR_TimeTableTooShort;
  }

  TimeTable::iterator anchor_point;

  switch(find_stable_region_mid_point(&time_table, &anchor_point))
  {
    case ERR_EmptyTable: 
      {
        // Redudant check. Ideally we should have errored out long before we reach this function call.
        print(&time_table, std::cerr);
        fprintf(stderr, "\e[1;31mFatal Error: empty table (run %d).\n\e[0m", run);
        return ERR_EmptyTable;
      }
    case ERR_NoStableRegion:
      {
        print(&time_table, std::cerr);
        fprintf(
          stderr,
          "\e[31;1mWarning: couldn't find a stable region where `next_pps` - `this_pps` ≈ 125 million.\n"
          "Will use the first second as the anchor point (run %d).\n\e[0m", run
        );
        break;
      }
    default:
      break;
  }

  insert_invalid_seconds_back(&time_table, &invalid_seconds);
  stupid_extrapolation(&time_table, anchor_point);
  // plot(time_table);

  ROOT::RDataFrame timemark_rdf("timemarkTree", "/work/all_timemarks.root");

  // Pick a somewhat arbitray reference readout time, say,
  // 10 seconds into the run so that the readout time is more or less stable and contiguous.
  // (not sure if it matters whether the readout is stable or not, probably doesn't hurt though)
  TimeTable::iterator some_row = std::next(time_table.begin(), 10);
  int32_t evt_corr_err = correct_one_event_second(&time_table, &some_row, header_rdf, timemark_rdf);
  if (evt_corr_err&ERR_NoNearbyTimemark) 
  {
    fprintf(stderr, "\e[1;33mWarning: can't correct `event_second` %d because I couldn't find a "
            "suitable timestamped event to work with (run %d).\n\e[0m", some_row->first, run);
    return ERR_NoNearbyTimemark;
  }

  TimeTable::iterator another_row = std::next(time_table.begin(), 15);
  correct_one_event_second(&time_table, &another_row, header_rdf, timemark_rdf);

  correct_all_event_seconds(&time_table, some_row, another_row);

  print(&time_table, std::cerr);

  return 0;
}

int32_t prepare_table(ROOT::RDF::RNode header_rdf, uint32_t* run, TimeTable* time_table, TimeTable* invalid_seconds)
{
  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture
  std::vector<uint32_t> run_numbers;
  run_numbers.reserve(2000);

  // start by filling a table of event_second vs lpps
  auto search_and_fill = 
    [&time_table, &run_numbers]
    (pueo::RawHeader& rhdr)
    {
      bool new_encounter = time_table->find((time_t)rhdr.triggerTime) == time_table->end();
      if (new_encounter) 
      {
        run_numbers.push_back(static_cast<uint32_t>(rhdr.run));
        // If you think about it,
        // the last_pps of any event is actually the "this_pps" if we're sitting exactly on `event_second`.
        time_table->emplace
          (
            rhdr.triggerTime, 
            event_second_start_end
              {
                .readout_time_sec=(int32_t)rhdr.readoutTime,
                .this_pps=rhdr.lastPPS
              }
          );
      }
    };
  header_rdf.Foreach(search_and_fill, {"header"});

  if (run_numbers.size() < 2) return ERR_TimeTableTooShort;
  if (! std::equal(run_numbers.begin() + 1, run_numbers.end(), run_numbers.begin()) )
    return ERR_MoreThanOneRun;

  *run = run_numbers.front();
  if (*run < PUEO_FIRST_AMP_RUN) return ERR_PreAmpRun ;

  // It only makes sense if we have at least one valid second (ie 3 consecutive `event_second`s).
  // Note: the first second and the final second of any run are invalid (can't compute their pps delta),
  if (time_table->size() < 3) return ERR_TimeTableTooShort;

  // Mar 10 2026: I manually figured out the shortest run is 889 at 20 seconds, 
  // so this warning would actually never be triggered :)
  if (time_table->size() < 20)
  {                        
    print(time_table, std::cerr);
    fprintf(stderr, "\033[1;33mWarning: small table (run %d).\n\033[0m", *run);
  }
  
  int32_t warning_code = 0;

  // loop from first second to last second, if a second is missing, add it as invalid
  for (TimeTable::iterator current=time_table->begin(); current!=std::prev(time_table->end(),1);++current)
  {
    TimeTable::iterator next = std::next(current, 1);
    int32_t actual_next_second = current->first+1;

    // if the next second does not exist, then the current second's delta pps cannot be computed
    // the missing second's delta couldn't be computed either, because it wouldn't have a this_pps
    if (actual_next_second != next->first)
    {
      (*time_table)[actual_next_second].missing = true;
      (*time_table)[actual_next_second].invalid_delta = true;
      current->second.invalid_delta = true;
      warning_code |= ERR_MissingSecond;
    }
  }

  // Next, compute next_pps and delta := (next_pps - this_pps) of each valid second;
  // REMOVE entries with invalid deltas, since we're gonna perform an average later.

  // The first second doesn't have a valid delta, because its this_pps is garbage
  TimeTable::iterator first_second = (*time_table).begin();
  first_second->second.this_pps=0; // clear out the garbage value and make it more obvious that this is bad
  first_second->second.invalid_delta=true;
  invalid_seconds->insert(time_table->extract(first_second)); // move invalid entry to a separate table

  for (TimeTable::iterator current=time_table->begin(); current!=std::prev(time_table->end(),1);)
  {
    if (current->second.invalid_delta)
    {
      invalid_seconds->insert(time_table->extract(current++));
    }
    else
    {
      TimeTable::iterator next = std::next(current, 1);
      // the this_pps of next second is the current second's next_pps
      current->second.next_pps =  next->second.this_pps;;
      // Explicitly use uint32_t so that wrap-around subtraction is automatic
      uint32_t delta = current->second.next_pps - current->second.this_pps;
      // And then convert to int so that values below nominal show up as negative
      int32_t rel_delta = static_cast<int32_t>(delta) - NOMINAL_CLOCK_FREQ;
      current->second.relative_delta = rel_delta;

      if (TMath::Abs(rel_delta) > 100) warning_code |= ERR_LargeDelta;
      ++current;
    }  
  }

  // the final second doesn't have a valid delta, because there isn't a next second for it
  TimeTable::iterator final_second = std::prev((*time_table).end(),1);
  final_second->second.invalid_delta=true;
  invalid_seconds->insert(time_table->extract(final_second));

  return warning_code;
}

int32_t check_event_seconds(const TimeTable* time_table)
{
  for (auto &sec: *time_table)
  {
    if (sec.first < PUEO_LAUNCH_SECOND) return ERR_Year1970;
  }

  return 0;
}

int32_t simple_moving_average(TimeTable* time_table, std::size_t half_width)
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

int32_t find_stable_region_mid_point(TimeTable * time_table, TimeTable::iterator* anchor_point) 
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
    if (current==time_table->begin())
      current->second.avg_relative_delta = std::next(current)->second.avg_relative_delta;
    else if (current==std::prev(time_table->end(),1))
      current->second.avg_relative_delta = std::prev(current)->second.avg_relative_delta;
    else {
      TimeTable::iterator past = std::prev(current);
      TimeTable::iterator future = std::next(current);
      double avg = (past->second.avg_relative_delta + future->second.avg_relative_delta) / 2;
      current->second.avg_relative_delta = avg;
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

int32_t correct_one_event_second(TimeTable* pps_corrected_time_table, TimeTable::iterator* row,
                                 ROOT::RDF::RNode header_rdf, ROOT::RDF::RNode timemark_rdf)
{
  const int32_t ref_readout = (*row)->second.readout_time_sec;

  std::map<int32_t, pueo::Timemark> useful_timemarks;

  // Pick timemarks whose rising.utc_secs ≈ reference readout time chosen above
  auto distance_to_ref = [ref_readout](const pueo::Timemark& timemark)
    {return std::abs(static_cast<int32_t>(timemark.rising.GetSec()) - ref_readout);};

  auto fill_ordered_map = [&useful_timemarks] (int32_t dist, const pueo::Timemark& timemark)
    {useful_timemarks[dist] = timemark;};

  timemark_rdf.Define("distance", distance_to_ref, {"timemark"})
              .Filter([](int32_t dist){return dist<=5;}, {"distance"})
              .Foreach(fill_ordered_map, {"distance", "timemark"});
    
  // todo: maybe make this recoverable instead of erroring out
  if (useful_timemarks.empty()) return ERR_NoNearbyTimemark;

  // The one with the smallest distance, since the map is sorted
  // We will try to find an event whose subsecond is close to this best_timemark's subsecond
  // The matching row's event_second will be corrected using the bes_timemark's second
  const pueo::Timemark &best_timemark = useful_timemarks.begin()->second;

  // Filter out events that are not close to the reference point
  auto narrow_range = [ref_readout](pueo::RawHeader& rhdr)
    {return std::abs(static_cast<int32_t>(rhdr.readoutTime) - ref_readout) < 3;};
  auto narrowed_rdf = header_rdf.Filter(narrow_range, {"header"});

  auto compute_subsec = 
    [pps_corrected_time_table]
    (const pueo::RawHeader& rhdr)
    {
      double corrected_last_pps = 
        pps_corrected_time_table->find(rhdr.triggerTime)->second.corrected_pps;

      // around 125 million clock counts
      double avg_delta = static_cast<double>(NOMINAL_CLOCK_FREQ) + 
        pps_corrected_time_table->find(rhdr.triggerTime)->second.avg_relative_delta;

      // subsecond [clock counts]
      double subsecond = std::fmod(
        static_cast<double>(rhdr.trigTime) - corrected_last_pps + static_cast<double>(UINT32_MAX) + 1.,
        static_cast<double>(UINT32_MAX) + 1
      );

      subsecond /= avg_delta; // subsecond [sec]

      return subsecond * 1e9; // subsecton [nanosec]
    };
  auto subsecond_rdf = narrowed_rdf.Define("subsecond", compute_subsec, {"header"});

  // Locate the event that was timemarked (using subsecond as the identifier)
  auto double_abs_diff = [best_timemark](double subsec)
    {return std::fabs(subsec - best_timemark.rising.GetNanoSec());};
  auto result_rdf = subsecond_rdf.Define("diff", double_abs_diff, {"subsecond"})
                                 .Filter("diff < 200"); // todo: 200 is a magic-number tolerance

  // extract the result from result_rdf
  std::map<double, pueo::RawHeader> maybe_timemarked_events;
  auto extract_row = [&maybe_timemarked_events](double diff, const pueo::RawHeader& rhdr)
    {maybe_timemarked_events[diff] = rhdr;};
  result_rdf.Foreach(extract_row, {"diff", "header"});

  // todo: mayhap this is recoverable
  if (maybe_timemarked_events.size() != 1) return -1;

  // use the timemarked event's rising.utc_secs to correct the `event_second` in the TimeTable
  int32_t maybe_wrong_event_second = maybe_timemarked_events.begin()->second.triggerTime;
  *row = pps_corrected_time_table->find(maybe_wrong_event_second);
  (*row)->second.corrected_sec = static_cast<int32_t>(best_timemark.rising.GetSec());
  
  return 0;
}

void print(const TimeTable* time_table, std::ostream& stream)
{

  stream << "\nColor: \e[1;33m Invalid Delta \e[1;31m Missing and Invalid Delta\e[0m\n";
  stream << "Relative delta is defined to be (next_pps - this_pps) - 125000000\n";
  stream << "--------------------------------------------------------------------------------------------------------\n"
         << " event_second | corrected    | readout     | this_pps  | next_pps  | relative | avg. rel. | corrected  \n"
         << " from DAQ     | event_second | time (sec)  |           |           | delta    | delta     | this_pps   \n"
         << " int32_t      | int32_t      | int32_t     | uint32_t  | uint32_t  | int32_t  | double    | double     \n"
         << "--------------------------------------------------------------------------------------------------------\n";

  TString color;
  for (auto it=time_table->begin(); it!=time_table->end(); ++it)
  {
    if (it->second.missing && it->second.invalid_delta) color = "\033[1;31m ";  // red
    else if (it->second.invalid_delta) color = "\033[1;33m "; // yellow
    else color = "\033[0m ";

    stream << color << std::setw(14) << std::left << it->first
           << color << std::setw(14) << std::left << it->second.corrected_sec
           << color << std::setw(13) << std::left << it->second.readout_time_sec
           << color << std::setw(11) << it->second.this_pps
           << color << std::setw(11) << it->second.next_pps
           << color << std::setw(10) << it->second.relative_delta
           << color << std::setw(9)  << std::fixed << std::setprecision(2) << it->second.avg_relative_delta
           << color << std::setw(15) << std::right << std::fixed << std::setprecision(2) << it->second.corrected_pps << "\n";
  }

  stream << "\033[0m---------------------------------------------------------------------------------------\n";
}

void plot(TimeTable& time_table, TString name)
{
  int32_t t0 = time_table.begin()->first;
  int32_t xlow = 0;
  int32_t xhigh = 0;

  TGraph original_this_pps(time_table.size());
  TGraph corrected_this_pps(time_table.size());
  TGraph this_pps_residual(time_table.size());

  TGraph original_delta(time_table.size());
  TGraph avg_delta(time_table.size());

  std::size_t counter=0;
  for (auto& row: time_table){

    double original = row.second.this_pps;
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
  // original_this_pps.GetXaxis()->SetLabelSize(0);
  original_this_pps.GetYaxis()->SetTitle("[sysclk counts]");
  original_this_pps.GetYaxis()->SetTitleSize(0.1);
  original_this_pps.GetYaxis()->SetTitleOffset(0.3);
  original_this_pps.GetXaxis()->SetTitle(Form("Event Second [seconds since t0 (%d)]", t0));
  original_this_pps.GetXaxis()->SetTitleOffset(2.2);
  original_this_pps.GetXaxis()->CenterTitle();
  original_this_pps.GetXaxis()->SetLabelOffset(0.05);
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
  // original_delta.GetXaxis()->SetLabelSize(0);
  original_delta.GetXaxis()->SetTitle(Form("Event Second [seconds since t0 (%d)]", t0));
  original_delta.GetXaxis()->SetTitleOffset(2.2);
  original_delta.GetXaxis()->CenterTitle();
  original_delta.GetXaxis()->SetLabelOffset(0.05);
  if (xlow && xhigh)
    original_delta.GetXaxis()->SetRangeUser(xlow, xhigh);
  // original_delta.GetYaxis()->SetRangeUser(30, 40);

  avg_delta.Draw("P");
  avg_delta.SetMarkerStyle(kCircle);
  avg_delta.SetMarkerColor(kRed);

  TLegend leg3(0.1, 0.9, 0.4, 1); // (x1,y1,x2,y2) in NDC
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
  this_pps_residual.GetXaxis()->SetTitle(Form("Event Second [seconds since t0 (%d)]", t0));
  this_pps_residual.GetXaxis()->SetTitleOffset(2.2);
  this_pps_residual.GetXaxis()->CenterTitle();
  this_pps_residual.GetXaxis()->SetLabelOffset(0.05);
  if (xlow && xhigh)
    this_pps_residual.GetXaxis()->SetRangeUser(xlow, xhigh);
  this_pps_residual.GetYaxis()->SetRangeUser(-10, 10);

  c1.SaveAs(name);
}
