#ifndef __CLING__
// It appears that having this include line causes macro to crash
// (any other header is fine, it seems)
// Therefore, we will hide this line to the ROOT interpreter.
#include "pueo/Conventions.h"
#endif
#include <iostream>
void convention_test(){
  pueo::pol::pol_t myP = pueo::pol::fromChar('H');
  std::cout << "Pol is " << pueo::pol::asChar(myP) << std::endl;
  std::cout << "foobar\n";
}
