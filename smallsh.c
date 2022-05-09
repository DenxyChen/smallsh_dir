//  Name: Xiao Yu Chen
//  Class: CS344 Spring 2022
//  Date: 05/09/2022
//  Description: An implementation of a shell program in C.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // pid_t
#include <unistd.h> // fork

#define MAX_LENGTH 2048
#define MAX_ARG 512
#define TOK_DELIM " "

struct args
{
    int argc;
    char *argv[MAX_ARG];
};

// /* Get user input and tokenize the arguments. Store the input and output file names. */
// void getInput(char* arguments, char* inputFile, char* outputFile, int* background) {
    
//     char *buf;
//     size_t buflen;

//     printf(": ");
//     getline(&buf, &buflen, stdin);
//     printf("%s", buf);
// }

int execute_command(pid_t pid);
void get_input(pid_t pid, char *line);
struct args* parse_args(char *line);
void cd(char *path);

/* ============================================================
main() runs a loop that continuously gets a command from the user
while run_flag is true. When the user enters the "exit" command,
run_flag is set to false and the program exits.
============================================================ */
int main() {
    int run_flag = 1;
    pid_t pid = getpid();
    printf("pid is:    %d\n", pid);
    fflush(stdout);

    while (run_flag == 1) {
        printf(": ");
        fflush(stdout);
        run_flag = execute_command(pid);
    }

    return EXIT_SUCCESS;
}

/* ============================================================
execute_command() calls get_input() to read in the line with the
expansion variable accounted for. Then it calls parse_args() to 
parse the raw input into the args struct defined at the top of 
the file. Then it executes the command.

Recieves: pid -> pid_t
Returns: 1 (to continue main loop), 0 (to exit the program)
============================================================ */
int execute_command(pid_t pid) {

    char line[MAX_LENGTH] = {0};
    get_input(pid, line);

    struct args *cmd = parse_args(line);

    // Ignore empty lines and lines that start with #
    if ((cmd->argc == 0) | (line[0] == '#')){
        printf("\n");
        fflush(stdout);
        free(cmd);
        return 1;
    }

    printf("%d - ", cmd->argc);
    fflush(stdout);
    for (int i = 0; i < cmd->argc; i++) {
        printf("%s ", cmd->argv[i]);
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);

    // exit command with error handling if arguments are provided
    if (strcmp(cmd->argv[0], "exit") == 0) {
        if (cmd->argc == 1) {
            free(cmd);
            return 0;
        }
        else {
            fprintf(stderr, "exit: too many arguments\n");
            fflush(stderr);
        }
    }

    // cd command which changes dir to HOME if no arguments are provided
    else if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc == 1) {
            if (chdir(getenv("HOME")) == 0) {
                char cwd[MAX_LENGTH];
                getcwd(cwd, sizeof(cwd));
                printf("cd: %s\n", cwd);
                fflush(stdout);
            }
        }
        // Changes dir to the path passed as an argument if one is provided
        else if (cmd->argc == 2) {
            if (chdir(cmd->argv[1]) == 0) {
                char cwd[MAX_LENGTH];
                getcwd(cwd, sizeof(cwd));
                printf("cd: %s\n", cwd);
                fflush(stdout);
            }
            else {
                fprintf(stderr, "cd: %s: no such directory\n", cmd->argv[1]);
                fflush(stderr);
            }
        }
        else {
            fprintf(stderr, "cd: too many arguments\n");
            fflush(stderr);
        }
    }

    free(cmd);
    return 1;
}

/* ============================================================
get_input() reads in the line character by character. It replaces
all instances of the expansion variable $$ with the PID.

Recieves: pid -> pid_t
Returns: input -> string
============================================================ */
void get_input(pid_t pid, char *line) {
    // Parse PID to string from Ed Discussion post
    char *pidstr;
    {
        int n = snprintf(NULL, 0, "%d", pid);
        pidstr = malloc((n + 1) * sizeof *pidstr);
        sprintf(pidstr, "%d", pid);
    }

    int index = 0;
    int current, expansion_flag = 0;

    // Read stdin until the max char length is reached or a newline is encountered
    do{
        current = getchar();

        if (current == '$' && expansion_flag == 0) {
            expansion_flag = 1;
        }
        // If the current and previous char were both $, append pidstr
        else if (current == '$' && expansion_flag == 1) {
            for (size_t i = 0; i < strlen(pidstr); i++) {
                line[index] = pidstr[i];
                index += 1;
            }
            expansion_flag = 0;
        }
        else{
            // If the previous char was $, but the current is not, append $ before the current
            if (expansion_flag == 1){
                line[index] = '$';
                index += 1;
                expansion_flag = 0;
            }
            // Otherwise append the current char
            line[index] = current;
            index += 1;
        }
    } while(index < MAX_LENGTH && current != '\n');

    // Null-terminate string
    line[index - 1] = 0;
}

struct args* parse_args(char *line) {
    struct args *cmd = malloc(sizeof(struct args));
    char line_copy[MAX_LENGTH];
    strcpy(line_copy, line);
    cmd->argc = 0;

    char *token = strtok(line_copy, TOK_DELIM);
    while (token != NULL) {
        cmd->argv[cmd->argc] = token;
        cmd->argc += 1;
        token = strtok(NULL, TOK_DELIM);
    }

    return cmd;
}