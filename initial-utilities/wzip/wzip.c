#define MAX_LENGTH 512
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  FILE* in = stdin;
  // FILE* out = stdout;
  char line[MAX_LENGTH];
  char value = 1;
  char extend = 1;
  int extendedCount = 0;
  int count = 0;

  if (argc == 1) {
    printf("wzip: file1 [file2 ...]\n");
    exit(1);
  }

  in = fopen(argv[1], "r");

  if (in != NULL) {
    while (fgets(line, sizeof(line), in) != NULL) {
      value = line[0];
      if (argc > 2) {extend = line[0];}
      for (int i = 0; line[i] != '\0'; i++) {
        count++;
        extendedCount++;
    
        if (line[i + 1] != value && argc <= 2) {
            fwrite(&count, 1, 4, stdout);
            fwrite(&value, 1, 1, stdout);
            value = line[i + 1];
            count = 0;
        }
      }
    }
  }

  for (int i = 2; i < argc; i++) {
    in = fopen(argv[i], "r");

    if (in != NULL) {
      while (fgets(line, sizeof(line), in) != NULL) {
        value = line[0];
        for (int k = 0; line[k] != '\0'; k++) {
          extendedCount++;
          if ((i + 1) == argc && line[k + 1] != value) {
              fwrite(&extendedCount, 1, 4, stdout);
              fwrite(&extend, 1, 1, stdout);
              value = line[k + 1];
              count = 0;
          }
        }
      }
    }
  }

  
}
