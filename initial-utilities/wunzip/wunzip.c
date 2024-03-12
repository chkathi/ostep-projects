#define MAX_LENGTH 512
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  FILE* in = stdin;
  // char line[MAX_LENGTH];
  int count = 0;
  char value = 0;

  int extendedCount =0; 
  char extend = 0; 

  if (argc == 1) {
    printf("wunzip: file1 [file2 ...]\n");
    exit(1);
  }

  in = fopen(argv[1], "r");

  if (in != NULL) {  
    // EOF means that the returned value is -1
    while (fread(&count, sizeof(int), 1, in) == 1 && fread(&value, sizeof(char), 1, in) == 1) {
      for(int i = 0; i < count; i++) 
        printf("%c", value);
    }

    extend = value;
    extendedCount = count;
  }

  for (int i = 2; i < argc; i++) {
    in = fopen(argv[i], "r");

    if (in != NULL) {  
    // EOF means that the returned value is -1
      while (fread(&count, sizeof(int), 1, in) == 1 && fread(&value, sizeof(char), 1, in) == 1) {
        extendedCount += count;
        
        if ((i + 1) == argc) {
          extendedCount -= count;
          for(int i = 0; i < extendedCount; i++) 
            printf("%c", extend);
        }
      }
    }
  }
 
}
