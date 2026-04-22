#include "ROOT/RDataFrame.hxx"
#include "TChain.h"
#include "TCanvas.h"
#include <regex>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

enum err_code
{
  ERR_NotContiguous = 1<<0,
  ERR_NotIdentical = 1<<1
};


int32_t check_contiguity(ROOT::RDF::RNode rdf, uint32_t * run){
  std::vector<std::pair<int32_t, int32_t>> event_second_and_correction;

  auto run_vector = rdf.Take<uint32_t>("run");

  rdf.Foreach(
    [&event_second_and_correction]
    (int32_t event_second, int32_t corrected_event_second)
    {
      event_second_and_correction.emplace_back(std::make_pair(event_second, corrected_event_second));
    },
    {"event_second", "corrected_event_second"}
  );

  (*run) = run_vector->front();
  int32_t error_code = 0;

  int32_t prev_second = event_second_and_correction.front().second;
  for(std::size_t i=1; i<event_second_and_correction.size(); ++i){
    int32_t this_second = event_second_and_correction.at(i).second;

    if (this_second != prev_second+1) {
      // std::cout << "\e[1;31m" << this_second << ", previous second: " << prev_second << "\e[0m\n";
      error_code |= ERR_NotContiguous;
      prev_second=event_second_and_correction.at(i).second;

    } else {
      // std::cout << this_second << ", previous second: " << prev_second << "\n";
      prev_second++;
    }
  }

  for(auto &p: event_second_and_correction){
    if (p.second != p.first){
      error_code |= ERR_NotIdentical;
      // std::cout << "\e[1;31m" << p.first << ": " << p.second << "\e[0m\n";
    } else {
      // std::cout << p.first << ": " << p.second << "\n";
    }

  }

  return error_code;
}

void plot_diff(ROOT::RDF::RNode rdf){
  auto diff_bw = [](int32_t readout_sec, int32_t corrected_event_second)
    {return readout_sec - corrected_event_second;};
  auto diff_df = rdf.Filter("readout_time_sec > 0 && corrected_event_second > 0")
                    .Define("readout_diff", diff_bw, {"readout_time_sec", "corrected_event_second"})
                    .Define("correction_diff", diff_bw, {"event_second", "corrected_event_second"});

  auto gr  = diff_df.Range(0, 0, 50).Graph<int32_t, int32_t>("readout_time_sec", "readout_diff");
  auto gr2 = diff_df.Range(0, 0, 50).Graph<int32_t, int32_t>("readout_time_sec", "correction_diff");
  auto gr3 = diff_df.Range(0, 0, 50).Filter("correction_diff>-1700*std::pow(10,6)")
                    .Graph<int32_t, int32_t>("readout_time_sec", "correction_diff");

  TCanvas c1("","", 1920, 1080);  
  c1.Divide(1,3);
  c1.cd(1);
  gr->SetTitle("RawHeader.readout_time - RawHeader.corrected_event_second");
  gr->GetYaxis()->CenterTitle();
  gr->GetYaxis()->SetTitle("Difference [sec]");
  gr->GetXaxis()->SetTitle("Date");
  gr->GetXaxis()->SetTimeDisplay(1);
  gr->GetXaxis()->SetTimeFormat("%m/%d/%Y%F1970-01-01 00:00:00");
  gr3->GetXaxis()->SetLabelSize(0.1);
  gr->GetYaxis()->SetTitleOffset(0.3);
  gr->GetYaxis()->SetTitleSize(0.1);
  gr->Draw("AP");
  gr->SetMarkerStyle(kCircle);
  gr->SetMarkerSize(1);
  c1.cd(2);
  gr2->SetTitle("RawHeader.event_second - RawHeader.corrected_event_second");
  gr2->GetYaxis()->CenterTitle();
  gr2->GetYaxis()->SetTitle("Difference [sec]");
  gr2->GetXaxis()->SetTitle("Date");
  gr2->GetXaxis()->SetTimeDisplay(1);
  gr2->GetXaxis()->SetTimeFormat("%m/%d/%Y%F1970-01-01 00:00:00");
  gr3->GetXaxis()->SetLabelSize(0.1);
  gr2->GetYaxis()->SetTitleOffset(0.3);
  gr2->GetYaxis()->SetTitleSize(0.1);
  gr2->GetYaxis()->SetLabelSize(0.05);
  gr2->Draw("AP");
  gr2->SetMarkerStyle(kCircle);
  gr2->SetMarkerSize(1);
  c1.cd(3);
  gr3->SetTitle("(RawHeader.event_second - RawHeader.corrected_event_second) : without TURF GPS Glitches");
  gr3->GetYaxis()->CenterTitle();
  gr3->GetYaxis()->SetTitle("Difference [sec]");
  gr3->GetXaxis()->SetTitle("Date");
  gr3->GetXaxis()->SetTimeDisplay(1);
  gr3->GetXaxis()->SetTimeFormat("%m/%d/%Y%F1970-01-01 00:00:00");
  gr3->GetYaxis()->SetTitleOffset(0.3);
  gr3->GetYaxis()->SetTitleSize(0.1);
  gr3->GetYaxis()->SetLabelSize(0.05);
  gr3->GetXaxis()->SetLabelSize(0.1);
  gr3->Draw("ALP");
  gr3->SetMarkerStyle(kCircle);
  gr3->SetMarkerSize(0.5);
  gr3->GetYaxis()->SetRangeUser(-1,1);
  c1.SaveAs("event_second_diff_readout_second.svg");
  
}

