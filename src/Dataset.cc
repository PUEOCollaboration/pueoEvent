/****************************************************************************************
*   Dataset.cc            The implementation of pueo::Dataset
*  
*   This file is used for convenient access to PUEO data. Adapted from AnitaDatast. 
* 
*  Cosmin Deaconu <cozzyd@kicp.uchicago.edu>    
*  Ben Strutt <strutt@physics.ucla.edu> 
*
*
*  (C) 2023-, The Payload for Ultrahigh Energy Observations (PUEO) Collaboration
* 
*  This file is part of pueoEvent, the ROOT I/O library for PUEO. 
* 
*  pueoEvent is free software: you can redistribute it and/or modify it under the
*  terms of the GNU General Public License as published by the Free Software
*  Foundation, either version 2 of the License, or (at your option) any later
*  version.
* 
*  Foobar is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
*  A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
*  You should have received a copy of the GNU General Public License along with
*  Foobar. If not, see <https://www.gnu.org/licenses/
*
****************************************************************************************/ 






#include "pueo/Dataset.h"
#include "pueo/UsefulEvent.h"
#include "pueo/RawHeader.h"
#include "pueo/Nav.h"
#include "pueo/TruthEvent.h" 
#include "pueo/Version.h" 
#include "pueo/Conventions.h"

#include "TTreeIndex.h" 
#include <math.h>
#include "TFile.h" 
#include "TTree.h" 
#include <stdlib.h>
#include <unistd.h>
#include "TMath.h"
#include "TROOT.h"
#include "TEventList.h" 
#include "TCut.h" 
#include "TMutex.h" 
#include <dirent.h>
#include <algorithm>
#include "TEnv.h" 
#include <iostream>
#include <fstream>
#include <sstream>


// from runToEvP*.txt
static std::vector<UInt_t> firstEvents[pueo::k::NUM_PUEO+1];
static std::vector<UInt_t> lastEvents[pueo::k::NUM_PUEO+1];
static std::vector<Int_t> runs[pueo::k::NUM_PUEO+1];


static std::vector<UInt_t> hiCalEventNumbers[pueo::k::NUM_PUEO+1];

static TFile* fHiCalGpsFile;
static TTree* fHiCalGpsTree;
static Double_t fHiCalLon;
static Double_t fHiCalLat;
static Double_t fHiCalAlt;
static Int_t fHiCalUnixTime;




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




static bool checkIfFileExists(const char * file)
{
  return access(file, R_OK) !=-1; 
}

static const char * checkIfFilesExist(int num, ...)
{

  va_list args; 
  va_start(args, num); 

  for (int i = 0; i < num; i++) 
  {
    const char * f = va_arg(args, const char *); 

    if (checkIfFileExists(f))
    {
      return f; 
    }

  }

  va_end(args); 

  return 0; 
}

static const char  pueo_root_data_dir_env[]  = "PUEO_ROOT_DATA"; 
static const char  pueo_versioned_root_data_dir_env[]  = "PUEO%d_ROOT_DATA"; 
static const char  mc_root_data_dir[] = "PUEO_MC_DATA"; 

const char * pueo::Dataset::getDataDir(DataDirectory dir) 
{
  int version = (int) dir; 

  //if anita version number is defined in argument
  if (version > 0) 
  {

    char env_string[sizeof(pueo_versioned_root_data_dir_env)+20]; 
    sprintf(env_string,pueo_versioned_root_data_dir_env, version); 
    const char * tryme = getenv(env_string); 
    if (!tryme)
    {
      fprintf(stderr,"%s, not defined, will try %s\n",env_string, pueo_root_data_dir_env); 
    }
    else return tryme; 
  }
  
  //if monte carlo
  if (version == 0) 
  {
    const char * tryme = getenv(mc_root_data_dir); 
    if (!tryme)
    {
      fprintf(stderr,"%s, not defined, will try %s\n",mc_root_data_dir, pueo_root_data_dir_env); 
    }
    else return tryme; 
  }

  //if version argument is default (-1)
  //if PUEO_ROOT_DATA exists return that, otherwise return what AnitaVersion thinks it should be
  if (const char * tryme = getenv(pueo_root_data_dir_env))
  {
    return tryme; 
  }
  else
  {
    char env_string[sizeof(pueo_versioned_root_data_dir_env)+20]; 
    sprintf(env_string,pueo_versioned_root_data_dir_env, version::get());
    if (const char *tryme = getenv(env_string)) {
      return tryme;
    }
    else {
      fprintf(stderr,"%s, not defined, please define it!", pueo_root_data_dir_env); 
      return 0;
    }
  }

  
}


pueo::Dataset::Dataset(int run, bool decimated,  DataDirectory version, BlindingStrategy strategy)
  : 
  fHeadTree(0), fHeader(0), 
  fEventTree(0), fRawEvent(0), fUsefulEvent(0), 
  fGpsTree(0), fGps(0), 
  fTruthTree(0), fTruth(0), 
  fCutList(0), fRandy()
{
  fHaveUsefulFile = false;
  setStrategy(strategy); 
  currRun = run;
  loadRun(run, decimated, version); 
  loadedBlindTrees = false;
  zeroBlindPointers();
  loadBlindTrees(); // want this to come after opening the data files to try to have correct ANITA flight
}

