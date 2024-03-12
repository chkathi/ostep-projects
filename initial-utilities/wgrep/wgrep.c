#define MAX_LENGTH 512
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  // define string and file pointer
  FILE* in = stdin;
  char line[MAX_LENGTH];

  if (argc == 1) {
    printf("wgrep: searchterm [file ...]\n");
    exit(1);
  }

  // ./wcat [stringToFind]
  if (argc == 2) {
    if (in != NULL) {
      while (fgets(line, sizeof(line), in) != NULL) {
        if (strstr(line, argv[1])) {
          printf("%s", line);
        }
      }
    }

    fclose(in);
    return 0;
  }

  // argc has 3 arguments 
  if (argc == 3) {
    in = fopen(argv[2], "r");

    if (in == NULL) {
      printf("wgrep: cannot open file\n");
      exit(1);
    }

    while (fgets(line, sizeof(line), in) != NULL) {
      if (strstr(line, argv[1])) {
        printf("%s", line);
      }
    }

    fclose(in);
  }
}


