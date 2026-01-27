// Implementation of the legacy (ANITA) portion of the Dataset class
// Right now, this means anything that has to do with the static member function `getRunContainingEventNumber`

#include "pueo/Dataset.h"
#include "pueo/Conventions.h"
#include "TMutex.h" 
#include "TTree.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// from runToEvP*.txt
namespace {
  std::vector<UInt_t> firstEvents[pueo::k::NUM_PUEO+1];
  std::vector<UInt_t> lastEvents[pueo::k::NUM_PUEO+1];
  std::vector<Int_t>  runs[pueo::k::NUM_PUEO+1];
}

void pueo::Dataset::loadRunToEv(int pueo){
  static TMutex m;
  m.Lock();

  const char* installDir = getenv("PUEO_UTIL_INSTALL_DIR");

  TString fileName = TString::Format("%s/share/anitaCalib/runToEvP%d.txt", installDir, pueo);
  std::ifstream runToEv(fileName.Data());
  if (!runToEv.is_open()) {
    std::cerr << "Error in " << __PRETTY_FUNCTION__ << " couldn't find " << fileName << std::endl;
  }
  else{
    const int hopefullyMoreThanEnoughRuns = 500;
    runs[pueo].reserve(hopefullyMoreThanEnoughRuns);
    firstEvents[pueo].reserve(hopefullyMoreThanEnoughRuns);
    lastEvents[pueo].reserve(hopefullyMoreThanEnoughRuns);

    int run, evLow,evHigh;
    // int elem = 0;
    while (runToEv >> run >> evLow >> evHigh){
      runs[pueo].push_back(run);
      firstEvents[pueo].push_back(evLow);
      lastEvents[pueo].push_back(evHigh);
      // std::cout << anita << "\t" << elem << "\t" << runs[anita][elem] << "\t" << firstEvents[anita][elem] << "\t" << lastEvents[anita][elem] << std::endl;
      // elem++;
    }
    // std::cout << "Finished reading in " << fileName << "!" << std::endl;
    runToEv.close();
  }
  m.UnLock();
}

// this is a static method
int pueo::Dataset::getRunContainingEventNumber(UInt_t ev){

  // TMutex();

  int pueo = version::get();

  // read in runToEvP*.txt list only once
  if(runs[pueo].size()==0){
    loadRunToEv(pueo);
    // If still don't have data, loadRunToEv should have printed something
    // There's a problem, so just return -1
    if(runs[pueo].size()==0){
      return -1;
    }
  }

  // Binary search to find first event number which is greater than ev
  std::vector<UInt_t>::iterator it = std::upper_bound(firstEvents[pueo].begin(), firstEvents[pueo].end(), ev);

  // Here we convert the iterator to an integer relative to first element, so we can read matching elements in the other array
  // And -1 so we have the last element isn't greater than ev.
  int elem = (it - firstEvents[pueo].begin()) - 1;

  // std::cout << anita << "\t" << elem << "\t" << runs[anita][elem] << "\t" << firstEvents[anita][elem] << "\t" << lastEvents[anita][elem] << std::endl;

  int run = -1; // signifies error, we will set correctly after doing bounds checking...

  if(elem < 0){ // then we are lower than the first event in the first run
    std::cerr << "Error in " << __PRETTY_FUNCTION__ << ", for PUEO " << version::get()
              << " eventNumber " << ev << " is smaller than then first event in the lowest run! "
              << "(eventNumber " << firstEvents[pueo][0] << " in run " << runs[pueo][0] << ")"
              << std::endl;
  }
  else if(ev > lastEvents[pueo].back()){ // then we are higher than the last event in the last run
    std::cerr << "Error in " << __PRETTY_FUNCTION__ << ", for pueo " << version::get()
              << " eventNumber " << ev << " is larger than then last event I know about in the highest run! "
              << "(eventNumber " << lastEvents[pueo].back() << " in run " << runs[pueo].back() << ")"
              << std::endl;
  }
  else if(ev > lastEvents[pueo][elem]){ // then we are in the eventNumber gap between two runs
    std::cerr << "Error in " << __PRETTY_FUNCTION__ << ", for pueo " << version::get()
              << " eventNumber " << ev << " is larger than then last event in run " << runs[pueo][elem]
              << " (" << lastEvents[pueo][elem] << "), but smaller than the first event in run "
              << runs[pueo][elem+1] << " (" << firstEvents[pueo][elem+1] << ")" << std::endl;
  }
  else{
    // preliminarily set run
    run = runs[pueo][elem];
  }

  return run;
}