void  pueo::Dataset::unloadRun() 
{

  for (unsigned i = 0; i < filesToClose.size(); i++) 
  {
    filesToClose[i]->Close(); 
    delete filesToClose[i]; 
  }

  fHeadTree = 0; 
  fDecimatedHeadTree = 0; 
  fEventTree = 0; 
  fGpsTree = 0; 
  fRunLoaded = false;
  filesToClose.clear();

  if (fCutList) 
  {
    delete fCutList; 
    fCutList = 0; 
  }  

}


pueo::nav::Attitude * pueo::Dataset::gps(bool force_load)
{

  if (fHaveGpsEvent)
  {
    if (fGpsTree->GetReadEntry() != fWantedEntry || force_load) 
    {
      fGpsTree->GetEntry(fWantedEntry); 
    }
  }
  else
  {
    if (fGpsDirty || force_load)
    {
      //try one that matches realtime
      int gpsEntry = fGpsTree->GetEntryNumberWithBestIndex(round(header()->triggerTime + header()->triggerTimeNs  / 1e9)); 
//      int offset = 0; 
      fGpsTree->GetEntry(gpsEntry); 
      /*
      while (fGps->attFlag == 1 && abs(offset) < 30)
      {
        offset = offset >= 0 ? -(offset+1) : -offset; 
        if (gpsEntry + offset < 0) continue; 
        if (gpsEntry + offset >= fGpsTree->GetEntries()) continue; 
        fGpsTree->GetEntry(gpsEntry+offset); 
      }
      if (fGps->attFlag==1)
      {
        fprintf(stderr,"WARNING: Could not find good GPS within 30 entries... reverting to closest bad one\n"); 
        fGpsTree->GetEntry(gpsEntry); 
      }
      */

      fGpsDirty = false; 
    }

  }

  return fGps; 

}



pueo::RawHeader * pueo::Dataset::header(bool force_load) 
{
  if (fDecimated)
  {
    if (force_load) 
    {
      fDecimatedHeadTree->GetEntry(fDecimatedEntry); 
    }
  }
  else if ((fHeadTree->GetReadEntry() != fWantedEntry || force_load)) 
  {
    fHeadTree->GetEntry(fWantedEntry); 
  }



  if(theStrat & kInsertedVPolEvents){
    Int_t fakeTreeEntry = needToOverwriteEvent(pol::kVertical, fHeader->eventNumber);
    if(fakeTreeEntry > -1){
      overwriteHeader(fHeader, pol::kVertical, fakeTreeEntry);
    }
  }


  if(theStrat & kInsertedHPolEvents){
    Int_t fakeTreeEntry = needToOverwriteEvent(pol::kHorizontal, fHeader->eventNumber);
    if(fakeTreeEntry > -1){
      overwriteHeader(fHeader, pol::kHorizontal, fakeTreeEntry);
    }
  }

  return fHeader; 
}


pueo::RawEvent * pueo::Dataset::raw(bool force_load) 
{
  if (fEventTree->GetReadEntry() != fWantedEntry || force_load) 
  {
    fEventTree->GetEntry(fWantedEntry); 
  }
  return fHaveUsefulFile ? fUsefulEvent : 
              fRawEvent ? fRawEvent : fUsefulEvent; 
}


pueo::UsefulEvent * pueo::Dataset::useful(bool force_load) 
{

  if (fEventTree->GetReadEntry() != fWantedEntry || force_load) 
  {

    fEventTree->GetEntry(fWantedEntry); 
    fUsefulDirty = fRawEvent; //if reading UsefulEvents, then no need to do anything
  }
  
  if (fUsefulDirty)
  {
    if (!fUsefulEvent) 
    {
      fUsefulEvent = new UsefulEvent; 
    }

    fUsefulEvent->~UsefulEvent();
    new (fUsefulEvent) UsefulEvent(*fRawEvent, *header()); 
    fUsefulDirty = false; 
  }

  // This is the blinding implementation for the header

  if(theStrat & kInsertedVPolEvents){
    Int_t fakeTreeEntry = needToOverwriteEvent(pol::kVertical, fUsefulEvent->eventNumber);
    if(fakeTreeEntry > -1){
      overwriteEvent(fUsefulEvent, pol::kVertical, fakeTreeEntry);
    }
  }


  if(theStrat & kInsertedHPolEvents){
    Int_t fakeTreeEntry = needToOverwriteEvent(pol::kHorizontal, fUsefulEvent->eventNumber);
    if(fakeTreeEntry > -1){
      overwriteEvent(fUsefulEvent, pol::kHorizontal, fakeTreeEntry);
    }
  }


  if ((theStrat & kRandomizePolarity) && maybeInvertPolarity(fUsefulEvent->eventNumber))
  {
    // std::cerr << "Inverting event " << fUsefulEvent->eventNumber << std::endl;
    for(int surf=0; surf < k::ACTIVE_SURFS; surf++){
      for(int chan=0; chan < k::NUM_CHANS_PER_SURF; chan++){
        int chanIndex = surf*k::NUM_CHANS_PER_SURF + chan;
        for(size_t samp=0; samp < fUsefulEvent->volts[chanIndex].size(); samp++){
          fUsefulEvent->volts[chanIndex][samp] *= -1;
          fUsefulEvent->data[chanIndex][samp] *= -1; // do the pedestal subtracted data too
        }
      }
    }
  }

  return fUsefulEvent; 
}

