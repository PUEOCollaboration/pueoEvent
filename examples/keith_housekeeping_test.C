// Originally: filterSensor.cpp by Keith McBride
// For testing purposes, modified and uploaded in pueoEvent PR#12 (https://github.com/PUEOCollaboration/pueoEvent/pull/12)
// Detail: see Slack thread (https://pueomission.slack.com/archives/C0A7BGR9FB8/p1774279892587859)
// Usage: root -l -b -q keith_housekeeping_test.C

#include "ROOT/RDataFrame.hxx"
#include "TSystem.h"
#include "TGraph.h"
#include "TCanvas.h"

void keith_housekeeping_test(unsigned short i=112) {
  gSystem->Load("libpueoEvent.so");      // needs to be in LD_LIBRARY_PATH if running as a macro
  // gSystem->Load("libpueorawdata.so"); // not needed
  // gROOT->SetBatch(kTRUE);             // same as root -b

  ROOT::EnableImplicitMT(); // Use all CPU cores for 51M entries
  ROOT::RDataFrame df("hskTree", "/work/hsk_test_2026Jan1.root");

  auto meta_df = df.Filter(Form("sensor_id == %d", i))
    .Define("s_sub", [](const std::string s){ return s; }, {"subsys"})
    .Define("s_nam", [](const std::string s){ return s; }, {"sens_name"})
    .Define("s_kind", [](const char s){ return s; }, {"kind_unit"});

  // Take will collect all instances, but we'll only look at the first one.
  // This books the event loop (using MT) to fill the vectors
  auto subsys_ptr = meta_df.Take<std::string>("s_sub");
  auto sens_name_ptr = meta_df.Take<std::string>("s_nam");
  auto kind_ptr = meta_df.Take<char>("s_kind");

  // Accessing the pointer will trigger the event loop
  if (subsys_ptr->empty()) {
    std::cout << "Sensor ID not found!" << std::endl;
    return;
  }

  std::string subsys_name = subsys_ptr->at(0);
  std::string sensor_name = sens_name_ptr->at(0);
  char kind_name = kind_ptr->at(0);
  std::cout << "Processing: " << subsys_name << " - " << sensor_name << std::endl;
  float fmin=-1.0;
  float fmax=1.0;
  switch(kind_name) {
    case 'C': fmin = -40.0;  fmax = 85.0;    break;
    case 'V': fmin = 0.0;    fmax = 80.0;    break;
    case 'G': fmin = -2.0;   fmax = 2.0;     break;
    case 'A': fmin = 0.0;    fmax = 22.0;    break;
    case 'W': fmin = 0.0;    fmax = 1500.0;  break;
    case 'I': fmin = 0.0;    fmax = 20.0;    break;
    default:  fmin = 0.0;    fmax = 1000.0;  break; 
  }
  

  auto cut_ = [i,fmin,fmax]
    (unsigned short id, float fval, unsigned int time) 
    {
      return id == i && fval >= fmin && fval <= fmax && time >= 1766163300 && time <= 1768188000;
    };

  auto filtered = df.Filter(cut_, {"sensor_id", "fval", "time_secs"})
    .Define("double_time", "static_cast<double>(time_secs)")
    .Define("double_fval", "static_cast<double>(fval)");


  // @todo this will cause the dataframe to loop a second time, so maybe book this earlier
  auto graphPtr = filtered.Graph("double_time", "double_fval");

  Long64_t nPoints = filtered.Count().GetValue();
  std::cout << "Points being added to graph: " << nPoints << std::endl;
  // Use Clone() to create a persistent copy on the heap
  TGraph *gPersistent = (TGraph*)graphPtr->Clone();
  // shift the graph and divide by 3600 to get hours?
  double* xValues = gPersistent->GetX();
  int n = gPersistent->GetN();
  // int xMin = TMath::LocMin(n, xValues); // Returns index of min
  //double minVal = xValues[xMin];
  if (n > 0) {
    double offset = 1766163300; // Store the minimum x value
    //double offset = xValues[xMin]; // Store the minimum x value

    for (int i = 0; i < n; ++i) {
      xValues[i] = (xValues[i] - offset)/3600.0; // from secs to hours?
    }
  }

  // Crucial: Tell the graph its points have changed so it can update its internal limits
  //gPersistent->GetHistogram()->GetXaxis()->Set(100, gPersistent->GetXaxis()->GetXmin(), gPersistent->GetXaxis()->GetXmax());
  // Or simply:
  gPersistent->Set(n);
  TString gr_name(
    Form("%s with sensor ID %d and subsystem: %s; time(hours); %c ", 
         sensor_name.c_str(), (int) i, subsys_name.c_str(), kind_name)
  );

  gPersistent->SetTitle(gr_name);
  // 4. Draw from the pointer
  gPersistent->SetMarkerStyle(20);
  gPersistent->SetMarkerSize(0.15);
  TCanvas * c1 = new TCanvas("c1","c1", 1200,600);
  gPersistent->Draw("AP");
  //gPersistent->GetXaxis()->SetRangeUser(248.90,252.59);
  //gPersistent->GetYaxis()->SetRangeUser(226.0,1060.1);
  gPad->Modified(); // Tell the pad something changed
  gPad->Update();   // Force the pad to calculate the axis ranges NOW
  // 2. Get the current Y-axis limits so the line fills the whole height
  // double ymin = gPad->GetUymin();
  // double ymax = gPad->GetUymax();

  // 3. Create the line at x = 1500
  //double xVal1= (double) (1767123180 - 1766225000)/3600.0;
  //TLine *line = new TLine(xVal1, ymin, xVal1, ymax);
  //line->SetLineColor(kRed);
  //line->SetLineStyle(2); // Dashed
  //line->SetLineWidth(2);
  //line->Draw();

  c1->SetGrid();
  c1->SaveAs(Form("%s.pdf", sensor_name.c_str()));
}
