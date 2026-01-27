// Implementation of the Dataset class, essentially a subset of `Dataset.cc`

#include "pueo/Dataset.h"
#include "pueo/UsefulEvent.h"
#include "pueo/RawHeader.h"
#include "pueo/Nav.h"
#include "pueo/TruthEvent.h" 
#include "pueo/Version.h" 
#include "pueo/Conventions.h"

#include <math.h>
#include "TFile.h" 
#include "TTree.h" 
#include <stdlib.h>
#include "TMutex.h"
#include "TEventList.h" 
#include "TCut.h" 
#include <dirent.h>
#include <algorithm>
#include "TEnv.h" 
#include <iostream>
#include <fstream>

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
  if (!fEventTree) return nullptr; 
  if (fEventTree->GetReadEntry() != fWantedEntry || force_load) 
  {
    fEventTree->GetEntry(fWantedEntry); 
  }
  return fHaveUsefulFile ? fUsefulEvent : 
              fRawEvent ? fRawEvent : fUsefulEvent; 
}

pueo::UsefulEvent * pueo::Dataset::useful(bool force_load) 
{

  if (!fEventTree) return nullptr; 

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
    if (!fHaveUsefulFile) fUsefulDirty = true; 
    if (!fHaveGpsEvent) fGpsDirty = true; 
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

      std::vector<const char *> possibleTimeRunMapFiles;
      possibleTimeRunMapFiles.reserve(3);
      possibleTimeRunMapFiles.emplace_back(Form("%s/timerunmap_%d.txt", getenv("PUEO_CALIB_DIR"),version)); 
      possibleTimeRunMapFiles.emplace_back(Form("%s/share/pueoCalib/timerunmap_%d.txt", getenv("PUEO_UTIL_INSTALL_DIR"),version)); 
      possibleTimeRunMapFiles.emplace_back(Form("./calib/timerunmap_%d.txt",version)); 
      const char * cache_file_name = checkIfFilesExist(possibleTimeRunMapFiles);
      if (cache_file_name) 
      {
          FILE * cf = fopen(cache_file_name,"r"); 
          run_info r; 
          while(!feof(cf))
          {
            fscanf(cf,"%d %lf %lf\n", &r.run, &r.start_time, &r.stop_time); 
            run_times[version].push_back(r); 
          }
          fclose(cf); 
      }

      if (!cache_file_name) 
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

            std::vector<const char *> possibleHeaderFiles;
            possibleHeaderFiles.reserve(2);
            possibleHeaderFiles.emplace_back(Form("%s/run%d/timedHeadFile%d.root", data_dir, run, run));
            possibleHeaderFiles.emplace_back(Form("%s/run%d/headFile%d.root", data_dir, run, run));

            if (const char * the_right_file = checkIfFilesExist(possibleHeaderFiles))
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
