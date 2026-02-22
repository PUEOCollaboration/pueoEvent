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
#include "pueo/Nav.h"


#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <dirent.h>
#include <algorithm>
#include <sys/stat.h>
#include <cstdio>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <unordered_map>



#ifdef HAVE_PUEORAWDATA

#include "pueo/rawdata.h"
#include "pueo/rawio.h"

template <typename T> const char * getName() { return "unnamed"; } 
template <typename T> const char * getTreeName() { return "unnamedTree"; } 

#define NAME_TEMPLATE(TAG, RAW, ROOT, POST, ARITY) template <> const char * getName<ROOT>() { return #TAG; }
#define TREE_NAME_TEMPLATE(TAG, RAW, ROOT, POST, ARITY) template <> const char * getTreeName<ROOT>() { return #TAG "Tree"; }

PUEO_CONVERTIBLE_TYPES(NAME_TEMPLATE)
PUEO_CONVERTIBLE_TYPES(TREE_NAME_TEMPLATE)

static const char * getTagFromRawName(const char* raw_name)
{

  static std::unordered_map<const char *, const char *> table;
  static bool init = false;
  if (!init)
  {
#define RAWTABLE(TAG, RAW, ROOT, POST, ARITY) table[#RAW] = #TAG;
    PUEO_CONVERTIBLE_TYPES(RAWTABLE)

    init = true;
  }

  if (table.count(raw_name)) return table[raw_name];
  return nullptr;
}



template <typename RootType, typename RawType, int (*ReaderFn)(pueo_handle_t*, RawType*), pueo::convert::postprocess_fn PostProcess  = nullptr, bool Arity = false>
static int converterImpl(size_t N, const char ** infiles,  const char * outfile, const pueo::convert::ConvertOpts & opts  )
{

  std::string tmpfilename = outfile + std::string(opts.tmp_suffix);

  TFile outf(tmpfilename.c_str(), "RECREATE");
  outf.SetCompressionAlgorithm(opts.compression_algo);
  outf.SetCompressionLevel(opts.compression_level);

  if (!outf.IsOpen())
  {
    std::cerr <<"Couldn't open temporary output file " << tmpfilename << std::endl;
    return -1;
  }

  const char * typetag = getName<RootType>();
  const char * treename = getTreeName<RootType>();

  TTree * t = new TTree(treename, treename);
  t->SetAutoSave(0);
  RootType * R = new RootType();
  t->Branch(typetag, &R);
  RawType r;

  int nprocessed = 0;

  for (size_t i = 0; i < N; i++)
  {
    pueo_handle_t h;
    std::cout << "Processing file " << infiles[i] << std::endl;
    pueo_handle_init(&h, infiles[i], "r");


    while (ReaderFn(&h, &r) > 0)
    {
      if constexpr (Arity)
      {
        int num_items = pueo::convert::arity(&r);
        for (int j = 0; j < num_items; j++)
        {
          nprocessed++;
          R->~RootType();
          R = new (R) RootType(&r, j);
          t->Fill();
        }
      }
      else
      {

        nprocessed++;
        R->~RootType();
        R = new (R) RootType(&r);
        t->Fill();
      }
    }
    pueo_handle_close(&h);
  }

  bool out_of_sorts = false;
  std::vector<std::pair<size_t,double>> sorted;

  if (opts.sort_by)
  {
    //see if we are sorted or not

    size_t N = t->Draw(opts.sort_by,"","goff");
    for (size_t i = 1; i < N; i++)
    {
      if (t->GetV1()[i] < t->GetV1()[i-1]) 
      {
        out_of_sorts = true;
        break;
      }

    }

    if (out_of_sorts)
    {
      sorted.resize(N);

      for (size_t i = 0; i < N; i++)
      {
        sorted[i].first = i;
        sorted[i].second = t->GetV1()[i];
      }

      std::sort(sorted.begin(), sorted.end(),
          [](const auto & l, const auto & r) { return l.second < r.second; });

    }

  }


  outf.Write();

  if (opts.sort_by && out_of_sorts)
  {
    TFile fsorted(tmpfilename.c_str(),"RECREATE"); //will overwrite original temp file, but it will still exist until we close outf

    fsorted.SetCompressionAlgorithm(opts.compression_algo);
    fsorted.SetCompressionLevel(opts.compression_level);
    TTree * t_sorted = new TTree(treename, treename);
    t_sorted->SetAutoSave(0);
    t_sorted->Branch(typetag, &R);
    for (size_t i = 0; i < sorted.size(); i++)
    {
      t->GetEntry(sorted[i].first);
      t_sorted->Fill();
    }

    outf.Close();
    fsorted.Write();
    fsorted.Close();
  }
  else
  {
    outf.Close();
  }

  ::operator delete(R);

  if (PostProcess != nullptr)
  {
    if (!PostProcess(tmpfilename.c_str(), outfile, opts.postprocess_args))
    {
      unlink(tmpfilename.c_str());
    }
    else
    {
      std::cerr << "  postprocesser for " << getName<RootType>() << "  didn't return 0, leaving stray temp file" << std::endl;
      return -1;
    }
  }
  else
  {
    if (rename(tmpfilename.c_str(), outfile))
    {
      std::cerr << " rename returned non-zero " << std::endl;
      return -1;
    }
  }

  return nprocessed;
}


