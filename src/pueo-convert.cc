#include "pueo/Converter.h"

#include <iostream>
#include <string.h>

void usage()
{

  std::cout << "Usage: pueo-convert [-f] [-t tmpsuf] [-P postprocessor args] typetag outfile.root input [input2]" <<std::endl;
  std::cout << "   -f   allow clobbering output " << std::endl;
  std::cout << "   -t   set a temporary file suffix " <<std::endl;
  std::cout << "   -P   post processor args (quote for multiple) " << std::endl;
  std::cout << "   typetag  typetag of input, or use auto to try to determine (problematic if more than one ROOT type can be generate from the same raw type)"<<std::endl;
  std::cout << "   outfile  name of output file " <<std::endl;
  std::cout << "   input    name(s) of input files or directories (note that directories are not recursive) " <<std::endl;
}

int main(int nargs, char ** args) 
{
  char * typetag = NULL;
  char * output = NULL;
  pueo::convert::ConvertOpts opts;

  std::vector<char *> inputs;
  for (int i = 1; i < nargs; i++)
  {
    if (!strcmp(args[i],"-f")) opts.clobber = true;
    else if (!strcmp(args[i],"-t"))
    {
      if (i == nargs - 1) 
      {
        usage(); 
        return 1;
      }
      opts.tmp_suffix = args[++i];
    }
    else if (!strcmp(args[i],"-P"))
    {
      if (i == nargs - 1)
      {
        usage();
        return 1;
      }
      opts.postprocess_args = args[++i];
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

