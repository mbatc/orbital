#include "app/Orbital.h"

int main(int argc, char ** argv) {
  Orbital orbital;

  if (!orbital.parseCommandLine(argc, argv)) {
    return 1;
  }

  if (!orbital.init()) {
    return 1;
  }

  int result = orbital.run();
  orbital.shutdown();
  return result;
}