int pueo::convert::convertFiles(const char * typetag, int nfiles, const char ** infiles,  const char * outfile, const ConvertOpts & opts)
{

  if (!opts.clobber && !access(outfile,F_OK))
  {
    std::cerr << outfile << " already exists and we didn't enable clobber" <<std::endl;
    return -1;
  }


  if (nfiles == 0 || !infiles || !outfile)
  {
    return 0;
  }

  if (!typetag || !*typetag || !strcmp(typetag,"auto"))
  {
    // open the first file to figure it out, then close it as if nothing happened
    pueo_handle_t h;
    pueo_handle_init(&h, infiles[0], "r");
    pueo_packet_t * packet = NULL;
    if (pueo_ll_read_realloc(&h,&packet))
    {
      switch(packet->head.type)
      {
#define DISPATCH_TYPE(TAG, NAME)\
        case TAG:\
          typetag = getTagFromRawName(#NAME); break;
        PUEO_IO_DISPATCH_TABLE(DISPATCH_TYPE)

      }
    }
    else
    {
      std::cerr << "Failed to read packet from " << infiles[0] << std::endl;
    }

    free(packet);
    pueo_handle_close(&h);

  }

  //on second attempt this should be set...
  if (!typetag || !*typetag || !strcmp(typetag,"auto"))
  {
    return -1;
  }

#define CONVERT_TEMPLATE(TAG, RAW, ROOT, POST, ARITY)\
  else if (!strcmp(typetag,#TAG))\
  {\
    return converterImpl<ROOT,pueo_##RAW##_t,pueo_read_##RAW,POST, ARITY>(nfiles, infiles, outfile, opts);\
  }

  PUEO_CONVERTIBLE_TYPES(CONVERT_TEMPLATE)

  else
  {
    std::cerr <<"Unhandled typetag \"" << typetag << "\"" << std::endl;
    return -1;
  }
}


#else

int pueo::convert::convertFiles(const char * typetag, int nfiles, const char ** infiles,  const char * outfile, const ConvertOpts & opts)
{
  (void) typetag;
  (void) nfiles;
  (void) infiles;
  (void) outfile;
  (void) opts;
  std::cerr << "You need to compile with libpueorawdata support to convert files. Sorry." << std::endl;
  return -1;
}


#endif

static int convert_filter(const struct dirent * d)
{
  return d->d_name[0]!='.';
}


int pueo::convert::convertFilesOrDirectories(const char * typetag,  int N, const char** in, const char * outfile, const ConvertOpts & opts)
{
  std::vector<char *> files;
  files.reserve(N);

  for (int i_in = 0 ; i_in < N; i_in++)
  {
    struct stat st;
    if (stat(in[i_in],&st))
    {
      std::cerr <<"Skipping unstatable " << in[i_in] << std::endl;
      continue;
    }

    if ( (st.st_mode & S_IFMT) != S_IFDIR)
    {
      files.push_back(strdup(in[i_in]));
      continue;
    }

    DIR * dirp = opendir(in[i_in]);
    if (!dirp)
    {
      std::cerr << "is " << in[i_in] << " a directory?" <<std::endl;
      continue;
    }

    //loop over input directory
    struct dirent ** namelist;

    int dfd = dirfd(dirp);

    errno = 0;
    int  nin = scandirat(dfd, ".", &namelist, convert_filter  , alphasort);

    if (nin < 0)
    {
      std::cerr << strerror(nin) << std::endl;
      continue;
    }
    files.reserve(files.size() + nin);

    for (int i = 0; i < nin ; i++)
    {
      char * f= 0;
      asprintf(&f, "%s/%s", in[i_in], namelist[i]->d_name);
      files.push_back(f);
      free(namelist[i]);
    }

  }

  int ret = convertFiles(typetag, files.size(), (const char**) &files[0], outfile, opts);
  for (auto f : files) free(f);

  return ret;
}

