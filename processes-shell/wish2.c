// only passes tests 1,2,4

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define ERROR_MESSAGE_LENGTH 30
#define MAX_TOKENS 64

void userloop();
void batchloop(FILE *file);
int wishexit(char **args);
int wishExecute(char **args);
int wishnumbuiltins();
char **tokenize(char *line, char *delim);
void changeDirectory(char *path);
void displayError();
int wishpath(char **args); // Declaration of wishpath
int wishcd(char **args);   // Declaration of wishcd
int wishLaunch(char **args, int redirect, int fval); // Declaration of wishLaunch

// Global variables
char *builtinstr[] = {"cd", "exit", "path"};
int (*builtinfunc[])(char **) = {&wishcd, &wishexit, &wishpath}; // Fix here
char error_message[ERROR_MESSAGE_LENGTH] = "An error has occurred\n";
char *path = "/bin";

int main(int argc, char *argv[])
{
    FILE *file;
    if (argc > 2)
    {
        fprintf(stderr, "%s", error_message);
        exit(EXIT_FAILURE);
    }
    else
    {
        if (argc == 2) {
            file = fopen(argv[1], "r");
            if (file != NULL) {
                batchloop(file);
            }
        } else if (argc == 1)
          userloop();
    }
}

// Rest of the code remains the same...

void userloop()
{
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

void batchloop(FILE *file)
{
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
        } while (nread != -1);
    }

    free(line);
    exit(0);
}


int wishcd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "%s", error_message);
    }
    else
    {
        changeDirectory(args[1]);
    }
    return 0;
}

void changeDirectory(char *path)
{
    int rc = chdir(path);
    if (rc != 0)
    {
        fprintf(stderr, "%s", error_message);
    }
}

int wishexit(char **args)
{
    return 0;
}

int wishExecute(char **args)
{
    if (args[0] == NULL)
        return 1;

    // Check if the command is a built-in command
    for (int i = 0; i < wishnumbuiltins(); i++)
    {
        if (strcmp(args[0], builtinstr[i]) == 0)
        {
            return (*builtinfunc[i])(args); // Execute built-in command
        }
    }

    // If the command is not a built-in command, execute it
    char binPath[256]; // Assuming max path length of 256
    sprintf(binPath, "/bin/%s", args[0]); // Prepend /bin/ to the command name

    // Execute the command
    if (execv(binPath, args) == -1)
    {
        perror("execv"); // Print error if execv fails
        return 1;
    }

    // This line will only be reached if execv fails
    return 1;
}

int wishnumbuiltins()
{
    return sizeof(builtinstr) / sizeof(char *);
}

char **tokenize(char *line, char *delim)
{
    char **tokens = malloc(64 * sizeof(char *));
    char *token;
    int index = 0;

    token = strtok(line, delim);
    while (token != NULL)
    {
        tokens[index] = token;
        index++;
        token = strtok(NULL, delim);
    }
    tokens[index] = NULL;
    return tokens;
}

int wishpath(char **args)
{
    return 0;
}

void displayError()
{
    fprintf(stderr, "%s", error_message);
}

int wishLaunch(char **args, int redirect, int fval)
{
    pid_t pid;
    int status;
    int fout;
    char *cleanArgs[redirect + 1];

    pid = fork();
    if (pid == 0)
    {
        // Child process
        /* Child process: stdout redirection */
        if (redirect != 0)
        {
            fout = creat(args[fval], 0644);
            //printf("fd_out is: %d\n", fout);
            //close(1);
            //dup(fout);
            dup2(fout, STDOUT_FILENO); // redirect stdout to file
            close(fout);
            int i;
            for (i = 0; i < redirect; i++)
            {
                cleanArgs[i] = args[i];
            }
            cleanArgs[i] = NULL;
            if (execvp(cleanArgs[0], cleanArgs) == -1)
            {
                //perror("lsh");
                //printf("execvp has an issue...\n");
            }
        }
        else
        {
            if (execvp(args[0], args) == -1)
            {
                //perror("lsh");
                //printf("execvp has an issue...\n");
            }
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
      
    }
    else
    {
        // Parent process
        //printf("pid is %d\n", pid);
        do
        {
            //wpid = waitpid(pid, &status, WUNTRACED);
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return pid;
}