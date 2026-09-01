/****************************************************************************************
*  DaqHsk.cc            Implementation of the PUEO Daqhsk classes
* 
*  Kevin McBride <kmcbride@uchicago.edu>
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

#include "pueo/DaqHsk.h" 
#ifdef HAVE_PUEORAWDATA

pueo::daqhsk::DaqHsk::DaqHsk(const pueo_daq_hsk_t *daqhsk) :
  l2_readout_time(daqhsk->l2_readout_time.utc_secs),
  l2_readout_timeNsecs(daqhsk->l2_readout_time.utc_nsecs),
  scaler_readout_time(daqhsk->scaler_readout_time.utc_secs),
  scaler_readout_timeNsecs(daqhsk->scaler_readout_time.utc_nsecs),
  soft_rate(daqhsk->soft_rate),
  pps_rate(daqhsk->pps_rate),
  ext_rate(daqhsk->ext_rate),
  MIE_total_H(daqhsk->MIE_total_H),
  MIE_total_V(daqhsk->MIE_total_V),
  LF_total_H(daqhsk->LF_total_H),
  LF_total_V(daqhsk->LF_total_V),
  aux_total(daqhsk->aux_total),
  global_total(daqhsk->global_total),
  //l2_enable_mask(daqhsk->l2_enable_mask),
  qwords_sent(daqhsk->qwords_sent),
  events_sent(daqhsk->events_sent),
  trigger_count(daqhsk->trigger_count),
  current_second(daqhsk->current_second),
  last_pps(daqhsk->last_pps),
  llast_pps(daqhsk->llast_pps),
  last_dead(daqhsk->last_dead),
  llast_dead(daqhsk->llast_dead),
  panic_count(daqhsk->panic_count),
  occupancy(daqhsk->occupancy),
  ack_count(daqhsk->ack_count),
  latency(daqhsk->latency),
  offset(daqhsk->offset),
  pps_trig_offset(daqhsk->pps_trig_offset)
{
  // Reset mask to zero before filling
  this->l2_enable_mask = daqhsk->l2_enable_mask;
  for (size_t i = 0; i < H_scalers.size(); ++i)
  {
    H_scalers[i] = daqhsk->Hscalers[i];
    V_scalers[i] = daqhsk->Vscalers[i];
    // the only time this is wrong is beginning of the flight. but we just check them all
    // if the bit is ==0 then the scaler better be >30 (unless of course it was falling and needed to get back to <15 to be set back to 1)
    // if the bit is ==1 then the scaler better be <30 (unless of course it was the reading we needed to see before setting it to 0 so we do that now)
    uint32_t h_bit = (1U << i);
    // Check if bit is 1 and scaler > 30 -> Clear bit (AND with bitwise NOT)
    if ((daqhsk->l2_enable_mask & h_bit) && (H_scalers[i] > 30)) {
        this->l2_enable_mask &= ~h_bit;
    } 
    // Check if bit is 0 and scaler < 15 -> Set bit (OR with bitmask)
    else if (!(daqhsk->l2_enable_mask & h_bit) && (H_scalers[i] < 15)) {
        this->l2_enable_mask |= h_bit;
    }
    uint32_t v_bit = (1U << (i + 12));
    // Check if bit is 1 and scaler > 30 -> Clear bit
    if ((daqhsk->l2_enable_mask & v_bit) && (V_scalers[i] > 30)) {
        this->l2_enable_mask &= ~v_bit;
    }
    // Check if bit is 0 and scaler < 15 -> Set bit
    else if (!(daqhsk->l2_enable_mask & v_bit) && (V_scalers[i] < 15)) {
        this->l2_enable_mask |= v_bit;
    }
  }

  for (size_t i = 0; i < turfio_L1_rate.size(); ++i)
    for (size_t j = 0; j < turfio_L1_rate[i].size(); ++j)
      turfio_L1_rate[i][j] = daqhsk->turfio_L1_rate[i][j];

  for (size_t i = 0; i < turfio_words_recv.size(); ++i)
    turfio_words_recv[i] = daqhsk->turfio_words_recv[i];

  for (size_t s = 0; s < Surfs.size(); ++s)
  {
    const auto &src_surf = daqhsk->surfs[s];
    auto &dst_surf = Surfs[s];

    dst_surf.readoutTime = src_surf.readout_time_start.utc_secs;
    dst_surf.readoutTimeNsecs = src_surf.readout_time_start.utc_nsecs;
    dst_surf.ms_elapsed = src_surf.ms_elapsed;
    dst_surf.surf_link = src_surf.surf_link;
    dst_surf.surf_slot = src_surf.surf_slot;

    for (size_t c = 0; c < dst_surf.agc_scale.size(); ++c)
    {
      dst_surf.agc_scale[c] = src_surf.agc_scale[c];
      dst_surf.agc_offset[c] = src_surf.agc_offset[c];
    }

    for (size_t b = 0; b < dst_surf.Beams.size(); ++b)
    {
      const auto &src_beam = src_surf.beams[b];
      auto &dst_beam = dst_surf.Beams[b];
      dst_beam.threshold = src_beam.threshold;
      dst_beam.pseudothreshold = src_beam.pseudothreshold;
      dst_beam.scaler = src_beam.scaler;
      dst_beam.pseudoscaler = src_beam.pseudoscaler;
      dst_beam.inMask = src_beam.in_mask;
      dst_beam.scalerBank = src_beam.scaler_bank;
    }
  }
}

#endif
