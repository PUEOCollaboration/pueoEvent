#include "pueo/Converter.h"

#include <iostream>
#include <vector>
#include <string.h>

void usage()
{

  std::cout << "Usage: pueo-convert [-f] [-t tmpsuf] [-s sortby] [-P postprocessor args] typetag outfile.root input [input2]                                 \n"
               "   -f   allow clobbering output                                                                                                              \n"
               "   -t   set a temporary file suffix                                                                                                          \n"
               "   -s   sort by an expression (quotes for complex expression, anything that goes in TTree::Draw and produces a double will work).            \n"
               "        Mostly useful for telemetered data. A useful expression may be \"run*1e9+event\".                                                    \n"
               "   -P   post processor args (quote for multiple)                                                                                             \n"
               "   typetag  typetag of input, or use auto to try to determine (problematic if more than one ROOT type can be generate from the same raw type)\n"
               "   outfile  name of output file                                                                                                              \n"
               "   input    name(s) of input files or directories. Note that directories are not recursive.                                                  \n"
               "            So, as an example, converting all timemarks (file structure timemarks/<year>_<month>_<day>/*.timemark.dat) into one root file,   \n"
               "            one would have to pass `/path/to/timemark/*` instead of `/path/to/timemark/`                                                   \n\n" 

               "            As another example: on the other hand, converting one day's worth of timemarks,                                                  \n"
               "            simply pass the directory path `/path/to/timemark/<year>_<month>_<day>/`                                                         \n"
    << std::endl;
}

int main(int nargs, char ** args) 
{
  char * typetag = NULL;
  char * output = NULL;
  pueo::convert::ConvertOpts opts;

  std::vector<char *> inputs;
#define CHECK_NOT_LAST if (i == nargs -1) { usage(); return 1; }
  for (int i = 1; i < nargs; i++)
  {
    if (!strcmp(args[i],"-f")) opts.clobber = true;
    else if (!strcmp(args[i],"-t"))
    {
      CHECK_NOT_LAST
      opts.tmp_suffix = args[++i];
    }
    else if (!strcmp(args[i],"-P"))
    {
      CHECK_NOT_LAST
      opts.postprocess_args = args[++i];
    }
    else if (!strcmp(args[i],"-s"))
    {
      CHECK_NOT_LAST
      opts.sort_by = args[++i];
    }
    else if (!typetag)
    {
      typetag = args[i];
      std::cout << "typetag: " << typetag << std::endl;
    }
    else if (!output)
    {
      output = args[i];
      std::cout << "output: " << output << std::endl;
    }
    else
    {
      std::cout << args[i] << std::endl;
      inputs.push_back(args[i]);
    }

  }

  if (!typetag || !output || !inputs.size())
  {
      usage();
      return 1;
  }


  int Nproc = pueo::convert::convertFilesOrDirectories(typetag,
        inputs.size(), (const char **) &inputs[0],
        output, opts);

  if (Nproc < 0)
  {
    std::cerr << "Something bad happened" << std::endl;
    return 1;
  }
  else
  {
    std::cout << "Processed " << Nproc << " files" << std::endl;
  }


  return 0;

}

