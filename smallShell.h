/*******************************************************************************
* Author:   Aaron Huber
* Due Date: 02-08-2021
* Desc:     This is the speicification file for the small shell program. It
*           contains the function prototypes for the functions used for the
*           shell.
*******************************************************************************/
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>      
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  // pid_t, not used in this example
#include <sys/wait.h>
#include <unistd.h>     // getpid, getppid

#ifndef SMALL_SHELL_H
#define SMALL_SHELL_H

struct command {
    char* pathname;
    char** args;
};

void runShell();

// char** prompt();
struct command* prompt();

// char** parseString(char*,char*);
void parseString(char*, char*, struct command*);

struct command* buildCommandStruct(char**);

void runNonBuiltIn(int argc, char* args[]);

// void freeMemArray(char**); // free array of strings
void freeMem(struct command*);

int checkCommand(char*); // check command type

void removeNewLine(char*); // removes '\n' character from string passed

// returns INT_MIN to indicate built-in command
// int runCd(char**);          // runs shell cd functionality
int runCd(struct command*);

// prints out either exit status or terminating signal of last foreground 
// process ran by your shell
// returns INT_MIN to indicate built-in command
int runStatus(int);      

// int runOther(char**, int*); // runs nonbuilt-in commands, returns status
int runOther(struct command*, int*);

// expands any instance of $$ into the smallsh process id:
char* expandAny$$(char*);

// check for redirection:
// bool redirNeeded(char**, int*);
bool redirNeeded(struct command*, int*, bool);

bool isBackgroundProcess(char**);

void initProcArray(int*);

void initArgArray(struct command*);

void addChildProcess(int*, int);

void checkChildProcesses(int*);

void ignoreSIGINT();

void handleSIGTSTP();

void ignoreSIGTSTP();

void installSIGTSTP();

void killChildProcesses(int*);

#endif 
