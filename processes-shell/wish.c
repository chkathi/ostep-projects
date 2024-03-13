#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Define Section */
#define MAXLINE 80
#define MAXARGS 20
#define MAX_PATH_LENGTH 50
#define TRUE 1
#define DELIM " \t\n"
#define _GNU_SOURCE

char error_message[30] = "An error has occurred\n";
char* shellPath[MAX_PATH_LENGTH] = {"/bin", "/usr/bin"};
int pathLocation = 2;
int onlyBuiltIn = 0;

// Batch mode require file input
void batchMode(FILE* fileName);
char* builtinCommand(char *command);
int checkAmpersand(int argc, char** argv);
int checkPath(char* currentPath);
char* concatStr(char *str1, char *str2);
void emptyArr(char** arr);
void errorMessage();
void interactiveLoop();
void parallelCommands(int argc, char** argv);
void printArgv(int argc, char **argv);
void printPaths();
void processExtCmd(int argc, char **argv);
void redirect(int argc, char*argv[]);
void runExtCmd(int argc, char** argv);
int splitLine(char* line, char** argv);

void wCat(int argc, char** argv);
void wCd(int argc, char** argv);
void wExit(int argc, char** argv);
void wPath(int argc, char** argv);

int main(int argc, char* argv[]) {
  FILE* file = stdin; 
  // set path to current working directory

  if (argc == 1){ 
    interactiveLoop();
    exit(0);
  } else{ // ./wish <fileName>
    file = fopen(argv[1],"r");

    struct stat stat_record;
    if(stat(argv[1], &stat_record)){
      fprintf(stderr, "%s", error_message);
      exit(1);
    } else if(stat_record.st_size <= 1){
      fprintf(stderr, "%s", error_message);
      exit(1);
    }

    if (file != NULL) {
      batchMode(file);
    }

    exit(0);
  } 
  
  errorMessage();
  exit(0); 
}

void batchMode(FILE* file) {
  char *line = NULL; // Initialize the pointer to NULL
  char* argv[MAXARGS];
  int argc = -1; 
  int parallel = -1; 
  size_t len = 0;
  ssize_t nread;

  while ((nread = getline(&line, &len, file)) != -1) {
    // printf("%s\n", line);
    argc = splitLine(line, argv);
    // printArgv(argc, argv);

    if (argc == -1)
      continue;

    if (strcmp(argv[0], "exit") == 0) {
      free(line);
      wExit(argc, argv);
      exit(0);
    } else if (strcmp(argv[0], "cd") == 0 || strcmp(line, "cd\n") == 0) {
      wCd(argc, argv);
    } else if (strcmp(argv[0], "path") == 0 || strcmp(argv[0], "path\n") == 0) {
      wPath(argc, argv);

      if (argc > 1)
        onlyBuiltIn = 0; 
    } else {
      // Check for parallel and if parallel == 0 then empty parallel 
      // skip processExtCmd and redirect 
      if (argc == 0) continue; 

      parallel = checkAmpersand(argc, argv);
      
      if (parallel == 1)
        parallelCommands(argc, argv);
      
      if (parallel == 0) {
        redirect(argc, argv);
        processExtCmd(argc, argv);
      }
    }

    free(line);

    line = NULL;
    len = 0;
  }

  return;
}

char* builtinCommand(char* command){
  if (strcmp(command, "path") == 0 || strcmp(command, "cd") == 0 || strcmp(command, "exit") == 0)
    return command;

  return "not-builtIn";
}

int checkAmpersand(int argc, char** argv) {
  int isAmp = 0;
    
  for (int i = 0; i < argc; i++) {
    if(strcmp(argv[i], "&") == 0) {
      if (i == 0) return -1;

      isAmp = 1;
    }
  }

  return isAmp;
}

int checkPath(char* enteredPath) {
  for (int i = 0; i < pathLocation; i++) {
    if (strcmp(shellPath[i], enteredPath) == 0) return i;
  }

  return -1; 
}