void time_table_event_second_check_health(){

  const char * msg1 = " none-contiguous";
  const char * msg2 = " none-identical";

  // uint32_t run = 0;
  // ROOT::RDataFrame time_table_rdf("time_table_tree", "/work/time_tables/run801/time_table.root");
  // int32_t err = check_contiguity(time_table_rdf, &run);
  // if (err & ERR_NotIdentical && err & ERR_NotContiguous)
  //   std::cout << "run " << run << ":" << msg1 << "," << msg2 << '\n';
  // else if (err & ERR_NotContiguous){
  //   std::cout << "run " << run << ":" << msg1 << '\n';
  // }
  // else if (err & ERR_NotIdentical) {
  //   std::cout << "run " << run << ":" << msg2 << '\n';
  // }
    
  
  fs::recursive_directory_iterator time_tables_dir_iter( "/work/time_tables");
  const std::regex pattern(R"(time_table\.root)");
  // for(auto const& entry: time_tables_dir_iter)
  // {
  //   if (!entry.is_regular_file()) continue;
  //
  //   std::string name = entry.path().filename();
  //   if (!std::regex_match(name, pattern)) continue;
  //   ROOT::RDataFrame time_table_rdf("time_table_tree", entry.path().c_str());
  //
  //   uint32_t run        = 0;
  //   int32_t err = check_contiguity(time_table_rdf, &run);
  //   if (err & ERR_NotIdentical && err & ERR_NotContiguous)
  //     std::cout << "run " << run << ":" << msg1 << "," << msg2 << '\n';
  //   else if (err & ERR_NotContiguous){
  //     std::cout << "run " << run << ":" << msg1 << '\n';
  //   }
  //   else if (err & ERR_NotIdentical) {
  //     std::cout << "run " << run << ":" << msg2 << "\n";
  //   }
  // }
  TChain time_table_chain("time_table_tree");
  for(auto const& entry: time_tables_dir_iter)
  {
    if (!entry.is_regular_file()) continue;

    std::string name = entry.path().filename();
    if (!std::regex_match(name, pattern)) continue;
    time_table_chain.Add(entry.path().c_str());
  }
  ROOT::RDataFrame time_table_rdf(time_table_chain);
  std::cout << *time_table_rdf.Count() << " entries\n";
  plot_diff(time_table_rdf);
}


