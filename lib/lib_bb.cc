#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int bc[2];

// Update branch information.
// A conditional branch is taken if <taken> is true.
extern "C" __attribute__((visibility("default")))
void updateBranchInfo(bool taken) {
  if (taken) {
    bc[0] += 1;
  }
  bc[1] += 1;
}

extern "C" __attribute__((visibility("default")))
void printOutBranchInfo() {
  fprintf(stderr, "taken\t%d\n", bc[0]);
  fprintf(stderr, "total\t%d\n", bc[1]);

  bc[0] = bc[1] = 0;
}
