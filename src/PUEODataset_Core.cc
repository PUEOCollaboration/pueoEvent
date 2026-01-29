// Implementation of the core of the Dataset class (ie the constructor and its helpers)
#include "pueo/Dataset.h"
#include "pueo/Version.h"
#include "pueo/RawEvent.h"
#include "pueo/UsefulEvent.h"
#include "pueo/Nav.h"
#include "pueo/TruthEvent.h"
#include "pueo/RawHeader.h"
#include "TEventList.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeIndex.h"
#include <iostream>
#include <fstream>

// why initialize this way instead of assigning to nullptr in the header file?
pueo::Dataset::Dataset(int run,  DataDirectory version, bool decimated, BlindingStrategy strategy)
  : 
  fHeadTree(nullptr), fHeader(nullptr), 
  fEventTree(nullptr), fRawEvent(nullptr), fUsefulEvent(nullptr), 
  fGpsTree(nullptr), fGps(nullptr), 
  fTruthTree(nullptr), fTruth(nullptr), 
  fCutList(nullptr), fRandy()
{
  fHaveUsefulFile = false;
  setStrategy(strategy); 
  currRun = run;
  loadRun(run, version, decimated); 
  loadedBlindTrees = false;
  zeroBlindPointers();
  loadBlindTrees(); // want this to come after opening the data files to try to have correct ANITA flight
}

