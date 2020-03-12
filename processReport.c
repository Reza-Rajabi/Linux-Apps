//
//  processReport.c
//  A simple app to capture Linux processes status.
//
//  Created by Reza Rajabi on 2020-01-17.
//

#include <stdio.h>          // EOF, FILE, printf(), fscanf(), fopen(), fclose()
#include <stdlib.h>         // atoi()
#include <string.h>         // strcat(), strcmp(), strncpy()
#include <dirent.h>         // DIR, dirnet, opendir(), readdir(), closedir()
#include <ctype.h>          // isdigit()
#include <sys/types.h>      // size_t

/**
 * This code will read the `/proc` directory of a Linux  file system
 * It finds the sub-directories with digit names (processes)
 * Then, it reads the `status` file of each process to report -
 * the name and id of those that consumed more than 10MB of memory
 */

typedef enum { false = 0, true = 1 } bool;

const char dirName[] = "/proc";
const char fileName[] = "/status";

// size of Pid for a number up to 2^22 = 4,194,304 in 64bit platform -> 7 char and '\0'
const size_t MAX_PID_SIZE = 8;
// size of path to the process directory: `/ proc / [Pid] -> 1 + 4 + 1 + MAX_PID_SIZE and '\0'
const size_t MAX_PATH_SIZE = 14;
// this is the longest title in the `status` file: `nonvoluntary_ctxt_switches`.
// However, we always read until `VmRSS` which is located before. So it can be less than 27
const size_t MAX_TITLE_SIZE = 27;
// name of process, can be up to 255 in most platforms. only show 30 characters.
const size_t MAX_NAME_SIZE_TO_DISPLAY = 31;


int main(void) {
    DIR* directoryStream = opendir(dirName);
    if (directoryStream == NULL) {
        printf("Could not open the %s directory.\n", dirName);
        return 1;
    }
    
    struct dirent* aEntityInDirectory;
    char path[MAX_PATH_SIZE];
    char title[MAX_TITLE_SIZE];
    char value[100]; // will be filled up to '\n' after a title from a line in the `status` file
    char Name[MAX_NAME_SIZE_TO_DISPLAY];
    char Pid[MAX_PID_SIZE];
    
    printf("%-30s | %-20s | %-20s\n", "Name", "Pid", "Memory Used");
    while( (aEntityInDirectory = readdir(directoryStream)) ) {
        if (isdigit(aEntityInDirectory->d_name[0])) {
            strcat(strcpy(path, dirName),"/");
            char* statusOfProcess_FileName = strcat(strcat(path, aEntityInDirectory->d_name),fileName);
            FILE* statusOfProcess_FilePtr = fopen(statusOfProcess_FileName, "r");
            if (statusOfProcess_FilePtr != NULL) {
                bool done = false;
                while (fscanf(statusOfProcess_FilePtr, "%[^:]: %[^\n]\n", title, value) != EOF && !done) {
                    size_t sizeValue = strlen(value);
                    if (strcmp(title, "Name") == 0) {
                        strncpy(Name, value, MAX_NAME_SIZE_TO_DISPLAY-1);
                        size_t size = sizeValue <= MAX_NAME_SIZE_TO_DISPLAY ? sizeValue : MAX_NAME_SIZE_TO_DISPLAY;
                        Name[size] = '\0';
                    }
                    if (strcmp(title, "Pid") == 0) {
                        strncpy(Pid, value, MAX_PID_SIZE-1);
                        size_t size = sizeValue <= MAX_PID_SIZE ? sizeValue : MAX_PID_SIZE;
                        Pid[size] = '\0';
                    }
                    if (strcmp(title, "VmRSS") == 0) {
                        // do not need to reed the rest of the file
                        done = true;
                        if (atoi(value) > 10000) // atoi() ignores the character values after the number
                            printf("%-30s | %-20s | %-20s\n", Name, Pid, value);
                    }
                }
                fclose(statusOfProcess_FilePtr);
            }
        }
    }
    closedir(directoryStream);
    
    printf("\n");
    return 0;
}
