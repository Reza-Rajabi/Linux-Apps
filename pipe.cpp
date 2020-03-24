//
//  pipe.cpp
//  A simple app that does the similar job as command line pipe does
//  example:        $ ./app "ls -la" "grep -i log"
//  is similar to:  $ ls -la | grep -i log
//
//  Created by Reza Rajabi on 2020-03-23.
//  Copyright © 2020 Reza Rajabi. All rights reserved.
//


#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <utility>

const size_t MAX_ARG_LEN = 45+1; /// max length of each user's given arguments
const int MAX_TOKEN = 3;         /// max number of tokens in each argument

void reportAndExit();
void deallocate();

char* arg1[MAX_TOKEN+1];
char* arg2[MAX_TOKEN+1];


int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cout << "Two arguments is needed." << std::endl;
        exit(1);
    }
    if (strlen(argv[1]) > MAX_ARG_LEN-1 || strlen(argv[2]) > MAX_ARG_LEN-1) {
        std::cout << "Each argument must have less than " << MAX_ARG_LEN;
        std::cout << " characters in total" << std::endl;
        exit(1);
    }
    
    // make a copy of the two const `argv`s
    /// setup variables
    char args[2][MAX_ARG_LEN];
    bzero(args[0], MAX_ARG_LEN);
    bzero(args[1], MAX_ARG_LEN);
    /// copy and append null terminator
    strcpy(args[0], argv[1]);
    args[0][strlen(argv[1])] = '\0';
    strcpy(args[1], argv[2]);
    args[1][strlen(argv[2])] = '\0';
    
    
    // split each copy to tokens and extract tokens
    /// define to char* for strtok() function to have the split version of args
    char* splitArgs[2];
    /// define some useful variables
    size_t max_tok_len = MAX_ARG_LEN/MAX_TOKEN;
    int count1 = 0, count2 = 0;
    /// split the args and extract tokens of each in an array of size `max_tok_len`
    for (int i = 0; i < 2; i++) {
        splitArgs[i] = strtok(args[i], " ");
        for (int j = 0; j < MAX_TOKEN && splitArgs[i] != NULL; j++) {
            if (strlen(splitArgs[i]) > max_tok_len-1) {
                std::cout << "Token " << j << " in argument " << i;
                std::cout << " is bigger than max " << max_tok_len << " character\n";
                exit(1);
            }
            if (i == 0) {
                arg1[j] = new char[max_tok_len];
                bzero(arg1[j], max_tok_len);
                strcpy(arg1[j], splitArgs[i]);
                arg1[j][strlen(splitArgs[i])] = '\0';
                count1++;
            }
            else if (i == 1) {
                arg2[j] = new char[max_tok_len];
                bzero(arg2[j], max_tok_len);
                strcpy(arg2[j], splitArgs[i]);
                arg2[j][strlen(splitArgs[i])] = '\0';
                count2++;
            }
            splitArgs[i] = strtok(NULL, " ");
        }
    }
    arg1[count1] = NULL;
    arg2[count2] = NULL;
    
    
    // making a pipe line
    /// setup variables
    int pipeFDs[2];
    pid_t leftPID, rightPID;
    /// make a pipe line
    if (pipe(pipeFDs) == -1) reportAndExit();
    /// spawn 2 processes each for one side of the pipe line
    ///     left hand side process
    leftPID = fork();
    if (leftPID == -1) reportAndExit();
    else if (leftPID == 0) { /// in the child process
        if ( dup2(pipeFDs[1], STDOUT_FILENO) == -1 ) reportAndExit();
        if ( close(pipeFDs[0]) == -1 || close(pipeFDs[1]) == -1 ) reportAndExit();
        execvp(arg1[0], arg1);
        /// if it touches next line, execl() has failed
        reportAndExit();
    }
    ///     right hand side process
    rightPID = fork();
    if (rightPID == -1) reportAndExit();
    else if (rightPID == 0) { /// in the child process
        if ( dup2(pipeFDs[0], STDIN_FILENO) == -1 ) reportAndExit();
        if ( close(pipeFDs[0]) == -1 || close(pipeFDs[1]) == -1 ) reportAndExit();
        execvp(arg2[0], arg2);
        /// if it touches next line, execl() has failed
        reportAndExit();
    }
    
    
    // clean up and closing in the main process
    if ( close(pipeFDs[0]) == -1 || close(pipeFDs[1]) == -1 )
        reportAndExit();
    if ( waitpid(leftPID, NULL, 0) == -1 || waitpid(rightPID, NULL, 0) == -1 ) reportAndExit();
    
    deallocate();
    
    return 0;
}


void reportAndExit() {
    strerror(errno);
    exit(1);
}

void deallocate() {
    for (int i = 0; i < MAX_TOKEN; i++) {
        delete arg1[i];
        delete arg2[i];
    }
}
