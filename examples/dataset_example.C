//# Dataset Example 
//This assumes you have the environmental PUEO_MC_DATA pointing at some simulated output
// Since no real PUEO data exists yet, I assume you don't have any 

// load libPueoEvent
R__LOAD_LIBRARY(libpueoEvent) 

void dataset_example(int run = 2, int entry  = 10) 
{

  // create a dataset using MC data
  pueo::Dataset d(run, pueo::Dataset::PUEO_MC_DATA);
  d.getEntry(entry) ;

  //  get channel number for  phi 10, top ring,  , vpol
  int chan = pueo::GeomTool::Instance().getChanIndexFromRingPhiPol(pueo::ring::kTopRing, 10, pueo::pol::kVertical); 

  auto g = d.useful()->makeGraph(chan); 
  g->SetTitle(Form("Run %d, Entry %d, Ant 1001V",run,entry));

  g->Draw("alp"); 

}

