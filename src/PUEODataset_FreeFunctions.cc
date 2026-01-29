// Implementation of the Dataset class's public static member methods, and their private helpers

#include "pueo/Dataset.h"
#include "TFile.h"
#include "TTree.h"
#include "TMutex.h"
#include "TEnv.h"
#include "TError.h"
#include <dirent.h>
#include <algorithm>

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

const char * pueo::Dataset::checkIfFilesExist(const std::vector<TString>& files)
{
  for (const TString& f: files) 
  {
    if(checkIfFileExists(f)) return f;
  }
  return nullptr; 
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

namespace {
  TMutex run_at_time_mutex; 
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
  std::vector<run_info> run_times[pueo::k::NUM_PUEO+1]; 
}
int pueo::Dataset::getRunAtTime(double t)
{

  int version= version::getVersionFromUnixTime(t); 

  if (run_times[version].empty())
  {
    TLockGuard lock(&run_at_time_mutex); 
    if (run_times[version].empty()) 
    {

      std::vector<TString> possiblePaths;
      possiblePaths.reserve(3);
      possiblePaths.emplace_back(TString::Format("%s/timerunmap_%d.txt", getenv("PUEO_CALIB_DIR"),version)); 
      possiblePaths.emplace_back(TString::Format("%s/share/pueoCalib/timerunmap_%d.txt", getenv("PUEO_UTIL_INSTALL_DIR"),version)); 
      possiblePaths.emplace_back(TString::Format("./calib/timerunmap_%d.txt",version)); 
      const char * cache_file_name = checkIfFilesExist(possiblePaths);
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

            std::vector<TString> possible_hfs;
            possible_hfs.reserve(2);
            possible_hfs.emplace_back(TString::Format("%s/run%d/timedHeadFile%d.root", data_dir, run, run));
            possible_hfs.emplace_back(TString::Format("%s/run%d/headFile%d.root", data_dir, run, run));

            if (const char * the_right_file = checkIfFilesExist(possible_hfs))
            {
              TFile f(the_right_file); 
              if (!f.IsZombie()){
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