bool pueo::Dataset::loadRun(int run, DataDirectory dir, bool dec) 
{
  datadir = dir; 

  // stop loadRun() from changing the ROOT directory
  // in case you book histograms or trees after instantiating AnitaDataset  
  const TString theRootPwd = gDirectory->GetPath();
  
  fDecimated = dec; 
  fIndices = nullptr; 

  currRun = run;

  unloadRun(); 
  fWantedEntry = 0; 

  const char * data_dir = getDataDir(dir); 

  //seems like a good idea 
  int version = (int) dir; 
  if (version>0) version::set(version); 

  if (!fDecimated) 
  {
    fDecimatedHeadTree = nullptr; 
  }
  else //if decimated, try to load decimated tree
  {
    fDecimatedEntry = 0; 
    TFile * decHead = TFile::Open(TString::Format("%s/run%d/decimatedHeadFile%d.root", data_dir, run, run)); 
    if (decHead == nullptr || decHead->IsZombie())
    {
      fprintf(stderr, "Could not find decimated header file for run %d, giving up!\n", run); 
      fRunLoaded = false;
      return false; 
    }
    filesToClose.push_back(decHead); 
    fDecimatedHeadTree = (TTree*) decHead->Get("headTree"); 
    if (!fDecimatedHeadTree) 
    {
      fprintf(stderr, "Could not load decimated header tree for run %d from %s", run, decHead->GetName());
      fRunLoaded = false;
      return false;
    }
    fDecimatedHeadTree->BuildIndex("eventNumber"); 
    fDecimatedHeadTree->SetBranchAddress("header",&fHeader); 
    fIndices = ((TTreeIndex*) fDecimatedHeadTree->GetTreeIndex())->GetIndex(); 
  }
  // try to load timed header file 
  
  // For telemetered crap 
  std::vector<TString> possibleHeaders;
  possibleHeaders.reserve(5);
  possibleHeaders.emplace_back(TString::Format("%s/run%d/eventHeadFile%d.root", data_dir, run, run));
  possibleHeaders.emplace_back(TString::Format("%s/run%d/timedHeadFile%d.root", data_dir, run, run)); 
  possibleHeaders.emplace_back(TString::Format("%s/run%d/headFile%d.root", data_dir, run, run)); 
  possibleHeaders.emplace_back(TString::Format("%s/run%d/SimulatedHeadFile%d.root", data_dir, run, run));
  possibleHeaders.emplace_back(TString::Format("%s/run%d/SimulatedPueoHeadFile%d.root", data_dir, run, run));

  bool simulated = false; 

  const char * aHeader = checkIfFilesExist(possibleHeaders);
  if (!aHeader)
  {
    fprintf(stderr,"Could not find header file for run %d, giving up!\n", run);
    fRunLoaded = false;
    return false; 
  }
  if (strcasestr(aHeader,"Simulated")) simulated = true; 

  fprintf(stdout, "Using header file: %s\n", aHeader);

  TFile * headFile = TFile::Open(aHeader);
  if (headFile == nullptr || headFile->IsZombie()) {
    fprintf(stderr, "Failed to open %s", aHeader);
    fRunLoaded = false;
    return false;
  }
  filesToClose.push_back(headFile);
  fHeadTree = (TTree*) headFile->Get("headTree");

  if (fHeadTree == nullptr) {
    fprintf(stderr, "Failed to load header tree for run %d from %s", run, data_dir);
    fRunLoaded = false;
    return false;
  }

  if (!fDecimated) {
    fHeadTree->SetBranchAddress("header",&fHeader);
    fHeadTree->BuildIndex("eventNumber");
    fIndices = ((TTreeIndex*) fHeadTree->GetTreeIndex())->GetIndex(); 
  }

  //try to load gps event file  
  std::vector<TString> possibleGps;
  possibleGps.reserve(5);
  possibleGps.emplace_back(TString::Format("%s/run%d/gpsEvent%d.root", data_dir, run, run));
  possibleGps.emplace_back(TString::Format("%s/run%d/SimulatedGpsFile%d.root", data_dir, run, run)); 
  possibleGps.emplace_back(TString::Format("%s/run%d/SimulatedPueoGpsFile%d.root", data_dir, run, run));
  if (const char * the_right_file = checkIfFilesExist(possibleGps))
  {
    TFile * f = TFile::Open(the_right_file);
    if (f->IsZombie()) {
      fprintf(stderr,"Could not load gps file for run %d from %s, giving up!\n",run, data_dir); 
      fRunLoaded = false;
      return false;
    }
    filesToClose.push_back(f); 
    fGpsTree = (TTree*) f->Get("gpsTree"); 
    if (fGpsTree==nullptr){
      fprintf(stderr,"Could not load gpsTree from file %s for run %d, giving up!\n", the_right_file, run); 
      fRunLoaded = false;
      return false;
    }
    fHaveGpsEvent = true; 
  } 
  else   // load gps file instead
  {
    TFile * gpsFile = TFile::Open(TString::Format("%s/run%d/gpsFile%d.root", data_dir, run, run));
    if (gpsFile->IsZombie() || gpsFile == nullptr) 
    {
      fprintf(stderr,"Could not find gps file for run %d from %s, giving up!\n", run, data_dir);
      fRunLoaded = false;
      return false; 
    }
    else
    {
      filesToClose.push_back(gpsFile);
      fGpsTree = (TTree*) gpsFile->Get("gpsTree"); 
      if (fGpsTree==nullptr){
        fprintf(stderr,"Could not load gpsTree from file %s for run %d, giving up!\n", gpsFile->GetName(), run); 
        fRunLoaded = false;
        return false;
      }
      fHaveGpsEvent = false; 
      fGpsTree->BuildIndex("realTime"); 
    }
  }

  fGpsTree->SetBranchAddress("gps",&fGps); 

  //try to load useful event file 
  std::vector<TString> possibleEventFiles;
  possibleEventFiles.reserve(3);
  possibleEventFiles.emplace_back(TString::Format("%s/run%d/usefulEventFile%d.root", data_dir, run, run));
  possibleEventFiles.emplace_back(TString::Format("%s/run%d/SimulatedEventFile%d.root", data_dir, run, run)); 
  possibleEventFiles.emplace_back(TString::Format("%s/run%d/SimulatedPueoEventFile%d.root", data_dir, run, run)); 
  if (const char * the_right_file = checkIfFilesExist(possibleEventFiles))
  {
    TFile * evtFile = TFile::Open(the_right_file); 
    if (evtFile->IsZombie())
    {
      fprintf(stderr,"Failed to load %s from %s", the_right_file, data_dir);
      fRunLoaded = false;
      return false; 
    }
    filesToClose.push_back(evtFile);
    fEventTree = (TTree*) evtFile->Get("eventTree"); 
    if (!fEventTree) 
    {
      fprintf(stderr,"Could not load event tree from file %s for run %d, giving up!\n", evtFile->GetName(), run); 
      fRunLoaded = false;
      return false;
    }
    fHaveUsefulFile = true; 
    fEventTree->SetBranchAddress("event",&fUsefulEvent); 
  }
  else 
  {
    TFile * eventFile = TFile::Open(TString::Format("%s/run%d/eventFile%d.root", data_dir, run, run)); 
    if (eventFile->IsZombie() || eventFile==nullptr)
    {
      fprintf(stderr,"Failed to load %s", the_right_file); 
      fRunLoaded = false;
      return false; 
    }
    filesToClose.push_back(eventFile); 
    fEventTree = (TTree*) eventFile->Get("eventTree"); 
    if (!fEventTree) 
    {
      fprintf(stderr,"Could not load event tree from file %s for run %d, giving up!\n", eventFile->GetName(), run); 
      fRunLoaded = false;
      return false;
    }
    fHaveUsefulFile = false; 
    fEventTree->SetBranchAddress("event",&fRawEvent); 
  }

  
  //try to load truth 
  if (simulated)
  {
    std::vector<TString> possibleTruths;
    possibleTruths.reserve(2);
    possibleTruths.emplace_back(TString::Format("%s/run%d/SimulatedTruthFile%d.root",data_dir,run,run));
    possibleTruths.emplace_back(TString::Format("%s/run%d/SimulatedPueoTruthFile%d.root",data_dir,run,run));
    if ( const char * the_right_file = checkIfFilesExist(possibleTruths) )
    {
      TFile * truthFile = TFile::Open(the_right_file); 
      if ( truthFile != nullptr && !truthFile->IsZombie())
      {
        filesToClose.push_back(truthFile); 
        fTruthTree = (TTree*) truthFile->Get("truthPueoTree"); 
        if (!fTruthTree) {
          fprintf(stderr, "Warning: found a truth file but couldn't load the truth tree!");
        } 
        else 
        {
          fTruthTree->SetBranchAddress("truth", &fTruth); 
        }
      }
    }
  }

  getEntry(0); //load the first entry 
  fRunLoaded = true;

  // change the ROOT gDirectory back (ie stop this function from changing the ROOT directory)
  // in case you book histograms or trees after instantiating AnitaDataset
  gDirectory->cd(theRootPwd); 
  
  return true; 
}

