//  Name: Xiao Yu Chen
//  Class: CS 344, WQ2022
//  Date: 2/07/2022
//  Description: An implementation of a shell in C that prompts for commands and runs them using the exec family of functions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> // pid_t
#include <unistd.h> // fork

#define MAX_LENGTH 2048
#define MAX_ARG 512
#define TOK_DELIM ' '

// struct movie
// {
//     char *title;
//     int year;
//     char *languages;
//     double rating;
//     struct movie *next;
// }; 

// /* Get user input and tokenize the arguments. Store the input and output file names. */
// void getInput(char* arguments, char* inputFile, char* outputFile, int* background) {
    
//     char *buf;
//     size_t buflen;

//     printf(": ");
//     getline(&buf, &buflen, stdin);
//     printf("%s", buf);
// }

int parse_command(pid_t pid);
void get_input(pid_t pid, char *line);

/* ============================================================
main() runs a loop that continuously gets a command from the user
while run_flag is true. When the user enters the "exit" command,
run_flag is set to false and the program exits.
============================================================ */
int main() {
    int run_flag = 1;
    pid_t pid = getpid();
    printf("pid is:    %d\n", pid);

    while (run_flag == 1) {
        printf(": ");
        fflush(stdout);
        run_flag = parse_command(pid);
    }

    return EXIT_SUCCESS;
}

/* ============================================================
parse_command() calls get_input() to read in the line with the
expansion variable accounted for. Then it executes the command.

Recieves: pid -> pid_t
Returns: 1 (to continue main loop), 0 (to exit the program)
============================================================ */
int parse_command(pid_t pid) {

    char line[MAX_LENGTH] = {0};
    get_input(pid, line);

    // Ignore empty lines and lines that start with #
    if ((strlen(line) == 0) | (line[0] == '#')){
        return 1;
    }

    printf("%s\n", line);
    fflush(stdout);

    if (strcmp(line, "exit") == 0) {
        return 0;
    }
    else{
        return 1;
    }
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