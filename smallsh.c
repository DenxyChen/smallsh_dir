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

int parse_command();

/* Prompts the user, then run the command. */
int main() {
    int run_flag = 1;
    pid_t pid = getpid();
    char *pidstr;
    {
        int n = snprintf(NULL, 0, "%d", pid);
        pidstr = malloc((n + 1) * sizeof *pidstr);
        sprintf(pidstr, "%d", pid);
    }

    printf("pid is:    %d\n", pid);
    printf("pidstr is: %s\n", pidstr);

    while (run_flag == 1) {
        printf(": ");
        fflush(stdout);
        run_flag = parse_command();
    }

    return EXIT_SUCCESS;
}

int parse_command() {
    char line[MAX_LENGTH];
    int current, index = 0;

    do{
        current = getchar();
        line[index] = current;
        index += 1;
    } while(index < MAX_LENGTH && current != '\n');

    line[index - 1] = 0;
    printf("%s", line);

    if (strcmp(line, "exit") == 0) {
        return 0;
    }
    else{
        return 1;
    }
}
