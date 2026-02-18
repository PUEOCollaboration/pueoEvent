//# Dataset Example 
//This assumes you have the environmental PUEO_ROOT_DATA pointing at some simulated output

// load libPueoEvent
R__LOAD_LIBRARY(libpueoEvent) 

void dataset_example(int run = 813, int entry  = 10) 
{

  // create a dataset using MC data
  pueo::Dataset d(run);
  d.getEntry(entry);

  //  get channel number for  phi 10, top ring,  , vpol
  int chan = pueo::GeomTool::Instance().getChanIndexFromRingPhiPol(pueo::ring::kTopRing, 10, pueo::pol::kVertical); 

  auto g = d.useful()->makeGraph(chan); 
  g->SetTitle(Form("Run %d, Entry %d, Ant 1001V",run,entry));

  g->Draw("alp"); 

}