// Calling this function on it's own is just for unblinding, please use honestly
Bool_t pueo::Dataset::maybeInvertPolarity(UInt_t eventNumber){
  // add additional check here for clarity, in case people call this function on it's own?
  if((theStrat & kRandomizePolarity) > 0){
    fRandy.SetSeed(eventNumber); // set seed from event number, makes this deterministic regardless of order events are processed
    Double_t aboveZeroFiftyPercentOfTheTime = fRandy.Uniform(-1, 1); // uniformly distributed random number between -1 and 1  
    return (aboveZeroFiftyPercentOfTheTime < 0);
  }
  return false;
}

int pueo::Dataset::getEntry(int entryNumber)
{

  //invalidate the indices 
  fIndex = -1; 
  fCutIndex=-1; 
  
 
  if (entryNumber < 0 || entryNumber >= (fDecimated ? fDecimatedHeadTree : fHeadTree)->GetEntries())
  {
    fprintf(stderr,"Requested entry %d too big or small!\n", entryNumber); 
  }
  else
  {
    (fDecimated ? fDecimatedEntry : fWantedEntry) = entryNumber; 
    if (fDecimated)
    {
      fDecimatedHeadTree->GetEntry(fDecimatedEntry); 
      fWantedEntry = fHeadTree->GetEntryNumberWithIndex(fHeader->eventNumber); 

    }
    fUsefulDirty = true; 
    fGpsDirty = true; 
  }


  // use the header to set the PUEO version 
  version::setVersionFromUnixTime(header()->realTime); 

  return fDecimated ? fDecimatedEntry : fWantedEntry; 
}


