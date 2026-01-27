// Implementation of the core of the Dataset class (ie the constructor and its helpers)
#include "pueo/Dataset.h"
#include "pueo/Version.h"
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

const char * pueo::Dataset::checkIfFilesExist(const std::vector<const char *>& files)
{
  for (const char * f: files) 
  {
    if(checkIfFileExists(f)) return f;
  }
  return nullptr; 
}

bool  pueo::Dataset::loadRun(int run, DataDirectory dir, bool dec) 
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
  std::vector<const char *> possibleHeaders;
  possibleHeaders.reserve(5);
  possibleHeaders.emplace_back(Form("%s/run%d/eventHeadFile%d.root", data_dir, run, run));
  possibleHeaders.emplace_back(Form("%s/run%d/timedHeadFile%d.root", data_dir, run, run)); 
  possibleHeaders.emplace_back(Form("%s/run%d/headFile%d.root", data_dir, run, run)); 
  possibleHeaders.emplace_back(Form("%s/run%d/SimulatedHeadFile%d.root", data_dir, run, run));
  possibleHeaders.emplace_back(Form("%s/run%d/SimulatedPueoHeadFile%d.root", data_dir, run, run));

  bool simulated = false; 

  if (const char * the_right_file = checkIfFilesExist(possibleHeaders))
  {
    if (strcasestr(the_right_file,"Simulated")) simulated = true; 

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
  std::vector<const char *> possibleGps;
  possibleGps.reserve(5);
  possibleGps.emplace_back(Form("%s/run%d/gpsEvent%d.root", data_dir, run, run));
  possibleGps.emplace_back(Form("%s/run%d/SimulatedGpsFile%d.root", data_dir, run, run)); 
  possibleGps.emplace_back(Form("%s/run%d/SimulatedPueoGpsFile%d.root", data_dir, run, run));
  if (const char * the_right_file = checkIfFilesExist(possibleGps))
  {
     TFile * f = new TFile(the_right_file); 
     filesToClose.push_back(f); 
     fGpsTree = (TTree*) f->Get("gpsTree"); 
     fHaveGpsEvent = true; 
  } 
  else   // load gps file instead
  {
    TFile * gpsFile = TFile::Open(Form("%s/run%d/gpsFile%d.root", data_dir, run, run));
    if (gpsFile->IsZombie()) 
    {
      fprintf(stderr,"Could not find gps file for run %d, giving up!\n",run); 
      fRunLoaded = false;
      return false; 
    }
    else
    {
       filesToClose.push_back(gpsFile);
       fGpsTree = (TTree*) gpsFile->Get("gpsTree"); 
       fGpsTree->BuildIndex("realTime"); 
       fHaveGpsEvent = false; 
    }
  }

  fGpsTree->SetBranchAddress("gps",&fGps); 

  //try to load useful event file 
  std::vector<const char *> possibleEventFiles;
  possibleEventFiles.reserve(3);
  possibleEventFiles.emplace_back(Form("%s/run%d/usefulEventFile%d.root", data_dir, run, run));
  possibleEventFiles.emplace_back(Form("%s/run%d/SimulatedEventFile%d.root", data_dir, run, run)); 
  possibleEventFiles.emplace_back(Form("%s/run%d/SimulatedPueoEventFile%d.root", data_dir, run, run)); 
  if (const char * the_right_file = checkIfFilesExist(possibleEventFiles))
  {
     TFile * f = new TFile(the_right_file); 
     filesToClose.push_back(f); 
     fEventTree = (TTree*) f->Get("eventTree"); 
     fHaveUsefulFile = true; 
     fEventTree->SetBranchAddress("event",&fUsefulEvent); 
  }
  else 
  {
    TFile * eventFile = TFile::Open(Form("%s/run%d/eventFile%d.root", data_dir, run, run)); 
    if (! eventFile->IsZombie())
    {
       filesToClose.push_back(eventFile); 
       fEventTree = (TTree*) eventFile->Get("eventTree"); 
       fHaveUsefulFile = false; 
       fEventTree->SetBranchAddress("event",&fRawEvent); 
    }
  }

  if (!fEventTree) 
  {
    std::cerr << "WARNING: did not load an event tree for run " << run << " in " << data_dir << std::endl; 

  }
  
  //try to load truth 
  if (simulated)
  {
    std::vector<const char *> possibleSimulatedFiles;
    possibleSimulatedFiles.reserve(2);
    possibleSimulatedFiles.emplace_back(Form("%s/run%d/SimulatedTruthFile%d.root",data_dir,run,run));
    possibleSimulatedFiles.emplace_back(Form("%s/run%d/SimulatedPueoTruthFile%d.root",data_dir,run,run));
    if ( const char * the_right_file = checkIfFilesExist(possibleSimulatedFiles) )
    {
     TFile * f = new TFile(the_right_file); 
     filesToClose.push_back(f); 
     fTruthTree = (TTree*) f->Get("truthPueoTree"); 
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

namespace {
  const char  pueo_root_data_dir_env[]  = "PUEO_ROOT_DATA"; 
  const char  pueo_versioned_root_data_dir_env[]  = "PUEO%d_ROOT_DATA"; 
  const char  mc_root_data_dir[] = "PUEO_MC_DATA"; 
}
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
  
  if (version == 0) //if monte carlo
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
    if (const char *tryme = getenv(env_string)) 
    {
      return tryme;
    } 
    else 
    {
      fprintf(stderr,"%s, not defined, please define it!", pueo_root_data_dir_env); 
      return 0;
    }
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


