#/************************************************************
*  pueo/Converter.cc           PUEO raw data convesion
*
*  Constants and such
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


#include "pueo/Converter.h"
#include "pueo/RawEvent.h"
#include "pueo/RawHeader.h"

#include <dirent.h>
#include <cstdio>
#include <errno.h>
#include <string.h>
#include <stdint.h>


#ifdef HAVE_LIBPUEORAWDATA

#include "pueo/rawdata.h"
#include "pueo/rawio.h"

template <typename T> const char * getName() { return "unnamed"; } 

#define NAME_TEMPLATE(TAG, RAW, ROOT, POST) template <> const char * getName<ROOT>() { return #TAG; }

CONVERTIBLE_TYPES(NAME_TEMPLATE)


template <typename RootType, typename RawType, int (*ReaderFn)(pueo_file_handle_t*, RawType*), pueo::convert::postprocess_fn postprocess  = nullptr>
static int converterImpl(size_t N, const char ** infiles,  char * outfile, bool clobber, const char * tmp_suffix)
{


  if (!clobber && access(outfile,F_OK))
  {
    std::cerr << outfile << " already exists and we didn't enable clobber" <<std::endl;
    return -1;
  }

  std::string tmpfilename = outfile + std::string(tmp_suffix);

  TFile outf(tmpfilename.c_str(), "RECREATE");

  if (!outf.IsOpen())
  {
    std::cerr <<"Couldn't open temporary output file " << tmpfilename << std::endl;
    return -1;
  }

  const char * typetag = getName<RootType>();

  TTree * t = new TTree(typetag, typetag);
  RootType * R = new RootType();
  t->Branch(typetag, &R);
  RawType r;

  int nprocessed = 0;

  for (int i = 0; i < N; i++)
  {
    pueo_handle_t h;
    pueo_handle_init(&h, infiles[i], "r");


    while (ReaderFn(&h, &r))
    {
      nprocessed++;
      R = new (R) RootType(&r);
      t->Fill();
    }
    pueo_handle_close(&h);
  }

  outf.Write();
  outf.Close();

  if (postprocess)
  {
    if (!postprocess(tmpfilename.c_str(), outfile))
    {
      unlink(tmpfilename.c_str());
    }
    else
    {
      std::cerr << "  postprocess didn't return 0, leaving stray temp file" << std::endl;
      return -1;
    }
  }
  else
  {
    return rename(tmfilename.c_str(), outfile);
  }

  return nprocessed;
}

#endif

