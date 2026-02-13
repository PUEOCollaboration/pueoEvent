#include "ROOT/RDataFrame.hxx"
#include "TSystem.h"
#include <iostream>
#include <iomanip>
#include <map>

using ROOT::RDataFrame;

struct second_boundaries 
{
  UInt_t start = 0;
  UInt_t end = 0;
  UInt_t delta = 0; // last_pps - llast_pps, with rollover taken care of
};

// approximately equal with some arbitrary default tolerance
bool approx_equal(UInt_t a, UInt_t b, UInt_t tolerance = 200)
{
  UInt_t diff = a > b ? a - b : b - a;
  return diff <= tolerance;
}

void print(std::map<UInt_t, second_boundaries>& encounters);

// average value of (end-start); last two seconds should be excluded when computing this,
// because they don't have (start, end) pairs.
UInt_t average_delta(std::map<UInt_t, second_boundaries>& encounters, UInt_t exclude1, UInt_t exclude2);

// Some assumptions about the data are made:
// (a)  The column `event_second` is monotonically increasing, ie "sorted"
// (b)  0 <= event_second[x] - event_second[x-1] <= 1
void header_time_postprocessor_toy()
{
  gSystem->Load("libpueoEvent.so");

  // default already disables this so no need to explicitly disable
  // ROOT::DisableImplicitMT(); // can't multithread cuz of the lambda capture
  RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/bfmr_r739_head.root");
  // RDataFrame tmp_header_rdf("header", "/usr/pueoBuilder/install/bin/real_R0813_head.root");

  std::map<UInt_t, second_boundaries> encounters; // stores unique encounters of `event_second`

  UInt_t previous_second   = UINT32_MAX;     // initialize to garbage
  UInt_t previous_previous = UINT32_MAX - 1; // initialize to garbage
  auto search_and_fill = 
    [&encounters, &previous_second, &previous_previous]
    (UInt_t event_second, UInt_t lpps)
    {
      bool new_encounter = encounters.find(event_second) == encounters.end();
      if (new_encounter) 
      {
        encounters.emplace(event_second, second_boundaries{});
        // the start of the previous second is the lpps of the current second
        // note that this inserts garbage into `encounters` during the first two iterations;
        encounters[previous_second].start = lpps;
        encounters[previous_previous].end = lpps;
        encounters[previous_previous].delta = lpps - encounters[previous_previous].start;
        // note: wrap-around subtraction is automatic for unsigned integers
        
        previous_previous = previous_second;
        previous_second = event_second;
      }
    };
  tmp_header_rdf.Foreach(search_and_fill, {"triggerTime","lastPPS"});
  encounters.erase(UINT32_MAX);  // erase the garbage
  encounters.erase(UINT32_MAX-1);
  print(encounters);

  // UInt_t avg_delta = average_delta(encounters, previous_second, previous_previous);
  //
  // // iterate from 1st entry to 2nd-to-last entry
  // for (auto it = encounters.begin(); it != std::prev(encounters.end()); it++) 
  // {
  //
  // }
  exit(0);
}

void print(std::map<UInt_t, second_boundaries>& encounters)
{
  std::cout << "   second   |   start   |    end    |  delta" << "\n"
            << "--------------------------------------------\n";
  for (auto& e: encounters)
  {
    std::cout << std::setw(11) << e.first
              << " " << std::setw(11) << e.second.start
              << " " << std::setw(11) << e.second.end
              << " " << std::setw(11) << e.second.delta << "\n";
  }
}

UInt_t average_delta(std::map<UInt_t, second_boundaries>& encounters, UInt_t exclude1, UInt_t exclude2)
{
  ULong64_t sum = 0;
  for (auto& e: encounters) sum += e.second.delta;
  sum -= encounters[exclude1].delta;
  sum -= encounters[exclude2].delta;
  sum /= encounters.size()-2;
  return sum;
}