char* concatStr(char* str1, char* str2) {
    char *result;
    if(str1 == NULL ){
        result  = malloc(strlen(str2) + 1); 
        strcpy(result, str2);
        return result;
    }else if (str2 ==NULL){
        result  = malloc(strlen(str1) + 1); 
        strcpy(result, str1);
        return result;
    }
    result  = malloc(strlen(str1) + strlen(str2) + 1); 
    
    // in real code you would check for errors in malloc here
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

void emptyArr(char** arr){
  for (int i = 0; i < MAXARGS; ++i) {
    arr[i] = NULL;
  }
}

void errorMessage(){
  char error_message[30] = "An error has occurred\n";
  fprintf(stderr, "%s", error_message);
  exit(0);
}

void interactiveLoop(){}

void parallelCommands(int argc, char** argv) {
  char* command[MAXARGS];
  int commandArgs = 0; 

  for (int i = 0; i < argc; i++) {
    if(strcmp(argv[i], "&") == 0) {

      // printArgv(commandArgs, command);
      command[commandArgs] = NULL;
      redirect(commandArgs, command);
      processExtCmd(commandArgs, command);

      emptyArr(command);
      commandArgs = 0;

      continue;
    }

    command[commandArgs++] = argv[i];
    // printf("commandArgs: %d, command: %s\n",commandArgs ,command[commandArgs - 1]);
  }

  if (commandArgs > 0) {
    command[commandArgs] = NULL;
    redirect(commandArgs, command);
    processExtCmd(commandArgs, command);
  }

  return; 
}

void printArgv(int argc, char** argv) {
  printf("\nArgc: %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("%s, ", argv[i]);
  }

  printf("\n");
}

void printPaths() {
  for (int i = 0; shellPath[i] != NULL; i++) {
    printf("%s, ", shellPath[i]);
  }

  printf("\n");
}

void processExtCmd(int argc, char **argv){
  int status;
  pid_t pid;

  pid = fork();

  switch(pid){
    case -1:
      errorMessage();
    case 0:
      /* I am child process.
       I will execute the command, call: execvp */
      runExtCmd(argc, argv);
      break;

    default:
      /* I am parent process */
      if (wait(&status) == -1){
        errorMessage();
      } else {
        // printf("Child returned status: %d\n\n", status);
      }

      break;
    }  /* end of the switch */

  return;
}   

void redirect(int argc, char** argv){
  int i;	     // loop counter
  int out = 0;  // track position of location Out redirection, >
  int fd;

  for (i = 0; i < argc; i++) {
    if (strcmp(argv[i], ">") == 0) {
      if (out != 0) {
        // User entered "ls > lsout > file1" 
        errorMessage(); 
      } else if (i == 0) {
        // User entered "> lsout"
        errorMessage();
      } 

      // User entered ls file1 > file2
      // set out to the current loop_counter
      out = i;     
    } 
  }

  if (out != 0) {
    if (argv[out + 1] == NULL || argv[out + 2] != NULL){
      // User entered "ls > NULL" or ls > file file
      errorMessage();
    }
        
    // Open the file using name from argv (create file if needed)
    fd = open(argv[out + 1], O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        
    // Error check the open
    if (fd == -1) {
      errorMessage();
    }

    // // Switch standard-out to the value of file descriptor 
    // dup2(fd, STDOUT_FILENO);
    // close(fd);
        
    argv[out] = NULL;
  }    
} 

void runExtCmd(int argc, char** argv) {
  int ret;
  char *cmd_path;

  // we check all path and concatStr(path[i],argv[0])
  for (int i = 0; shellPath[i] != NULL ; i++) {
    // printf("shellPath: %s\n", shellPath[i]);

    cmd_path = shellPath[i];
    cmd_path = concatStr(cmd_path, "/");
    cmd_path = concatStr(cmd_path, argv[0]);
    // printf("shellPath: %s\n", shellPath[i]);

    // printPaths();

    if (access(cmd_path, X_OK) == 0 && onlyBuiltIn == 0) {
      ret = execv(cmd_path, argv);
      
      if (ret == -1)
        errorMessage();
    } else if (onlyBuiltIn == -1) {
      errorMessage();
    }
  }

  errorMessage();
  return;
}

int splitLine(char *line, char** argv ){
    int argc = 0;
    char *separator = " \n\t";
    char *token;
    char *subtoken;
    
    while ((token = strsep(&line, separator)) != NULL && (argc + 1 < MAXARGS)) {
      // Variable whitespace & Empty Commands
      if((*token == '\0') || (*token == '\n') || (*token == '\t')) {
        continue;
      }
      
      // printf("token: %s\n", token);

      if ((strchr(token, '>')) != NULL && (token[0] != '>')) {
        // found > within the token. Now break it up into parts
        subtoken = strsep(&token, ">");
        argv[argc++] = subtoken;
        argv[argc++] = concatStr(">", NULL);
        subtoken = strsep(&token, ">");
        argv[argc++] = subtoken;
      } else if ((strchr(token, '&')) != NULL && (token[0] != '&')) {
        while((subtoken = strsep(&token, "&"))){
          argv[argc++] = subtoken;
          subtoken = concatStr("&", NULL);
          argv[argc++] = subtoken;
        }
      } else {
        argv[argc++] = token;
      }
    }

    if (argc > 0)
      argv[argc] = NULL;
    
    // printArgv(argc, argv);

    return argc;
}
void wCat(int argc, char** argv) {
  FILE* in = stdin;
  char line[512];

  // just ./wcat entered then return 0
  if (argc == 1) {}

  for (int i = 1; i < argc; i++) {
    // output if not found (wcat: cannot open file)
    in = fopen(argv[i],"r");

    if (in == NULL) {}

    while (fgets(line, sizeof(line), in) != NULL) {
      printf("%s", line);
    }
  }

  fclose(in);
}

void wCd(int argc, char** argv) {
  char* dir = NULL;
  int ret = 0; 

  if (argc == 1 || argc > 2) {
    errorMessage();
  }
  
  dir = argv[1];

  if (access(dir, X_OK) == 0)
    ret = chdir(dir);

  else
    errorMessage();

  if (ret != 0) {
    errorMessage();
  }

  return;
}

void wExit(int argc, char** argv) {
  if (argc > 1) {
    errorMessage();
  }

  // Hello World

  exit(0);
}

void wPath(int argc, char** argv) {
  // int isPath = 0;
  if (argc == 1) {
    onlyBuiltIn = -1;
    return;
  } else {
    for (int i = 1; i < argc; i++) {
      if (argv[i] != NULL) {
        char *currentPath = (char *)malloc(200*sizeof(char));
        // isPath = checkPath(argv[i]);

        // if (isPath > 0) return; 

        getcwd(currentPath,200);
        strcat(currentPath, "/");
        strcat(currentPath, argv[i]);

        // printf("Current Path to be added: %s\n", currentPath);
        if (access(currentPath, X_OK) == 0) {
          shellPath[pathLocation] = currentPath;
          // printf("added: %s\n", shellPath[pathLocation]);
          pathLocation++;   
        } 
      }
    }
  }

  // printPaths();
}

// tests 22 checks if everything works serially
// Problem?
