#ifndef _PUEO_EVENT_CONVERTER_H
#define _PUEO_EVENT_CONVERTER_H

/************************************************************
*  pueo/Converter.h           PUEO raw data convesion
*
*  Converter API. Main consumer will be a driver program but maybe someone will want to use 
*  stuff interactively.
*
*  Cosmin Deaconu <cozzyd@kicp.uchicago.edu
*
*  (C) 2027-, The Payload for Ultrahigh Energy Observations (PUEO) Collaboration
*
*  This file is part of pueoEvent, the ROOT I/O library for PUEO.
*
*  pueoEvent is free software: you can redistribute it and/or modify it under the
*  terms of the GNU General Public License as published by the Free Software
*  Foundation, either version 2 of the License, or (at your option) any later
*  version.
*
*  pueoEvent is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
*  A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with
*  pueoEvent. If not, see <https://www.gnu.org/licenses/
*
****************************************************************************/

#include <vector>


// X-macro defining the convertible types,
//
// arguments are:  (typetag, raw_type, root_type, postprocess_function)
//
// The typetag is used as the argument to the convert driver program, as well
// as the tree and branch name.
//
// The raw type should not have the pueo_ prefix and _t suffix (e.g. for
// pueo_full_waveform_t should just be full_waveform)
//
// The postprocess_function will be used to postprocess a generated ROOT file
// as an intermediate step (e.g. doing fixups that require the whole tree), if
// nullptr then the intermediate tmp tree is just renamed to the output. It
// takes arguments of input file, output file and a freeform argument. If NULL
// the temporary file will simply be moved to the final file.

#define CONVERTIBLE_TYPES(PUEO_CONVERT_TYPE)\
  PUEO_CONVERT_TYPE(waveform, full_waveforms, pueo::RawEvent, nullptr)\
  PUEO_CONVERT_TYPE(header, full_waveforms,pueo::RawHeader, pueo::convert::postprocess_headers)\
  PUEO_CONVERT_TYPE(attitude, nav_att, pueo::nav::Attitude, pueo::convert::postprocess_attitude)\



namespace pueo
{

  namespace convert
  {
    struct ConvertOpts
    {
      bool clobber = false;
      const char * tmp_suffix = ".tmp";
      const char * postprocess_args = nullptr;
    };

    // These are just shortcuts here
    namespace typetags
    {
      constexpr const char * AUTO = "auto";
      constexpr const char * HEADER = "header";
      constexpr const char * WAVEFORMS = "waveform";
      constexpr const char * ATTITUDE = "attitude";
    }

    /* Typedef for a postprocessor function when converting.
     * This has a very dumb (and flexible) interface taking the name of the input temporary ROOT file (with a TTree*) and the output
     * that we should hopefully create as well as a free-form argument string.
     *
     **/
    typedef int ( * postprocess_fn) (const char * infile, const char * outfile, const char * args);

    /** Convert input files to output file
     *
     * If typetag is NULL, empty or auto, the first packet from the first file will be read to determine the type. 
     * Note that this is fraught with peril as there is not necessarily a one-to-one mapping 
     * (e.g. full_waveforms_t can either generate RawEvents or RawHeaders and auto won't know which one you want!)
     *
     * We only allow one type per output file, not supporting heterogenous files 
     * (that could be done, but in practice is not that useful given how we wrote out the data and the fact that there is no one-to-one mapping)
     *
     * @param typetag the type tag, you can use one of the helper constants under convert::typetags, or pass empty, 
     *         NULL or "auto" to try try to determine by itself, but this will work poorly in cases there is not a one-to-one mapping.
     *
     * @param nfiles  the number of input files
     * @param infiles  array of input files
     * @param outfile The output file
     *
     */
    int convertFiles(const char * typetag, int nfiles, const char ** infiles,  const char * outfile, const ConvertOpts & opts = ConvertOpts());

    /** Similar to above, but an argument can be a directory instead of a file and in that case everything in the directory is added */
    int convertFilesOrDirectories(const char * typetag, int N, const char ** in,  const char * outfile, const ConvertOpts & opts = ConvertOpts());

    int postprocess_headers(const char * infile, const char * outfile, const char * args);
    int postprocess_attitudes(const char * infile, const char * outfile, const char * args);
  }
}



#endif
