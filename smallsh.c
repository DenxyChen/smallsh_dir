//  Name: Xiao Yu Chen
//  Class: CS344 Spring 2022
//  Date: 05/09/2022
//  Description: An implementation of a shell program (command-line interface) in C.

#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h> 
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_LENGTH 2048
#define MAX_ARG 512
#define DELIM " "

int exit_status = 0;

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

pid_t background_processes[256];
int process_count = 0;

// Unused due to issues with pointers to the struct
// struct args* parse_args(char *line);
// void cd(char *path);
// void create_child(struct args* cmd);
// void run_in_foreground(struct args* cmd);

/* ============================================================
main() runs a loop that continuously calls run_command() 
to get a line of input from the user and executes it while
run_flag is true. If the "exit" command is given, run_command()
sets run_flag to false which breaks the loop and exits the program.
============================================================ */
int main() {
    int run_flag = 1;
    pid_t pid = getpid();

    // struct sigaction SIGSTP_action = {0};

    // SIGSTP.sa_handler = handle_SIGSTP;
    // sigfillset(&SIGSTP_action.sa_mask);
    // SIGSTP_action.sa_flags = 0;
    // sigaction(SIGSTP, &SIGSTP_action, NULL);
    // struct sigaction ignore_action;

    // ignore_action.sa_handler = SIG_IGN;
    // sigaction(SIGINT, &ignore_action, NULL);

    printf("%d\n", pid);

    while (run_flag == 1) {
        // Check for terminated background processes

        for(int i = 0; i < process_count; i++) {
            if(background_processes[i] != -1) {
                int result, status;
                result = waitpid(background_processes[i], &status, WNOHANG);
                if (result != 0) {
                    if (WIFSIGNALED(status)) {
                        printf("background pid %d is done: terminated by signal %d\n", background_processes[i], WTERMSIG(status));
                        fflush(stdout);
                        background_processes[i] = -1;
                    }
                    if (WIFEXITED(status)) {
                        printf("background pid %d is done: exit value %d\n", background_processes[i], WEXITSTATUS(status));
                        fflush(stdout);
                        background_processes[i] = -1;
                    }
                }
            }
        }

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

Note to grader: Initially implemented with parse_args() to handle
tokenization of the user input, but I ran into issues with 
parsing more than 10 arguments. It seems to work when integrated
with this function so I moved all of the functionality over. I
ran into more issues with create_child() and run_in_foreground()
so unfortunately, those were implemented in this function as well.

Recieves: pid -> pid_t
Returns: 1 (to continue main loop), 0 (to exit the program)
============================================================ */
int run_command(pid_t pid) {

    char line[MAX_LENGTH] = {0};
    get_input(pid, line);

    // char line_cpy[MAX_LENGTH];
    // strcpy(line_cpy, line);

    struct args *cmd = malloc(sizeof(struct args));
    cmd->argc = 0;
    memset(cmd->argv, 0, sizeof(cmd->argv));
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->background_flag = 0;
    
    int redir_flag = 0;

    // Parse line into an array of strings delineated by whitespace
    char *token = strtok(line, DELIM);
    while (token != NULL) {
        // redir_flag == 1 indicates next token is the input file
        if ((strcmp(token, "<") == 0) && (redir_flag == 0) && (cmd->input_file == NULL)) {
            redir_flag = 1;
            cmd->argv[cmd->argc] = NULL;
        }
        // redir_flag == 2 indicates next token is the output file
        else if (strcmp(token, ">") == 0 && (redir_flag == 0) && (cmd->output_file == NULL)) {
            redir_flag = 2;
            cmd->argv[cmd->argc] = NULL;
        }
        else {
            if (redir_flag == 1) {
                cmd->input_file = token;
            }
            else if (redir_flag == 2) {
                cmd-> output_file = token;
            }
            redir_flag = 0;
            cmd->argv[cmd->argc] = token;
        }
        cmd->argc += 1;
        token = strtok(NULL, DELIM);
    }

    // Set background flag and null the last arg if &
    if (strcmp(cmd->argv[cmd->argc - 1], "&") == 0){
        cmd->background_flag = 1;
        cmd->argv[cmd->argc - 1] = NULL;
    }

    // Ignore empty lines and lines that start with #
    if ((cmd->argc == 0) | (line[0] == '#')){
        printf("\n");
        fflush(stdout);
        free(cmd);
        return 1;
    }

    // printf("%d - ", cmd->argc);
    // fflush(stdout);
    // for (int i = 0; i < cmd->argc; i++) {
    //     printf("%s ", cmd->argv[i]);
    //     fflush(stdout);
    // }
    // printf("\n");
    // fflush(stdout);

    // exit command
    if (strcmp(cmd->argv[0], "exit") == 0) {
        // Exits main loop if no arguments are provided
        if (cmd->argc == 1) {
            free(cmd);

            // Kill all background child processes
            for (int i = 0; i < process_count; i++) {
                if (background_processes[i] != -1) {
                    kill((background_processes[i]), SIGTERM);
                }
            }

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
                // char cwd[MAX_LENGTH];
                // getcwd(cwd, sizeof(cwd));
                // printf("cd: %s\n", cwd);
                // fflush(stdout);
            }
        }
        // Changes working dir to the relative path passed as an argument if one is provided
        else if (cmd->argc == 2) {
            if (chdir(cmd->argv[1]) == 0) {
                // char cwd[MAX_LENGTH];
                // getcwd(cwd, sizeof(cwd));
                // printf("cd: %s\n", cwd);
                // fflush(stdout);
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

    // status command
    else if (strcmp(cmd->argv[0], "status") == 0) {
        printf("exit value %d\n", exit_status);
        fflush(stdout);
    }

    // Fork to run exec() family commands
    else {
        pid_t this_pid = fork();

        // Print error message if forking fails
        if (this_pid == -1) {
            fprintf(stderr, "fork: %s", strerror(errno));
            fflush(stderr);
        }

        // Run if the calling process is a child
        else if (this_pid == 0) {
            int in_fd, out_fd;
            int dev_null_open_flag = 0; 
            // struct sigaction SIGSTP_action = {0};
            // struct sigaction default_action = {0};

            // SIGSTP.sa_handler = handle_SIGSTP;
            // sigfillset(&SIGSTP_action.sa_mask);
            // SIGSTP_action.sa_flags = 0;
            // sigaction(SIGSTP, &SIGSTP_action, NULL);

            // default_action.sa_handler = SIG_DFL;

            // sigaction(SIGINT, &default_action, NULL);

            // Handle input redirection if specified
            if(cmd->input_file != NULL) {
                in_fd = open(cmd->input_file, O_RDONLY);
                if (in_fd == -1) {
                    fprintf(stderr, "input: cannot open %s for input\n", cmd->input_file);
                    fflush(stderr);
                    exit_status = 1;
                    free(cmd);
                    return 1;
                }
                dup2(in_fd, STDIN_FILENO);
            }
            // Handle input redirection if background and unspecified
            else if (cmd->background_flag == 1) {
                in_fd = open("/dev/null", O_RDWR);
                dev_null_open_flag = 1;
                dup2(in_fd, STDIN_FILENO);
            }

            // Handle output redirection if specified
            if(cmd->output_file != NULL) {
                out_fd = open(cmd->output_file, O_WRONLY | O_CREAT|O_TRUNC, 00644);
                if (out_fd == -1) {
                    fprintf(stderr, "output: cannot open %s for output\n", cmd->output_file);
                    fflush(stderr);
                    exit_status = 1;
                    free(cmd);
                    return 1;
                }
                dup2(out_fd, STDOUT_FILENO);
            }
            // Handle input redirection if background and unspecified
            else if (cmd->background_flag == 1 && (dev_null_open_flag != 1)) {
                out_fd = open("/dev/null", O_RDWR);
                dev_null_open_flag = 1;
                dup2(in_fd, STDOUT_FILENO);
            }

            // Call exec() to run the given command with args
            // Redirection operators < and > are set as NULL in argv
            // exec() stops as soon as it encounters NULL
            execvp(cmd->argv[0], cmd->argv);

            // Error code that is executed if exec() fails
            fprintf(stderr, "%s: %s\n", cmd->argv[0], strerror(errno));
            fflush(stderr);
            exit_status = 1;
            free(cmd);

            if (cmd->background_flag == 0) {
                exit(1);
            }
            else{
                kill(this_pid, SIGTERM);
            }
        }

        // Run if the calling process is the parent
        else {
            // Run foreground process to completion before returning control
            if (cmd->background_flag == 0) {
                int status;
                waitpid(this_pid, &status, 0);
                if (WIFSIGNALED(status)) {
                    exit_status = WTERMSIG(status);
                    printf("foreground: terminated by signal %d", exit_status);
                    fflush(stdout);
                }
                else if (WIFEXITED(status)) {
                    exit_status = WEXITSTATUS(status);
                }
            }
            // Otherwise add the background process to an array of background PIDs
            // Status of background is tracked in the main loop
            else
            {
                printf("background pid is: %d\n", this_pid);
                background_processes[process_count] = this_pid;
                process_count += 1;
            }
        }
    }

    // Free malloc'd struct, return 1 to continue loop
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
// struct args* parse_args(char *line) {
//     // char line_cpy[MAX_LENGTH];
//     // strcpy(line_cpy, line);

//     // struct args *cmd = malloc(sizeof(struct args));
//     // cmd->argc = 0;
//     // cmd->background_flag = 0;
//     // int redir_flag = 0;

//     // // Parse line into an array of strings delineated by whitespace
//     // char *token = strtok(line_cpy, DELIM);
//     // while (token != NULL) {
//     //     // redir_flag == 1 indicates next token is the input file
//     //     if ((strcmp(token, "<") == 0) && (redir_flag == 0) && (cmd->input_file == NULL)) {
//     //         redir_flag = 1;
//     //     }
//     //     // redir_flag == 2 indicates next token is the output file
//     //     else if (strcmp(token, ">") == 0 && (redir_flag == 0) && (cmd->output_file == NULL)) {
//     //         redir_flag = 2;
//     //     }
//     //     else {
//     //         if (redir_flag == 1) {
//     //             cmd->input_file = token;
//     //         }
//     //         else if (redir_flag == 2) {
//     //             cmd-> output_file = token;
//     //         }
//     //         redir_flag = 0;
//     //     }
        
//     //     cmd->argv[cmd->argc] = token;
//     //     cmd->argc += 1;
//     //     token = strtok(NULL, DELIM);
//     // }

//     // // Set background flag if the last arg is &
//     // if (strcmp(cmd->argv[cmd->argc - 1], "&") == 0){
//     //     cmd->background_flag = 1;
//     // }

//     return cmd;
// }

// void create_child(struct args* cmd) {
    // pid_t this_pid = fork();
    
    // // Print error message if forking fails
    // if (this_pid == -1) {
    //     fprintf(stderr, "fork: %s", strerror(errno));
    //     fflush(stderr);
    // }

    // // Run if the calling process is a child
    // else if (this_pid == 0) {
    //     // printf("I am a child running the command: %s\n", cmd->argv[0]);
    //     fflush(stdout);

    //     for (int i = 0; i < cmd->argc; i++) {
    //         printf("%s ", cmd->argv[i]);
    //     }
    //     // execvp(cmd->argv[0], cmd->argv);
    // }

    // // Run if the calling process is the parent
    // else {
    //     // printf("I am a parent running the command: %s\n", cmd->argv[0]);
    //     fflush(stdout);
    // }
// }