int pueo::Dataset::getEvent(int eventNumber, bool quiet)
{

  int entry  =  (fDecimated ? fDecimatedHeadTree : fHeadTree)->GetEntryNumberWithIndex(eventNumber); 

  if (entry < 0 && (eventNumber < fHeadTree->GetMinimum("eventNumber") || eventNumber > fHeadTree->GetMaximum("eventNumber")))
  {
    int run = getRunContainingEventNumber(eventNumber);
    if(run > 0)
    {
      loadRun(run, datadir, fDecimated);
      if (!quiet) fprintf(stderr, "changed run to %d\n", run);
      getEvent(eventNumber, quiet); 
    }
  }
  else if (entry < 0 ) 
  {
      if (!quiet) fprintf(stderr,"WARNING: event %lld not found in header tree\n", fWantedEntry); 
      if (fDecimated) 
      {
        if (!quiet) fprintf(stderr,"\tWe are using decimated tree, so maybe that's why?\n"); 
      }
      return -1; 
   }

  getEntry(entry);
  return fDecimated ? fDecimatedEntry : fWantedEntry; 
}

int pueo::Dataset::loadPlaylist(const char* playlist)
{
  std::vector<std::vector<long> > runEv;
  int rN;
  int evN;
  std::ifstream pl(playlist);
  pl >> evN;

  // Simulated events
  // As iceMC generates random eventNumbers, simulated data event numbers aren't linked to actual event numbers, so ignore evN restrictions
  Bool_t simulatedData = false; // must be set to false for non-simulated data
  if(simulatedData == true) // well, how the fuck is this ever going to be true???
  {
    std::cout << "Using simulated data! Turn off the simulatedData variable if you are working with real data." << std::endl;
    rN = evN;
    pl >> evN;
    std::vector<long> Row;
    Row.push_back(rN);
    Row.push_back(evN);
    runEv.push_back(Row);
    while(pl >> rN >> evN)
    {
      std::vector<long> newRow;
      newRow.push_back(rN);
      newRow.push_back(evN);
      runEv.push_back(newRow);
    }

  } else {	
    if(evN < 400)
    {
      rN = evN;
      pl >> evN;
      std::vector<long> Row;
      Row.push_back(rN);
      Row.push_back(evN);
      runEv.push_back(Row);
      while(pl >> rN >> evN)
      {
        std::vector<long> newRow;
        newRow.push_back(rN);
        newRow.push_back(evN);
        runEv.push_back(newRow);
      }
    } else {
      rN = getRunContainingEventNumber(evN);
      if(rN == -1)
      {
        fprintf(stderr, "Something is wrong with your playlist\n");
        return -1;
      }
      std::vector<long> Row;
      Row.push_back(rN);
      Row.push_back(evN);
      runEv.push_back(Row);
      while(pl >> evN)
      {
        rN = getRunContainingEventNumber(evN);
        if(rN == -1)
        {
          fprintf(stderr, "Something is wrong with your playlist\n");
          return -1;
        }
        std::vector<long> newRow;
        newRow.push_back(rN);
        newRow.push_back(evN);
        runEv.push_back(newRow);
      }
    }
  }
  fPlaylist = runEv;
  return runEv.size();
}
