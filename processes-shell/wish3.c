#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For system calls like execv and chdir
#include <string.h> // For string manipulation functions
#include <errno.h> // For error handling
#include <fcntl.h> // For file control options
#include <sys/stat.h> 
#include <sys/wait.h>

// define section 
#define _GNU_SOURCE
#define ERROR_MESSAGE_LENGTH 30 // Maximum length of an error message
#define MAX_TOKENS 64 // Maximum number of tokens in a command
#define MAX_PATHS 100 // Maximum number of paths in the path variable

// Define processIDs and processCount to keep track of processes
pid_t processIDs[MAX_PATHS]; // An array to store process IDs
int processCount = 0; // A counter to keep track of the number of processes

// Declare getPath function if not defined elsewhere
char *getPath(char *arg);
void userloop(); // Function to handle interactive mode
void batchloop(FILE *file); // Function to handle batch mode
int wishexit(char **args); // Function to handle the 'exit' command
int wishExecute(char **args); // Function to execute commands
int wishnumbuiltins(); // Function to get the number of built-in commands
char **tokenize(char *line, char *delim); // Function to tokenize a string
void changeDirectory(char *path); // Function to change the current directory
int wishPath(char **args); // Function to handle the 'path' command
int wishcd(char **args); // Function to handle the 'cd' command
int wishLaunch(char **args); // Function to launch external commands
char *concatPath(const char *path1, const char *path2); // Function to concatenate paths
void para2(char **args);
void parallelCommandExecute(char *line);
int validateArgs(char **args); // Function to validate command arguments
char *getPath(char *arg);
// Global variables
char *builtinstr[] = {"cd", "exit", "path"}; // Array of built-in commands
int (*builtinfunc[])(char **) = {&wishcd, &wishexit, &wishPath}; // Array of functions corresponding to built-in commands
char error_message[ERROR_MESSAGE_LENGTH] = "An error has occurred\n"; // Error message template
char *path[MAX_PATHS] = {"/usr/bin", "/bin"}; // initial paths available
int pathNull = 0; // Flag to indicate if path is empty
int paths = 2; // Initial number of paths

// main functions 
int main(int argc, char *argv[]) {
    FILE *file; // File pointer for batch mode

    if (argc > 2) {
        fprintf(stderr, "%s", error_message); // Print an error message if too many arguments are provided
        exit(EXIT_FAILURE); // Exit the program with a failure status
    } else {
        if (argc == 2) {
            file = fopen(argv[1], "r");
            if (file != NULL) {
                batchloop(file);
            } else {
                fprintf(stderr, "%s", error_message);
                exit(EXIT_FAILURE);
            }
        } else if (argc == 1)
            userloop();
    }

    return 0;
}

void userloop() {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1)
    {
        printf("wish> ");
        nread = getline(&line, &len, stdin);
        if (nread == -1)
        {
            exit(0);
        }
        else
        {
            wishExecute(tokenize(line, " \t\n"));
        }
    }
    free(line);
}

void batchloop(FILE *file) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    nread = getline(&line, &len, file);

    if (nread != -1)
    {
        do
        {
            if (strcmp(line, "exit") == 0 || strcmp(line, "exit\n") == 0)
            {
                exit(0);
            }

            wishExecute(tokenize(line, " \t\n"));


            nread = getline(&line, &len, file);
        } while (nread!= -1);
    }

    free(line);
    exit(0);
}

void para2(char **args) {
    pid_t pid;
    int paraCount = 0;

    // Count the number of parallel commands
    for (int i = 0; args[i] != NULL; i++) {
        if (*args[i] == '&') {
            paraCount++;
        }
    }

    // Initialize index to iterate over args array
    int index = 0;

    // Execute each parallel command
    while (index < paraCount) {
        // Execute the command starting from the current index
        pid = wishExecute(args + index);
        if (pid > 0) {
            // Skip to the next command by finding the next '&' symbol
            while (args[index] != NULL && *args[index] != '&') {
                index++;
            }
            if (args[index] != NULL) {
                index++; // Move past the '&' symbol
            }
        }
    }
}



