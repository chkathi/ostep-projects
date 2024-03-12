#define MAX_STRING 512
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
  FILE* in = stdin;
  char line[MAX_STRING];

  // just ./wcat entered then return 0
  if (argc == 1) {
    return 0;
  }

  for (int i = 1; i < argc; i++) {
    // output if not found (wcat: cannot open file)
    in = fopen(argv[i],"r");

    if (in == NULL) {
      printf("wcat: cannot open file\n");
      exit(1);
    }

    while (fgets(line, sizeof(line), in) != NULL) {
      printf("%s", line);
    }
  }

  fclose(in);
}