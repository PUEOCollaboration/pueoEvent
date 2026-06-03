#include "pueo/Dataset.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>


#include <sys/resource.h>

void print_mem_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // ru_maxrss is typically in kilobytes on Linux
        printf("Memory usage: %ld KB\n", usage.ru_maxrss);
    }
}

int main(int nargs, char ** args)
{
  int run = nargs > 1 ? atoi(args[1]) : 903;

  int num = 0;
  while (true)
  {
    print_mem_usage();
    printf("Instantiating Dataset round %d\n", num++);
    pueo::Dataset d(run);
    sleep(1);
  }

}