void parallelCommandExecute(char *line) {
    char *commands[MAX_TOKENS]; // Adjusted the array size
    int nCommands = 0;
    char *command = strtok(line, "&");
    
    while (command != NULL && nCommands < MAX_TOKENS) {
        commands[nCommands++] = command;
        command = strtok(NULL, "&");
    }

    pid_t pids[MAX_TOKENS];

    for (int i = 0; i < nCommands; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // Child process
            char *args[MAX_TOKENS];
            int args_num = 0;
            char *token = strtok(commands[i], " ");
            while (token != NULL && args_num < MAX_TOKENS) {
                args[args_num++] = token;
                token = strtok(NULL, " ");
            }
            args[args_num] = NULL;
            wishExecute(args); // Call wishExecute with the command arguments
            exit(0);
        } else if (pids[i] < 0) {
            // Error handling for fork failure
            fprintf(stderr, "%s", error_message);
        }
    }

    // Parent process waits for child processes to finish
    for (int i = 0; i < nCommands; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

int wishcd(char **args) {
    // no path specified
    if(args[1] == NULL){
            fprintf(stderr, "%s", error_message);
    }
    else{
        if(chdir(args[1]) != 0){
            fprintf(stderr, "%s", error_message);
            return 1;
        }  
    }
    return 0;
}

void changeDirectory(char *path) {
    int rc = chdir(path);
    if (rc != 0)
    {
        fprintf(stderr, "%s", error_message);
    }
}

int wishexit(char **args) {
    int i = 1;
    while (args[i] != NULL) {
        fprintf(stderr, "%s", error_message);
        return 0;
        i++;
    }
    exit(0);
}


int wishExecute(char **args) {
    if (args[0] == NULL)
        return 1;

    // Check if the command is a built-in command
    for (int i = 0; i < wishnumbuiltins(); i++) {
        if (strcmp(args[0], builtinstr[i]) == 0) {
            return (*builtinfunc[i])(args); // Execute built-in command
        }  
    }

    // Check for the presence of '&' symbol in the command
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            //parallelCommandExecute(args[i - 1]); // Call parallelCommandExecute
        para2(args);
            return 0;
        }
    }

    // If '&' symbol not found, proceed with regular execution
    if (!pathNull) {
        return wishLaunch(args);
    } else {
        fprintf(stderr, "%s", error_message);
    }  
    
    for (int k=0; k<processCount; k++){
        int status;
        waitpid(processIDs[k], &status, 0);
    }
    return 0; 
}

int wishnumbuiltins() {
    return sizeof(builtinstr) / sizeof(char *);
}

char **tokenize(char *line, char *delim) { // passes tests 11 by splitting the string into tokens 
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    char *token;
    int index = 0;

    token = strtok(line, " \r\n\t");
    while (token != NULL)
    {
        // this is going to handle whitespace
        if ((*token == '\0') || (*token == '\n') || (*token == '\t')) {
            token = strtok(NULL, " \r\n\t");
            continue;
        }
        // this is gonna split the command
        if ((strchr(token, '>')) && (token[0] != '>')) {  
            char *subtoken;
            subtoken = strtok(token, ">");
            tokens[index] = subtoken;
            index++;
            subtoken = ">";
            tokens[index] = subtoken;
            index++;
            subtoken = strtok(NULL, ">");
            tokens[index] = subtoken;
            index++; 
        } else if ((strchr(token, '&')) && (token[0] != '&')) { 
            // will split the command from valid 
            char *subtoken;
            while ((subtoken = strtok(token, "&"))) {
                tokens[index] = subtoken;
                index++;
                subtoken = "&";
                tokens[index] = subtoken;
                index++;
                token = strtok(NULL, "&");
            }
            tokens[index - 1] = NULL;
            index--;
        }  else if ((strchr(token, '\"'))) { 
            char *quoteToken = malloc(1024 * sizeof(char*));
            quoteToken = strtok(token, "\"");
            if(strchr(token, '\"')){
                quoteToken = strtok(NULL, "\"");
            }else{
                quoteToken = strtok(NULL, "\"");
                token = strtok(NULL, "\"");
                strcat(quoteToken, " ");
                strcat(quoteToken, token);
            }
            // remove any new line character at the end
            strtok(quoteToken, "\n");
            // add the full quote token to the tokens for args
            tokens[index] = quoteToken;
            index++;
        } else {
            tokens[index] = token;
            index++;
        }
        token = strtok(NULL, " \r\n\t");
    }
    // this will end the array with a null
    tokens[index] = NULL;
    return tokens;
}

