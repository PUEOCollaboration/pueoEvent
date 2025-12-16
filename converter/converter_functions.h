#ifndef CONVERTER_FUNCS
#define CONVERTER_FUNCS

#include <vector>
#include <filesystem>
#include<algorithm>
#include "pueo/RawEvent.h"
extern "C" {
  #include "pueo/rawio.h"
  #include "pueo/rawdata.h"
}

namespace converter {

const int surf_mapping[28] = {5, 4, 3, 2, 1, 0, 25, 6, 7, 8, 9, 10, 11, 24, 18, 19, 20, 21, 22, 23, 26, 17, 16, 15, 14, 13, 12, 26}; 
const int channel_mappings[8] = {4, 5, 6, 7, 0, 1, 2, 3};


struct id_pair {
    int run;
    int event;
  };

/**
 * Check if the combination of run ID and event ID is already present in the vector
**/
bool has_event_ids(const std::vector<id_pair> *vec, int run, int event) {
  for (unsigned int i=0; i<vec->size(); i++) {
    if ((vec->at(i).run == run)&&(vec->at(i).event == event)) {
      return true;
    }
  }
  return false;
}

/**
 * Goes through all files in the directory and produces a list of the run IDs and event IDs
 * in the data files.
 */
void list_ids(
  const std::filesystem::path *directory, 
  std::vector<id_pair> *events, 
  std::vector<int> *run_ids
) {
  pueo_handle_t pueo_handle;
  pueo_single_waveform_t waveform = {};
  for (auto const &direntry : std::filesystem::directory_iterator{*directory}) {
    pueo_handle_init(&pueo_handle, direntry.path().string().c_str(), "r");
    int read = pueo_read_single_waveform(&pueo_handle, &waveform);
    if (!has_event_ids(events, waveform.run, waveform.event)) {
      events->push_back(id_pair());
      events->at(events->size()-1).run = waveform.run;
      events->at(events->size()-1).event = waveform.event;
      if (std::count(run_ids->begin(), run_ids->end(), waveform.run) == 0) {
        run_ids->push_back(waveform.run);
      }
    }
    pueo_handle_close(&pueo_handle);
  }
}

/**
 * Returns a sorted list of all event IDs inside the vector that belong to a given run.
 */
std::vector<int> get_events_in_run(
  const std::vector<id_pair> *id_pairs,
  int run_id
) {
  std::vector<int> ids;
  for (unsigned int i_id=0; i_id<id_pairs->size(); i_id++) {
    if ((id_pairs->at(i_id).run == run_id) && (count(ids.begin(), ids.end(), id_pairs->at(i_id).event)==0)) {
      ids.push_back(id_pairs->at(i_id).event);
    }
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

/**
 * Sets up a vector of pueo::rawEvents objects to write the waveform data into for each event in the run
 */
void prepare_output_events(
  std::vector<pueo::RawEvent*> *output_events,
  const std::vector<int> *event_ids,
  int run_id
) {
  for (unsigned int i=0; i < event_ids->size(); i++) {
    output_events->push_back(new pueo::RawEvent());
    output_events->at(i)->runNumber = run_id;
    output_events->at(i)->eventNumber = event_ids->at(i);
    for (unsigned int i_ch=0; i_ch<208; i_ch++) {
      output_events->at(i)->data[i_ch].resize(1024);
      for (unsigned int j=0; j<1024; j++) {
        output_events->at(i)->data[i_ch][j] = 0;
      }
    }
  }
}

/**
 * Goes through all raw data files and writes the waveforms into the corresponding output events. 
 */
void fill_output_events(
  std::vector<pueo::RawEvent*> *output_events,
  std::vector<pueo::RawHeader*> *output_headers,
  const std::filesystem::path *directory,
  const std::vector<int> *event_numbers,
  int run_id
) {
  pueo_handle_t pueo_handle;
  pueo_single_waveform_t waveform = {};

  for (auto const &direntry : std::filesystem::directory_iterator{*directory}) {
    pueo_handle_init(&pueo_handle, direntry.path().string().c_str(), "r");
    int read = pueo_read_single_waveform(&pueo_handle, &waveform);
    if ((waveform.run == run_id) && (waveform.wf.length > 0)) {

      int output_event_index = std::find(event_numbers->begin(), event_numbers->end(), waveform.event) - event_numbers->begin();
    
      int i_surf = surf_mapping[waveform.wf.channel_id / 8];
      if (i_surf > 25) {
        pueo_handle_close(&pueo_handle);
        continue;
      }
      int i_surf_channel = channel_mappings[waveform.wf.channel_id % 8];
      int i_channel = i_surf * 8 + i_surf_channel;
      for (unsigned int i_sample=0; i_sample<std::min(waveform.wf.length,(u_int16_t) 1024); i_sample++) {
        output_events->at(output_event_index)->data[i_channel][i_sample] = waveform.wf.data[i_sample];        
      }
      //TODO: Fill event header
    }
    pueo_handle_close(&pueo_handle);
  }
}

void prepare_output_headers(
  std::vector<pueo::RawHeader*> *output_headers,
  const std::vector<int> *event_ids,
  int run_id
) {
  for (unsigned int i=0; i<event_ids->size(); i++) {
    output_headers->push_back(new pueo::RawHeader());
    output_headers->at(i)->run = run_id;
    output_headers->at(i)->eventNumber = event_ids->at(i);
  }
}
}


#endif