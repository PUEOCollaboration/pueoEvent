// Implementation of the HiCal portion of the Dataset class.
// Created for organization purposes.
// Mayhap we should just have a HiCal class?
#include "pueo/Dataset.h"
#include "pueo/RawHeader.h"
#include "TTree.h"
#include "TFile.h"

// non-const global is potentially a bad idea
namespace {
  // std::vector<UInt_t> hiCalEventNumbers[pueo::k::NUM_PUEO+1]; // note: variable not used at all
  TFile* fHiCalGpsFile;
  TTree* fHiCalGpsTree;
  Double_t fHiCalLon;
  Double_t fHiCalLat;
  Double_t fHiCalAlt;
  Int_t fHiCalUnixTime;
}

void pueo::Dataset::hiCal(Double_t& longitude, Double_t& latitude, Double_t& altitude) {
  UInt_t realTime = fHeader ? fHeader->realTime : 0;
  hiCal(realTime, longitude, latitude, altitude);
}

void pueo::Dataset::hiCal(UInt_t realTime, Double_t& longitude, Double_t& latitude, Double_t& altitude) {
  loadHiCalGps();
  Long64_t entry = fHiCalGpsTree->GetEntryNumberWithIndex(realTime);

  if(entry > 0) {
    fHiCalGpsTree->GetEntry(entry);
    longitude = fHiCalLon;
    latitude = fHiCalLat;
    const double feetToMeters = 0.3048;
    altitude = fHiCalAlt*feetToMeters;
  } else {
    longitude = -9999;
    latitude = -9999;
    altitude = -9999;
  }
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