int wishPath(char **args) {
    if (args[1] == NULL)
    {
        pathNull = 1; // non built ins are working 
        return 0;
    }
    for (int i = 1; args[i] != NULL; i++)
    {
        if (args[i] != NULL)
        {
            char *tmp = (char *)malloc(200 * sizeof(char));
            getcwd(tmp, 200);
            if (!access(args[i], X_OK))
            {
            
                // add the current working directory so it's an absolute path
                path[paths] = concatPath(tmp, args[i]);
                paths++;
                // a path has been specified.
                pathNull = 0;
            }
        }
    }

    return 0;
}

int wishLaunch(char **args) {
    int i = 0;
    pid_t pid;
    
    //while (args[i] != NULL) {
        pid = fork();
        processIDs[processCount] = pid;
        processCount++;
        
        if (pid == 0) {
            // Child process
            if (validateArgs(args)) {
                fprintf(stderr, "%s", error_message);
                exit(EXIT_FAILURE);
            } else {
                //printf("\targ[0]: %s\n",args[0]);
                char *cmdpath = getPath(args[i]);
                if (*cmdpath != '\0') {
                    execv(cmdpath, args); //not a built in when you use exec 
                } else {
                    fprintf(stderr, "%s", error_message);
                    exit(EXIT_FAILURE);
                }
            }
        } else if (pid < 0) {
            fprintf(stderr, "%s", error_message);
        }
        
        i++;
    //}
    
    return pid;
}


char *concatPath(const char *path1, const char *path2) {
    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);

    // Allocate memory for the concatenated path
    char *result = malloc(len1 + len2 + 2); // Plus 2 for '/' and '\0'
    if (result == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Copy path1 and path2 into the result buffer
    strcpy(result, path1);
    strcat(result, "/");
    strcat(result, path2);

    return result;
}

void printcwd() {
    char *buf = (char *)malloc(100 * sizeof(char));
    int print = 1; // Set to 1 to ensure the loop runs at least once

    while (print) {
        getcwd(buf, 100);
        printf("cwd is: %s \n", buf);

        // Change the condition that terminates the loop
        // For example, press Ctrl+C or provide another condition to break the loop
        // Here, the loop will only execute once, but you can modify it according to your requirements
        print = 0; // Change this condition accordingly
    }

    free(buf);
}

char* getPath(char *s1) {
    char *s2;
    char *result = malloc(100 * sizeof(char));
    int i = 0;
    //printf("paths: %d\n", paths);
    while (i < paths) {
        s2 = path[i];
        result = concatPath(s2, s1);
        if (!access(result, X_OK)) {
            return result;
        }
        i++;
    }
    // Corrected to use getcwd instead of printcwd
    s2=(char *)malloc(100*sizeof(char));
    getcwd(s2,100);
    result = concatPath(s2, s1);
    if (!access(result, X_OK)) {
        return result;
    }
    result[0] = '\0'; // Setting the first character to null terminator
    return result;
}

int validateArgs(char **args) {
    int redirectCount = 0, i, fout;

    for (i = 0; args[i] != NULL; i++) {
        switch (*args[i]) {
            case '>':
                if (!i) {
                    // First arg is '>', which is an error
                    return 1;
                }
                if (args[i + 1] == NULL || args[i + 2] != NULL) {
                    // Nothing after redirect or too many things
                    // after redirect, so error
                    return 1;
                }
                redirectCount++;
                break;
            case '|':
                // Increment pipe count if '|' found

                break;
        }
    }

    if (redirectCount > 1) {
        // Too many redirects in single command, so error
        return 1;
    } else if (redirectCount == 1) {
        // Redirect to file instead of stdout
        fout = creat(args[i - 1], 0644); // Create or truncate file with read/write permissions
        if (fout < 0) {
            perror("creat"); // Print error if creat fails
            return 1;
        }
        // Redirect stdout to the file
        if (dup2(fout, STDOUT_FILENO) < 0) {
            perror("dup2"); // Print error if dup2 fails
            return 1;
        }
        close(fout); 
        args[i - 1] = NULL;
        args[i - 2] = NULL;
    }
    return 0;
}