pueo::Dataset::~Dataset() 
{
  unloadRun(); 
  if (fHeader) delete fHeader;
  if (fUsefulEvent) delete fUsefulEvent;
  if (fRawEvent) delete fRawEvent;
  if (fGps) delete fGps; 
  if (fTruth) delete fTruth; 
  if (fCutList) delete fCutList;

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

void  pueo::Dataset::unloadRun() 
{
  for (auto f: filesToClose) 
  {
    f->Close(); 
    delete f; 
  }
  fHeadTree = nullptr; 
  fDecimatedHeadTree = nullptr; 
  fEventTree = nullptr; 
  fGpsTree = nullptr; 
  fRunLoaded = false;
  filesToClose.clear();

  if (fCutList) 
  {
    delete fCutList; 
    fCutList = nullptr;
  }  
}

void pueo::Dataset::zeroBlindPointers() 
{
  loadedBlindTrees = false;

  for(int pol=0; pol < pol::kNotAPol; pol++)
  {
    fBlindHeadTree[pol] = NULL;
    fBlindEventTree[pol] = NULL;
    fBlindHeader[pol] = NULL;
    fBlindEvent[pol] = NULL;
  }

  fBlindFile = NULL;
}

void pueo::Dataset::loadBlindTrees() 
{
  if(!loadedBlindTrees && version::get()==3){

    fBlindFile = NULL;

    char calibDir[FILENAME_MAX-64];
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
      strncpy(calibDir,calibEnv,sizeof(calibDir));
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
