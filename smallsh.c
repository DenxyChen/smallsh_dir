//  Name: Xiao Yu Chen
//  Class: CS344 Spring 2022
//  Date: 05/09/2022
//  Description: An implementation of a shell program (command-line interface) in C.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // pid_t
#include <unistd.h> // fork

#define MAX_LENGTH 2048
#define MAX_ARG 512
#define DELIM " "

struct args
{
    int argc;
    char *argv[MAX_ARG];
    char *input_file;
    char *output_file;
    int background_flag;
};

int run_command(pid_t pid);
void get_input(pid_t pid, char *line);
struct args* parse_args(char *line);
void cd(char *path);

/* ============================================================
main() runs a loop that continuously calls run_command() 
to get a line of input from the user and executes it while
run_flag is true. If the "exit" command is given, run_command()
sets run_flag to false which breaks the loop and exits the program.
============================================================ */
int main() {
    int run_flag = 1;
    pid_t pid = getpid();
    printf("pid is:    %d\n", pid);
    fflush(stdout);

    while (run_flag == 1) {
        printf(": ");
        fflush(stdout);
        run_flag = run_command(pid);
    }

    return EXIT_SUCCESS;
}

/* ============================================================
run_command() calls get_input() to read in the line with the
expansion variable accounted for. Then it calls parse_args() to 
parse the raw input into the args struct defined at the top of 
the file. Then it executes the command.

Recieves: pid -> pid_t
Returns: 1 (to continue main loop), 0 (to exit the program)
============================================================ */
int run_command(pid_t pid) {

    char line[MAX_LENGTH] = {0};
    get_input(pid, line);

    struct args *cmd = parse_args(line);

    int exit_status = 0;

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

    // exit command
    if (strcmp(cmd->argv[0], "exit") == 0) {
        // Exits main loop if no arguments are provided
        if (cmd->argc == 1) {
            free(cmd);
            return 0;
        }
        else {
            fprintf(stderr, "exit: too many arguments\n");
            fflush(stderr);
        }
    }

    // cd command
    else if (strcmp(cmd->argv[0], "cd") == 0) {
        // Changes wprking dir to the HOME environment variable if no arguments are provided
        if (cmd->argc == 1) {
            if (chdir(getenv("HOME")) == 0) {
                char cwd[MAX_LENGTH];
                getcwd(cwd, sizeof(cwd));
                printf("cd: %s\n", cwd);
                fflush(stdout);
            }
        }
        // Changes working dir to the relative path passed as an argument if one is provided
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

    else if (strcmp(cmd->argv[0], "status") == 0) {
        printf("exit value %d\n", exit_status);
    }

    free(cmd);
    return 1;
}

/* ============================================================
get_input() reads in the line character by character. It replaces
all instances of the expansion variable $$ with the PID.

Recieves: pid -> pid_t, line -> char*
Returns: none
Postconditions: line references input string
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
    int current_char, expansion_flag = 0;

    // Read stdin until the max char length is reached or a newline is encountered
    do{
        current_char = getchar();

        if (current_char == '$' && expansion_flag == 0) {
            expansion_flag = 1;
        }
        // If the current and previous char were both $, append pidstr
        else if (current_char == '$' && expansion_flag == 1) {
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
            line[index] = current_char;
            index += 1;
        }
    } while(index < MAX_LENGTH && current_char != '\n');

    // Null-terminate string
    line[index - 1] = 0;
}

/* ============================================================
parse_args() takes the user input and parses tokens delineated
by whitespaces into an arg struct defined at the top and returns
the struct.

Recieves: line -> char*
Returns: struct args*
============================================================ */
struct args* parse_args(char *line) {
    char line_cpy[MAX_LENGTH];
    strcpy(line_cpy, line);

    struct args *cmd = malloc(sizeof(struct args));
    cmd->argc = 0;
    cmd->background_flag = 0;
    int redir_flag = 0;

    // Parse line into an array of strings delineated by whitespace
    char *token = strtok(line_cpy, DELIM);
    while (token != NULL) {
        // redir_flag == 1 indicates next token is the input file
        if ((strcmp(token, "<") == 0) && (redir_flag == 0) && (cmd->input_file == NULL)) {
            redir_flag = 1;
        }
        // redir_flag == 2 indicates next token is the output file
        else if (strcmp(token, ">") == 0 && (redir_flag == 0) && (cmd->output_file == NULL)) {
            redir_flag = 2;
        }
        else {
            if (redir_flag == 1) {
                cmd->input_file = token;
            }
            else if (redir_flag == 2) {
                cmd-> output_file = token;
            }
            redir_flag = 0;
        }
        
        cmd->argv[cmd->argc] = token;
        cmd->argc += 1;
        token = strtok(NULL, DELIM);
    }

    // Set background flag if the last arg is &
    if (strcmp(cmd->argv[cmd->argc - 1], "&") == 0){
        cmd->background_flag = 1;
    }

    return cmd;
}