int pueo::Dataset::getEvent(int eventNumber, bool quiet)
{

  int entry  =  (fDecimated ? fDecimatedHeadTree : fHeadTree)->GetEntryNumberWithIndex(eventNumber); 

  if (entry < 0 && (eventNumber < fHeadTree->GetMinimum("eventNumber") || eventNumber > fHeadTree->GetMaximum("eventNumber")))
  {
    int run = getRunContainingEventNumber(eventNumber);
    if(run > 0)
    {
      loadRun(run, fDecimated,datadir);
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
  
pueo::Dataset::~Dataset() 
{

  unloadRun(); 



  if (fHeader) 
    delete fHeader; 

  if (fUsefulEvent) 
  {
    delete fUsefulEvent; 
  }

  if (fRawEvent) 
    delete fRawEvent; 

  if (fGps) 
    delete fGps; 


  if (fTruth) 
    delete fTruth; 

  if (fCutList) 
    delete fCutList;

  // // Since we've set the directory to 0 for these,
  // // ROOT won't delete them when the fBlindFile is closed
  // // So we need to do it here.
  // for(int pol=0; pol < AnitaPol::kNotAPol; pol++){
  //   if(fBlindHeadTree[pol]){
  //     delete fBlindHeadTree[pol];
  //     fBlindHeadTree[pol] = NULL;
  //   }
  //   if(fBlindEventTree[pol]){
  //     delete fBlindEventTree[pol];
  //     fBlindEventTree[pol] = NULL;
  //   }
  //   if(fBlindHeader[pol]){
  //     delete fBlindHeader[pol];
  //     fBlindHeader[pol] = NULL;
  //   }
  //   if(fBlindEvent[pol]){
  //     delete fBlindEvent[pol];
  //     fBlindEvent[pol] = NULL;
  //   }
  // }

  if(fBlindFile){
    fBlindFile->Close();
    delete fBlindFile;
  }

  
}

bool  pueo::Dataset::loadRun(int run, bool dec,  DataDirectory dir) 
{

  datadir = dir; 

  // stop loadRun() changing the ROOT directory
  // in case you book histograms or trees after instantiating AnitaDataset  
  const TString theRootPwd = gDirectory->GetPath();
  
  fDecimated = dec; 
  fIndices = 0; 

  currRun = run;

  unloadRun(); 
  fWantedEntry = 0; 

  const char * data_dir = getDataDir(dir); 

  //seems like a good idea 
  
  int version = (int) dir; 
  if (version>0) version::set(version); 

  //if decimated, try to load decimated tree

  if (fDecimated) 
  {

    fDecimatedEntry = 0; 
    TString fname = TString::Format("%s/run%d/decimatedHeadFile%d.root", data_dir, run, run); 
    if (checkIfFileExists(fname.Data()))
    {
      TFile * f = new TFile(fname.Data()); 
      filesToClose.push_back(f); 
      fDecimatedHeadTree = (TTree*) f->Get("headTree"); 
      fDecimatedHeadTree->BuildIndex("eventNumber"); 
      fDecimatedHeadTree->SetBranchAddress("header",&fHeader); 
      fIndices = ((TTreeIndex*) fDecimatedHeadTree->GetTreeIndex())->GetIndex(); 
    }
    else
    {
      fprintf(stderr," Could not find decimated head file for run %d, giving up!\n", run); 
      fRunLoaded = false;
      return false; 
    }
  }
  else
  {
    fDecimatedHeadTree = 0; 
  }
  // try to load timed header file 
  
  // For telemetered crap 
  TString fname0 = TString::Format("%s/run%d/eventHeadFile%d.root", data_dir, run, run);
  TString fname1 = TString::Format("%s/run%d/timedHeadFile%dOfflineMask.root", data_dir, run, run);   
  TString fname2 = TString::Format("%s/run%d/timedHeadFile%d.root", data_dir, run, run); 
  TString fname3 = TString::Format("%s/run%d/headFile%d.root", data_dir, run, run); 
  TString fname4 = TString::Format("%s/run%d/SimulatedHeadFile%d.root", data_dir, run, run);

  bool simulated = false; 

  if (const char * the_right_file = checkIfFilesExist(5, fname0.Data(), fname1.Data(), fname2.Data(), fname3.Data(), fname4.Data()))
  {

    if (strcasestr(the_right_file,"SimulatedHeadFile")) simulated = true; 

    fprintf(stderr,"Using head file: %s\n",the_right_file); 
    TFile * f = new TFile(the_right_file); 
    filesToClose.push_back(f); 
    fHeadTree = (TTree*) f->Get("headTree"); 
  }
  else 
  {
    fprintf(stderr,"Could not find head file for run %d, giving up!\n", run); 
    fRunLoaded = false;
    return false; 
  }

  if (!fDecimated) fHeadTree->SetBranchAddress("header",&fHeader); 

  fHeadTree->BuildIndex("eventNumber"); 

  if (!fDecimated) fIndices = ((TTreeIndex*) fHeadTree->GetTreeIndex())->GetIndex(); 

  //try to load gps event file  
  TString fname = TString::Format("%s/run%d/gpsEvent%d.root", data_dir, run, run);
  fname2 = TString::Format("%s/run%d/SimulatedGpsFile%d.root", data_dir, run, run); 
  if (const char * the_right_file = checkIfFilesExist(2,fname.Data(),fname2.Data()))
  {
     TFile * f = new TFile(the_right_file); 
     filesToClose.push_back(f); 
     fGpsTree = (TTree*) f->Get("adu5PatTree"); 
     fHaveGpsEvent = true; 

  }
  // load gps file instead
  else 
  {
    fname = TString::Format("%s/run%d/gpsFile%d.root", data_dir, run, run);
    if (const char * the_right_file = checkIfFilesExist(1, fname.Data()))
    {
       TFile * f = new TFile(the_right_file); 
       filesToClose.push_back(f); 
       fGpsTree = (TTree*) f->Get("adu5PatTree"); 
       fGpsTree->BuildIndex("realTime"); 
       fHaveGpsEvent = false; 
    }
    else 
    {
      fprintf(stderr,"Could not find gps file for run %d, giving up!\n",run); 
      fRunLoaded = false;
      return false; 
    }
  }

  fGpsTree->SetBranchAddress("pat",&fGps); 


  //try to load useful event file 

  fname = TString::Format("%s/run%d/usefulEventFile%d.root", data_dir, run, run);
  fname2 = TString::Format("%s/run%d/SimulatedEventFile%d.root", data_dir, run, run); 
  if (const char * the_right_file = checkIfFilesExist(2, fname.Data(), fname2.Data()))
  {
     TFile * f = new TFile(the_right_file); 
     filesToClose.push_back(f); 
     fEventTree = (TTree*) f->Get("eventTree"); 
     fHaveUsefulFile = true; 
     fEventTree->SetBranchAddress("event",&fUsefulEvent); 
  }
  else 
  {
    fname = TString::Format("%s/run%d/eventFile%d.root", data_dir, run, run); 
    if (checkIfFileExists(fname.Data()))
    {
       TFile * f = new TFile(fname.Data()); 
       filesToClose.push_back(f); 
       fEventTree = (TTree*) f->Get("eventTree"); 
       fHaveUsefulFile = false; 
       fEventTree->SetBranchAddress("event",&fRawEvent); 
       
    }
  }
  

  //try to load truth 
  if (simulated)
  {
    fname = TString::TString::Format("%s/run%d/SimulatedTruthFile%d.root",data_dir,run,run);
    if (checkIfFileExists(fname.Data()))
    {
     TFile * f = new TFile(fname.Data()); 
     filesToClose.push_back(f); 
     fTruthTree = (TTree*) f->Get("truthTree"); 
     fTruthTree->SetBranchAddress("truth",&fTruth); 
    }
  }

  //load the first entry 
  getEntry(0); 
  

  fRunLoaded = true;

  // stop loadRun() changing the ROOT directory
  // in case you book histograms or trees after instantiating AnitaDataset
  gDirectory->cd(theRootPwd); 
  
  return true; 
}


int pueo::Dataset::previousEvent() 
{
  if (fIndex < 0) 
  {
    fIndex = TMath::BinarySearch(N(), fIndices, fDecimated ? fDecimatedEntry : fWantedEntry); 
  }

  if (fIndex >0) 
    fIndex--; 

  return nthEvent(fIndex); 
}


int pueo::Dataset::firstEvent()
{
  return nthEvent(0); 
}

int pueo::Dataset::lastEvent()
{
  return nthEvent(N()-1); 
}

int pueo::Dataset::nthEvent(int n)
{
  int ret = getEntry(fIndices[n]); 
  fIndex = n; 
  return ret; 
}



int pueo::Dataset::nextEvent() 
{
  if (fIndex < 0) 
  {
    fIndex = TMath::BinarySearch(N(), fIndices, fDecimated ? fDecimatedEntry : fWantedEntry); 
  }

  if (fIndex < N() -1)
    fIndex++; 

  return nthEvent(fIndex); 
}

int pueo::Dataset::N() const
{
  TTree* t = fDecimated? fDecimatedHeadTree : fHeadTree;
  return t ? t->GetEntries() : 0;
}

int pueo::Dataset::previousMinBiasEvent()
{
  if (fIndex < 0)
  {
    fIndex = TMath::BinarySearch(N(), fIndices, fDecimated ? fDecimatedEntry : fWantedEntry);
  }

  while(fIndex >= 0)
  {
    fIndex--;
    if(fIndex < 0)
    {
      loadRun(currRun - 1);
      fIndex = N() - 1;
    }
    fHeadTree->GetEntry(fIndex);
    if((fHeader->trigType&1) == 0) break;
  }
  
  return nthEvent(fIndex);
}

int pueo::Dataset::nextMinBiasEvent()
{
  if (fIndex < 0)
  {
    fIndex = TMath::BinarySearch(N(), fIndices, fDecimated ? fDecimatedEntry : fWantedEntry);
  }

  while(fIndex <= N()-1)
  {
    fIndex++;
    if(fIndex == N())
    {
      loadRun(currRun + 1);
      fIndex = 0;
    }
    fHeadTree->GetEntry(fIndex);
    if((fHeader->trigType&1) == 0) break;
  }
  
  return nthEvent(fIndex);
}


int pueo::Dataset::setCut(const TCut & cut)
{
  if (fCutList) 
  {
    delete fCutList; 
  }

  int n = (fDecimated? fDecimatedHeadTree : fHeadTree)->Draw(">>evlist1",cut,"goff"); 
  fCutList = (TEventList*) gDirectory->Get("evlist1");
  return n; 
}



int pueo::Dataset::NInCut() const
{

  if (fCutList)
  {
    return fCutList->GetN(); 
  }

  return -1; 
}

int pueo::Dataset::firstInCut() 
{
  return nthInCut(0); 
}

int pueo::Dataset::lastInCut() 
{
  return nthInCut(NInCut()-1); 
}


int pueo::Dataset::nthInCut(int i)
{
  if (!fCutList) 
    return -1; 
  int ret = getEntry(fCutList->GetEntry(i)); 

  fCutIndex = i;
  return ret;
}

int pueo::Dataset::nextInCut()
{
  if (!fCutList) return -1; 
  if (fCutIndex < 0) 
  {
    fCutIndex = TMath::BinarySearch(NInCut(), fCutList->GetList(), (Long64_t) (fDecimated ? fDecimatedEntry : fWantedEntry)); 
  }

  if (fCutIndex <  NInCut() - 1) 
  {
    fCutIndex++; 
  }
  return nthInCut(fCutIndex); 

}

int pueo::Dataset::previousInCut()
{
  if (!fCutList) return -1; 
  if (fCutIndex < 0) 
  {
    fCutIndex = TMath::BinarySearch(NInCut(), fCutList->GetList(),(Long64_t)  (fDecimated ? fDecimatedEntry : fWantedEntry)); 
  }

  if (fCutIndex >  0) 
  {
    fCutIndex--; 
  }
  return nthInCut(fCutIndex); 

}

int pueo::Dataset::setPlaylist(const char* playlist)
{
  if (!fPlaylist.empty()) 
  {
    fPlaylist.clear(); 
  }

  int n = loadPlaylist(playlist); 
  return n; 
}

int pueo::Dataset::NInPlaylist() const
{

  if (!fPlaylist.empty())
  {
    return fPlaylist.size(); 
  }

  return -1; 
}

int pueo::Dataset::firstInPlaylist() 
{
  return nthInPlaylist(0); 
}

int pueo::Dataset::lastInPlaylist() 
{
  return nthInPlaylist(NInPlaylist()-1); 
}


int pueo::Dataset::nthInPlaylist(int i)
{
  if (fPlaylist.empty()) return -1; 
	fPlaylistIndex = i;
	if(getCurrRun() != getPlaylistRun()) loadRun(getPlaylistRun());
  int ret = getEvent(getPlaylistEvent()); 

  return ret;
}

int pueo::Dataset::nextInPlaylist()
{
  if (fPlaylist.empty()) return -1; 
	if(fPlaylistIndex < 0) fPlaylistIndex = 0;
  if (fPlaylistIndex <  NInPlaylist() - 1) 
  {
    fPlaylistIndex++; 
  }
  return nthInPlaylist(fPlaylistIndex); 

}

int pueo::Dataset::previousInPlaylist()
{
  if (fPlaylist.empty()) return -1; 
	if(fPlaylistIndex < 0) fPlaylistIndex = 0;
  if (fPlaylistIndex >  0) 
  {
    fPlaylistIndex--; 
  }
  return nthInPlaylist(fPlaylistIndex); 

}


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
  if(simulatedData == true)
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

    }
  else
    {	
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
	}
      else
	{
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




pueo::TruthEvent * pueo::Dataset::truth(bool force_reload) 
{

  if (!fTruthTree) return 0; 
  if (fTruthTree->GetReadEntry() != fWantedEntry || force_reload) 
  {
    fTruthTree->GetEntry(fWantedEntry); 
  }

  return fTruth; 
}



struct run_info
{

  double start_time; 
  double stop_time; 
  int run;

  bool operator< (const run_info & other) const 
  {
    return stop_time < other.stop_time; 
  }


}; 

static std::vector<run_info> run_times[pueo::k::NUM_PUEO+1]; 

static TMutex run_at_time_mutex; 
int pueo::Dataset::getRunAtTime(double t)
{

  int version= version::getVersionFromUnixTime(t); 

  if (!run_times[version].size())
  {
    TLockGuard lock(&run_at_time_mutex); 
    if (!run_times[version].size()) 
    {

      // load from cache
      bool found_cache = false; 


      TString cache_file1= TString::Format("%s/timerunmap_%d.txt", getenv("PUEO_CALIB_DIR"),version) ; 
      TString cache_file2= TString::Format("%s/share/pueoCalib/timerunmap_%d.txt", getenv("PUEO_UTIL_INSTALL_DIR"),version) ; 
      TString cache_file3= TString::Format("./calib/timerunmap_%d.txt",version); 

      const char * cache_file_name = checkIfFilesExist(3, cache_file1.Data(), cache_file2.Data(), cache_file3.Data()); 

      if (checkIfFileExists(cache_file_name))
      {

          found_cache = true; 
          FILE * cf = fopen(cache_file_name,"r"); 
          run_info r; 
          while(!feof(cf))
          {
            fscanf(cf,"%d %lf %lf\n", &r.run, &r.start_time, &r.stop_time); 
            run_times[version].push_back(r); 
          }
          fclose(cf); 
      }


      if (!found_cache) 
      {
        //temporarily suppress errors and disable recovery
        int old_level = gErrorIgnoreLevel;
        int recover = gEnv->GetValue("TFile.Recover",1); 
        gEnv->SetValue("TFile.Recover",1); 
        gErrorIgnoreLevel = kFatal; 

        const char * data_dir = getDataDir((DataDirectory)version); 
        fprintf(stderr,"Couldn't find run file map. Regenerating %s from header files in %s\n", cache_file_name,data_dir); 
        DIR * dir = opendir(data_dir); 

        while(struct dirent * ent = readdir(dir))
        {
          int run; 
          if (sscanf(ent->d_name,"run%d",&run))
          {

            TString fname1 = TString::Format("%s/run%d/timedHeadFile%d.root", data_dir, run, run); 
            TString fname2 = TString::Format("%s/run%d/headFile%d.root", data_dir, run, run); 

            if (const char * the_right_file = checkIfFilesExist(2, fname1.Data(), fname2.Data()))
            {
              TFile f(the_right_file); 
              TTree * t = (TTree*) f.Get("headTree"); 
              if (t) 
              {
                run_info  ri; 
                ri.run = run; 
                //TODO do this to nanosecond precision 
                ri.start_time= t->GetMinimum("triggerTime"); 
                ri.stop_time = t->GetMaximum("triggerTime") + 1; 
                run_times[version].push_back(ri); 
              }
            }
          }
        }

        gErrorIgnoreLevel = old_level; 
        gEnv->SetValue("TFile.Recover",recover); 
        std::sort(run_times[version].begin(), run_times[version].end()); 

        TString try2write;  
        try2write.Form("./calib/timerunmap_%d.txt",version); 
        FILE * cf = fopen(try2write.Data(),"w"); 

        if (cf) 
        {
          const std::vector<run_info> &  v = run_times[version]; 
          for (unsigned i = 0; i < v.size(); i++)
          {
              printf("%d %0.9f %0.9f\n", v[i].run, v[i].start_time, v[i].stop_time); 
              fprintf(cf,"%d %0.9f %0.9f\n", v[i].run, v[i].start_time, v[i].stop_time); 
          }

         fclose(cf); 
        }

      }
    }
  }
  
  run_info test; 
  test.start_time =t; 
  test.stop_time =t; 
  const std::vector<run_info> & v = run_times[version]; 
  std::vector<run_info>::const_iterator it = std::upper_bound(v.begin(), v.end(), test); 

  if (it == v.end()) return -1; 
  if (it == v.begin() && (*it).start_time >t) return -1; 
  return (*it).run; 
}

void pueo::Dataset::zeroBlindPointers(){
  loadedBlindTrees = false;

  for(int pol=0; pol < pol::kNotAPol; pol++){
    fBlindHeadTree[pol] = NULL;
    fBlindEventTree[pol] = NULL;
    fBlindHeader[pol] = NULL;
    fBlindEvent[pol] = NULL;
  }

  fBlindFile = NULL;
}

TString pueo::Dataset::getDescription(BlindingStrategy strat){

  TString description = "Current strategy: ";

  if(strat == kNoBlinding){
    description = "No blinding. ";
  }

  if(strat & kInsertedVPolEvents){
    description += "VPol events inserted. ";
  }

  if(strat & kInsertedHPolEvents){
    description += "HPol events inserted. ";
  }

  if(strat & kRandomizePolarity){
    description += "Polarity randomized. ";
  }


  return description;
}

pueo::Dataset::BlindingStrategy pueo::Dataset::setStrategy(BlindingStrategy newStrat){
  theStrat = newStrat;
  return theStrat;
}


pueo::Dataset::BlindingStrategy pueo::Dataset::getStrategy(){
  return theStrat;
}

void pueo::Dataset::loadBlindTrees() {

  // std::cout << __PRETTY_FUNCTION__ << std::endl;

  /* for now, just A3 */ 
  if(!loadedBlindTrees && version::get()==3){

    fBlindFile = NULL;

    char calibDir[FILENAME_MAX];
    char fileName[FILENAME_MAX];
    char *calibEnv=getenv("PUEO_CALIB_DIR");
    if(!calibEnv) {
      char *utilEnv=getenv("PUEO_UTIL_INSTALL_DIR");
      if(!utilEnv){
	sprintf(calibDir,"calib");
      }
      else{
	snprintf(calibDir,sizeof(calibDir),"%s/share/pueoCalib",utilEnv);
      }
    }
    else {
      strncpy(calibDir,calibEnv,FILENAME_MAX);
    }

    // std::cout << __PRETTY_FUNCTION__ << ": here 2" << std::endl;


    // these are the fake events, that will be inserted in place of some min bias events    
    snprintf(fileName,sizeof(fileName), "%s/insertedDataFile.root", calibDir);

    TString theRootPwd = gDirectory->GetPath();
    // std::cerr << "Before opening blind file" << "\t" << gDirectory->GetPath() << std::endl;
    fBlindFile = TFile::Open(fileName);
    // std::cerr << "After opening blind file" << "\t" << gDirectory->GetPath() << std::endl;
    
    if(fBlindFile){

      TString polPrefix[pol::kNotAPol];
      polPrefix[pol::kHorizontal] = "HPol";
      polPrefix[pol::kVertical] = "VPol";

      for(int pol=0; pol < pol::kNotAPol; pol++){

	TString headTreeName = polPrefix[pol] + "HeadTree";
	fBlindHeadTree[pol] = (TTree*) fBlindFile->Get(headTreeName);

	TString eventTreeName = polPrefix[pol] + "EventTree";
	fBlindEventTree[pol] = (TTree*) fBlindFile->Get(eventTreeName);

	// If you found the data then prepare for data reading
	if(fBlindHeadTree[pol] && fBlindEventTree[pol]){

	  fBlindHeadTree[pol]->SetBranchAddress("header", &fBlindHeader[pol]);
	  fBlindEventTree[pol]->SetBranchAddress("event", &fBlindEvent[pol]);          
	}
	else{
	  // complain if you can't find the data
	  std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": "
		    << "fBlindHeadTree[" << pol << "] = " << fBlindHeadTree[pol] << ", "
		    << "fBlindEventTree[" << pol << "] = " << fBlindEventTree[pol] << std::endl;
	}
      }
    }
    else{
      std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": "
		<< "Unable to find " << fileName << " for inserted event blinding." << std::endl;
    }

    // std::cout << __PRETTY_FUNCTION__ << ": here 3" << std::endl;

    // these are the min bias event numbers to be overwritten, with the entry in the fakeEventTree
    // that is used to overwrite the event
    snprintf(fileName,sizeof(fileName), "%s/pueo%dOverwrittenEventInfo.txt",calibDir, version::get());
    std::ifstream overwrittenEventInfoFile(fileName);
    char firstLine[180];
    overwrittenEventInfoFile.getline(firstLine,179);
    UInt_t overwrittenEventNumber;
    Int_t fakeTreeEntry, pol;
    Int_t numEvents = 0;
    while(overwrittenEventInfoFile >> overwrittenEventNumber >> fakeTreeEntry >> pol){
      eventsToOverwrite.push_back(overwrittenEventNumber);
      fakeTreeEntries.push_back(fakeTreeEntry);
      pol::pol_t thePol = pol::pol_t(pol);
      polarityOfEventToInsert.push_back(thePol);
      // std::cout << overwrittenEventNumber << "\t" << fakeTreeEntry << "\t" << thePol << std::endl;
      numEvents++;
    }
    if(numEvents==0){
      std::cerr << "Warning in " << __FILE__ << std::endl;
      std::cerr << "Unable to find overwrittenEventInfo" << std::endl;
    }

    gDirectory->cd(theRootPwd);
    
    // const char* treeNames[4] = {"HPolHeadTree", "HPolEventTree", "VPolHeadTree", "VPolEventTree"};
    // for(int i=0; i < 4; i++){
    //   std::cerr << "after close file " << "\t" << gROOT->FindObject(treeNames[i]) << std::endl;
    // }    

    loadedBlindTrees = true;
  }

//   gROOT->cd(0); 
  // std::cout << __PRETTY_FUNCTION__ << ": here 4" << std::endl;

}



void pueo::Dataset::loadHiCalGps() {
  if(!fHiCalGpsFile){

    const TString theRootPwd = gDirectory->GetPath();    

    TString fName = TString::Format("%s/share/pueoCalib/H1b_GPS_time_interp.root", getenv("ANITA_UTIL_INSTALL_DIR"));
    fHiCalGpsFile = TFile::Open(fName);
    fHiCalGpsTree = (TTree*) fHiCalGpsFile->Get("Tpos");

    fHiCalGpsTree->BuildIndex("unixTime");

    fHiCalGpsTree->SetBranchAddress("longitude", &fHiCalLon);
    fHiCalGpsTree->SetBranchAddress("latitude", &fHiCalLat);
    fHiCalGpsTree->SetBranchAddress("altitude", &fHiCalAlt);
    fHiCalGpsTree->SetBranchAddress("unixTime", &fHiCalUnixTime);

    gDirectory->cd(theRootPwd);
  }
}



/** 
 * Where was hical? Uses the current header realTime to query the hical gps tree...
 * 
 * @param longitude hical position
 * @param latitude hical position
 * @param altitude hical position
 */
void pueo::Dataset::hiCal(Double_t& longitude, Double_t& latitude, Double_t& altitude) {
  UInt_t realTime = fHeader ? fHeader->realTime : 0;
  hiCal(realTime, longitude, latitude, altitude);
}



/** 
 * Where was hical at a particular time?
 * 
 * @param longitude hical position
 * @param latitude hical position
 * @param altitude hical position
 * @param realTime unixTime stamp of the gps tree
 */
void pueo::Dataset::hiCal(UInt_t realTime, Double_t& longitude, Double_t& latitude, Double_t& altitude) {
  loadHiCalGps();
  Long64_t entry = fHiCalGpsTree->GetEntryNumberWithIndex(realTime);

  if(entry > 0){
    fHiCalGpsTree->GetEntry(entry);
    longitude = fHiCalLon;
    latitude = fHiCalLat;
    const double feetToMeters = 0.3048;
    altitude = fHiCalAlt*feetToMeters;
  }
  else{
    longitude = -9999;
    latitude = -9999;
    altitude = -9999;
  }
}



 


/**
 * Loop through list of events to overwrite for a given polarisation and return the fakeTreeEntry we need to overwrite
 *
 * @param pol is the polarity to consider blinding
 * @param eventNumber is the eventNumber, obviously
 *
 * @return -1 if we don't overwrite, the entry in the fakeTree otherwise
 */
Int_t pueo::Dataset::needToOverwriteEvent(pol::pol_t pol, UInt_t eventNumber){

  Int_t fakeTreeEntry = -1;
  for(UInt_t i=0; i < polarityOfEventToInsert.size(); i++){
    if(polarityOfEventToInsert.at(i)==pol && eventNumber == eventsToOverwrite.at(i)){
      fakeTreeEntry = fakeTreeEntries.at(i);
      break;
    }
  }
  return fakeTreeEntry;
}

void pueo::Dataset::overwriteHeader(RawHeader* header, pol::pol_t pol, Int_t fakeTreeEntry){

  Int_t numBytes = fBlindHeadTree[pol]->GetEntry(fakeTreeEntry);

  if(numBytes <= 0){
    std::cerr << "Warning in " << __PRETTY_FUNCTION__ << ", I read " << numBytes << " from the blinding tree " << fBlindHeadTree[pol]->GetName()
              << ". This probably means the salting blinding is broken" << std::endl;    
  }

  // Retain some of the header data for camouflage
  UInt_t realTime = header->realTime;
  UInt_t triggerTimeNs = header->triggerTimeNs;
  UInt_t eventNumber = header->eventNumber;
  Int_t run = header->run;
  Int_t trigNum = header->trigNum;

  (*header) = (*fBlindHeader[pol]);

  header->realTime = realTime;
  header->triggerTimeNs = triggerTimeNs;
  header->eventNumber = eventNumber;
  header->run = run;
  header->trigNum = trigNum;

}

void pueo::Dataset::overwriteEvent(UsefulEvent* useful, pol::pol_t pol, Int_t fakeTreeEntry){

  Int_t numBytes = fBlindEventTree[pol]->GetEntry(fakeTreeEntry);
  if(numBytes <= 0){
    std::cerr << "Warning in " << __PRETTY_FUNCTION__ << ", I read " << numBytes << " from the blinding tree " << fBlindEventTree[pol]->GetName()
              << ". This probably means the salting blinding is broken" << std::endl;    
  }
  

  UInt_t eventNumber = useful->eventNumber;
  /*
  UInt_t surfEventIds[NUM_SURF] = {0};
  UChar_t wrongLabs[NUM_SURF*NUM_CHAN] = {0};
  UChar_t rightLabs[NUM_SURF*NUM_CHAN] = {0};
  for(int surf=0; surf < NUM_SURF; surf++){
    surfEventIds[surf] = useful->surfEventId[surf];

    for(int chan=0; chan < NUM_CHAN; chan++){
      const int chanIndex = surf*NUM_CHAN + chan;
      wrongLabs[chanIndex] = UChar_t(fBlindEvent[pol]->getLabChip(chanIndex));
      rightLabs[chanIndex] = UChar_t(useful->getLabChip(chanIndex));
      // std::cout << chanIndex << "\t" << chipIdFlags[chanIndex] << "\t" << (chipIdFlags[chanIndex] & 0x3) << std::endl;
    }
  }
  */

  (*useful) = (*fBlindEvent[pol]);

  useful->eventNumber = eventNumber;
  /*
  for(int surf=0; surf < NUM_SURF; surf++){
    useful->surfEventId[surf] = surfEventIds[surf];

    // here we manually set the bits in the chipId flag that correspond to the lab chip
    // this ensures that as you click through magic display the LABS will still go A->B->C->D->A...
    for(int chan=0; chan < NUM_CHAN; chan++){

      const int chanIndex = surf*NUM_CHAN + chan;
      // std::cerr << useful->chipIdFlag[chanIndex] << "\t";
      useful->chipIdFlag[chanIndex] -= wrongLabs[chanIndex];
      // std::cerr << useful->chipIdFlag[chanIndex] << "\t";
      useful->chipIdFlag[chanIndex] += rightLabs[chanIndex];
      // std::cerr << useful->chipIdFlag[chanIndex] << std::endl;


    }
  }
  */

}







