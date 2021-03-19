/*******************************************************************************
* Author:   Aaron Huber
* Due Date: 02-08-2021
* Desc:     This is the implementation file for the small shell program. It
*           contains the primary functions for running the shell.
*******************************************************************************/
#include "smallShell.h"

// global variable used to entering/exiting foreground-only mode:
bool foregroundMode = false;

/*******************************************************************************
* Function: runShell
* Desc:     function loops small shell and reacts to user's input. it receives
*           the status of processes run for notification to the user. commands
*           cd, status, and exit are built-in commands and programmed within
*           this file. remaining commands use exec() functions.
*******************************************************************************/
void runShell(){

    // ignore SIGINT:
    ignoreSIGINT();
    // holds status of last foreground process:
    int wstatus = 0; 

    // holds all of the child processes running in the background:
    int* childProcessArray = (int*)malloc(100 * sizeof(int)); 
    // initialize child process array elements to 0:
    initProcArray(childProcessArray);
    
    // command struct holds first arg as pathname and remaining as args array:
    struct command* newCommand = NULL;

    // categorizes user's input for switch:
    enum cmd{exit, cd, status, other, empty};
    enum cmd input = other;                             // holds user's input

    while(input != exit){
        // parent catches SIGTSTP and run only foreground:
        installSIGTSTP();

        newCommand = prompt();
        // assign input based on user's prompt entry:
        input = checkCommand(newCommand->pathname);

        switch(input){
            // built-in function:
            case exit:
                // kill any remaining child processes running:
                killChildProcesses(childProcessArray);
                break;
            // built-in function:
            case cd:
                wstatus = runCd(newCommand);
                break;
            // built-in function:
            case status:
                wstatus = runStatus(wstatus);
                break;
            // nonbuilt-in function:
            case other:
                wstatus = runOther(newCommand, childProcessArray);
                if(wstatus == 2){
                    printf("terminated by signal 2\n");
                    fflush(stdout);
                }
                break;
            default:
                break;
        }

        // check if child processes have concluded/terminated:
        if(input != exit)
            checkChildProcesses(childProcessArray);

        // free dyn allocated memory:
        freeMem(newCommand);
    }
}

/*******************************************************************************
* Function: prompt
* Desc:     creates new command struct and gets user input. it allows for 2048
*           characters and 512 arguments within those characters. if a user
*           enters $$, this is expanded to the process id of smallsh. the input
*           string is parsed to create the command struct which gets returned.
*******************************************************************************/
struct command* prompt(){
    // holds user's input:
    char* input;

    // new struct to get populated with user's input:
    struct command* newCommand = malloc(sizeof(struct command));

    // 2048 characters permitted, but must make space for /n and /0:
    int numChars = 2048;
    input = (char*)malloc((numChars + 2) * sizeof(char));

    // get input:
    printf(": ");
    fflush(stdout);
    fgets(input, numChars + 2, stdin);

    // if $$ is entered anywhere, replace with smallsh pid:
    input = expandAny$$(input);

    // remove \n character:
    if (strlen(input) > 2)
        removeNewLine(input);

    // parse string by spaces and populate command struct:
    parseString(input, " ", newCommand);
    free(input);

    return newCommand;
}

/*******************************************************************************
* Function: parseString
* Desc:     function receives the string to be parsed, the string to use as a
*           delimiter, and the command struct to populate. nothing is returned
*           since data is saved to memory addresses.
*******************************************************************************/
void parseString(char* str, char* delim, struct command* newCommand){
    int i = 0; // used for storing args in indices
    char* saveptr = NULL;

    // 512 arguments allowed (513 extra slot needed for NULL indicator):
    newCommand->args = malloc(513 * sizeof(char*));
    initArgArray(newCommand);

    // if input starts with enter or is # then set pathname to null:
    if (str[0] == '\n' || str[0] == '#'){
        newCommand->pathname = NULL;
        return;
    }

    // first token represents the pathname of the command:
    char* token = strtok_r(str, delim, &saveptr);
    newCommand->pathname = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCommand->pathname, token);

    // first token pathname is also saved in args[0] index for execv function:
    newCommand->args[i] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(newCommand->args[i], token);
    ++i;

    // move to next index:
    token = strtok_r(NULL, delim, &saveptr);

    // add all remaining commands to args array:
    while(token != NULL){
        newCommand->args[i] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(newCommand->args[i], token);
        ++i;
        token = strtok_r(NULL, delim, &saveptr);
    }

    // NULL used for indicating:
    newCommand->args[i] = NULL;
}

/*******************************************************************************
* Function: freeMem
* Desc:     function receives command struct and frees dynamically allocated
*           memory.
*******************************************************************************/
void freeMem(struct command* newCommand){

    free(newCommand->pathname);

    for (int i = 0; i < 513; ++i)
        free(newCommand->args[i]);

    free(newCommand->args);

    free(newCommand);
}

/*******************************************************************************
* Function: checkCommand
* Desc:     function is used within smallsh loop logic runShell. it receives a
*           string that represents the user's first command, which should be the
*           pathname of the function being called. it returns a number which
*           is used for determining the action based on the input
*******************************************************************************/
int checkCommand(char* arg){

    if(arg == NULL)
        return 4;  // empty (enum in runShell)

    if(strcmp(arg, "exit") == 0)
        return 0;

    if(strcmp(arg, "cd") == 0)
        return 1;

    if(strcmp(arg, "status") == 0)
        return 2;
    
    return 3;      // other (enum in runShell)
}

/*******************************************************************************
* Function: removeNewLine
* Desc:     function removes the trailing \n character added to the string
*           during fgets input in the prompt function. the \n character is
*           replaced with the \0 terminator.
*******************************************************************************/
void removeNewLine(char* str){
    int len = strlen(str);
    if (str[len - 1] == '\n')
        str[len - 1] = '\0';
    if (str[len - 2] == '\n')
        str[len - 2] = '\0';
}

/*******************************************************************************
* Function: runStatus
* Desc:     function receives an integer representing the status of a function.
*           this is a built-in function of smallsh. it takes the integer and
*           intreprets is status. if the function receives INT_MIN, it means
*           a built-in function was just ran which are ignored. since status
*           is a built-in function it returns INT_MIN back to runShell.
*******************************************************************************/
int runStatus(int status){
    if (status == INT_MIN)  // INT_MIN designates built-in command, no return
        return INT_MIN;

    if(WIFEXITED(status)) // terminated normally:
        printf("exit value %d\n", WEXITSTATUS(status));
    else // did not terminate normally:
        printf("terminated by signal %d\n", WTERMSIG(status));

    fflush(stdout);

    // status function is built-in function:
    return INT_MIN;
}

/*******************************************************************************
* Function: runCd
* Desc:     this is a built-in function. the function receives a command struct
*           and gets the 2nd argument passed from the user. the second argument
*           is the file path to be used with chdir (if not empty).
*******************************************************************************/
int runCd(struct command* newCommand){
    char* home = getenv("HOME");
    char* command = newCommand->args[1];

    // cd doesn't react to & entered by user:
    if(command != NULL && strcmp(command, "&") == 0)
        command = NULL;

    // if there is no 2nd argument or it's ~, send user to home dir:
    if(command == NULL || strcmp(command, "~") == 0)
        chdir(home);
    // else take user to 2nd argument path or catch error:
    else if (chdir(command) != 0){
        printf(": cd: %s: no such file or directory\n", command);
        fflush(stdout);
    }

    // cd is built-in function so return INT_MIN as indicator to status:
    return INT_MIN;
}

/*******************************************************************************
* Function: runOther
* Desc:     runs non-built in commands by the user. these include all those
*           outside of cd, status, and exit. the function recieves a command
*           struct and the child process array. it's status is returned to the
*           calling function, runShell.
*           
*         
*******************************************************************************/
int runOther(struct command* newCommand, int* childProcessArray){

    int spawnStatus;
    int childPid = INT_MIN;

    // check if process should be run in the background:
    bool isBackProc = isBackgroundProcess(newCommand->args);

    // check if program is in foreground mode(global variable):
    if(foregroundMode)
        isBackProc = false;

    // holds fd array if redirection is used:
    int* fdArray = NULL;

    // fork a new child process:
    pid_t spawnId = fork();

    switch(spawnId){
        case -1:
            perror("fork() failed!\n");
            exit(1);
            break;
        case 0:
            childPid = getpid();

            // ignore SIGTSTP (^Z):
            ignoreSIGTSTP();

            // notify user of background pid, if in background mode:
            if(isBackProc){
                printf("background pid is %d\n", childPid);
                fflush(stdout);
            }
            else // set SIGINT (^C)to default so foreground child responds:
                signal(SIGINT, SIG_DFL);

            // check if redirection is not needed, if so use execvp function
            if(!redirNeeded(newCommand, fdArray, isBackProc))
                execvp(newCommand->pathname, newCommand->args);
            else // redirection is needed so used execlp:
                execlp(newCommand->pathname, newCommand->pathname, NULL);

            // exec error printing (child only returns due to error):
            printf("%s: no such file or directory\n", newCommand->pathname);
            fflush(stdout);
            freeMem(newCommand);          // free dyn memory of child process
            exit(1);
        default:
            if(isBackProc){
                /* if it's run in the background, add child to child process
                array for tracking within runShell */
                addChildProcess(childProcessArray, spawnId);

                // returns 0 if parent isn't waiting for child to finish
                spawnId = waitpid(spawnId, &spawnStatus, WNOHANG);

                // sleep to allow time for smallsh to print ":"
                sleep(1);
            }
            else {
                spawnId = waitpid(spawnId, &spawnStatus, 0);
                // printf("spawnId = %d\n", spawnId);
            }
            break;
    }

    return spawnStatus;
}

/*******************************************************************************
* Function: expandAny$$
* Desc:     function receives string and replaces any $$ with smallsh process
*           id. it returns the expanded string to the calling function (prompt).
*******************************************************************************/
char* expandAny$$(char* str){
   if (str == NULL)
        return NULL;

    int trackAdj$ = 0;   // used to track adjacent $ char
    int i = 0;           // for traversing passed string
    int j = 0;           // for incrementing expanded char array below

    // hold process id in c-string, up to 10 chars:
    char* pidChar = malloc(11 * sizeof(char));
    sprintf(pidChar, "%d", getpid());

    // new string to hold expanded string:
    char* expandedStr = malloc(strlen(str) * sizeof(pidChar));

    // expand $$ occurences:
    while (str[i] != '\0'){

        // if char is $ and prev char is $, expand to process id:
        if(str[i] == '$' && trackAdj$ == 1){
            trackAdj$ = 0;

            // save process id to new string:
            for (int k = 0; k < strlen(pidChar); ++k){
                expandedStr[j] = pidChar[k];
                ++j;
            }
        }

        // if current char is not $, but previous is save prev $:
        else if(trackAdj$ == 1){
            expandedStr[j] = '$';    // save prev char $
            ++j;                     // incr j for next char of new string
            --i;                     // decrement i since saving prev char
            trackAdj$ = 0;
        }

        // if prev char was not $, but current char is $, 
        // note hold off from saving char until next char is evaluated:
        else if (str[i] == '$')
            ++trackAdj$;

        // if current char is not $ and prev char is not $, save to new string:
        else { 
            expandedStr[j] = str[i];
            ++j;
        }
            
        ++i;
    }

    // terminate new string:
    expandedStr[j] = '\0';

    // free dyn memory of process id and passed in old string:
    free(pidChar);
    free(str);

    return expandedStr;
}

/*******************************************************************************
* Function: redirNeeded
* Desc:     function determines if a redirection is required. this means that 
*           the user entered < to redirect stdout and > to redirect stdin. the
*           redirected input is opened for read only. the output is opened for
*           write only, truncating if already exist or create new. both of these
*           redirections can occur at once.
*           
*           the function returns true if redirection is needed and false if it 
*           is not.
*******************************************************************************/
bool redirNeeded(struct command* newCommand, int* fdArray, bool isBackProc){
    bool sourceRedirect = false; // used for tracking if source redir occured
    bool targetRedirect = false; // used for tracking if target redir occured
    int sourceFD; // used to store source redirection file descriptor
    int targetFD; // used to store target redirection file descriptor
    int result; // used to capture dup2 result value
    int i = 0; // used to cycle through string Array
    int j = 0; // used to track index for saving file descriptors
    char* sourceFileName;
    char* targetFileName;
    char** args = newCommand->args;

    // create int array for hold FDs to close later:
    fdArray = (int*)malloc(10 * sizeof(int));

    while(args[i] != NULL){
        // look for target redirection and filename will trail it:
        if(strcmp(args[i], ">") == 0 && args[i+1] != NULL){
            targetFileName = args[i+1];
            targetFD = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            fdArray[j++] = targetFD;
            targetRedirect = true;
        }

        // look for source redirection and filename will trail it:
        if(strcmp(args[i], "<") == 0 && args[i+1] != NULL){
            sourceFileName = args[i+1];
            sourceFD = open(args[i+1], O_RDONLY);
            fdArray[j++] = sourceFD;
            sourceRedirect = true;
        }
        ++i;
    }

    // no source redirect specified and process is ran in background
    // therefore stdin redirected to /dev/null:
    if(!sourceRedirect && isBackProc)
        sourceFD = open("/dev/null", O_RDONLY);

    // no target redirect specified and process is ran in background
    // therefore stdout redirected to /dev/null:
    if(!targetRedirect && isBackProc)
        targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    fdArray[j] = INT_MIN; // INT_MIN used as end of array terminator

    // catch source file error:
    if(sourceRedirect && sourceFD == -1){
        printf("cannot open %s for input\n", sourceFileName);
        fflush(stdout);
        free(fdArray);
        freeMem(newCommand);
        exit(1);
    }

    // catch target file error:
    if(targetRedirect && targetFD == -1){
        printf("cannot open %s for output\n", targetFileName);
        fflush(stdout);
        free(fdArray);
        freeMem(newCommand);
        exit(1);
    }

    if(sourceRedirect){
        // redirect stdin to source file:
        result = dup2(sourceFD, 0);
        if (result == -1){
            perror("source dup2()");
            free(fdArray);
            freeMem(newCommand);
            exit(2);
        }
    }

    if(targetRedirect){
        // redirect stdout to target file:
        result = dup2(targetFD, 1);
        if (result == -1){
            perror("target dup2()");
            free(fdArray);
            freeMem(newCommand);
            exit(2);
        }
    }

    if(!sourceRedirect && !targetRedirect){
        free(fdArray); // fdArray not used
        return false;
    }

    return true;
}

/*******************************************************************************
* Function: isBackgroundProcess
* Desc:     function returns an array of strings representing all the args
*           entered by the user. if the trailing arg is a &, then the process
*           is to be ran in the background, returns true. else return false.
*******************************************************************************/
bool isBackgroundProcess(char** args){

    int i = 0;

    while(args[i] != NULL){
        if(strcmp(args[i], "&") == 0 && args[i + 1] == NULL){
            args[i] = NULL; // remove & from args array
            return true;
        }
        ++i;
    }

    return false;
}

/*******************************************************************************
* Function: initProcArray
* Desc:     initializes child process list created in runShell to hold values of
*           zero.
*******************************************************************************/
void initProcArray(int* childProcessArray){

    for (int i = 0; i < 100; ++i)
        childProcessArray[i] = 0;
}

/*******************************************************************************
* Function: initArgArray
* Desc:     function receives struct command and initializes all indices to NULL
*******************************************************************************/
void initArgArray(struct command* newCommand){

    for(int i = 0; i < 513; ++i)
        newCommand->args[i] = NULL;
}

/*******************************************************************************
* Function: addChildProcess
* Desc:     function receives child process array and adds integer to it which
*           represents the background processed child. calling funciton is
*           runOther.
*******************************************************************************/
void addChildProcess(int* childProcessArray, int childPid){

    for (int i = 0; i < 100; ++i){
        if(childProcessArray[i] == 0){
            childProcessArray[i] = childPid;
            break;
        }
    }
}

/*******************************************************************************
* Function: checkChildProcesses
* Desc:     function receives child process array and checks each one to see if
*           they are still running. if so, no action is taken. if not, the
*           background pid is printed and the child pid is removed from the
*           array.
*******************************************************************************/
void checkChildProcesses(int* childProcessArray){

    int childId;
    int childIdStatus;

    for(int i = 0; i < 100; ++i){
        if(childProcessArray[i] != 0){

            // check child process stored in child process array:
            childId = waitpid(childProcessArray[i], &childIdStatus, WNOHANG);

            /* if child process is still running childId == 0, else it is equal
             to original child process id when finished */
            if(childId == childProcessArray[i]){
                printf("background pid %d is done: ", childProcessArray[i]);
                fflush(stdout);
                runStatus(childIdStatus); // prints exit status
                childProcessArray[i] = 0; // remove child pid from array
            }
        }
    }
}

/*******************************************************************************
* Function: ignoreSIGINT
* Desc:     function disables SIGINT (^C) by ignoring it
*******************************************************************************/
void ignoreSIGINT(){

    // initialize ignore_action struct to be empty:
    struct sigaction sigint_ignore_action = {{0}};

    // set to signal ignore constant:
    sigint_ignore_action.sa_handler = SIG_IGN;

    // install signal handler:
    sigaction(SIGINT, &sigint_ignore_action, NULL);
}

/*******************************************************************************
* Function: installSIGTSTP
* Desc:     function catches SIGTSTP (^Z) execution and redirects it to it's
*           signal handler handleSIGTSTP.
*******************************************************************************/
void installSIGTSTP(){

    // initialize sigstp_action struct to be empty:
    struct sigaction sigtstp_action = {{0}};

    // set to signal ignore constant:
    sigtstp_action.sa_handler = handleSIGTSTP;

    // block all catchable signals while handleSIGSTP is running
    sigfillset(&sigtstp_action.sa_mask);

    // no flags set:
    sigtstp_action.sa_flags = SA_RESTART;

    // install signal handler:
    sigaction(SIGTSTP, &sigtstp_action, NULL);
}

/*******************************************************************************
* Function: handleSIGTSTP
* Desc:     function fires with SIGTSTP (^Z) is executed. it toggles the boolean
*           global variable, foregroundMode, which enables/disables foreground
*           only mode within smallsh. it notifies the user, note: write must
*           be used here because it is a reentrant function (not printf). also
*           note that strlen is not used since it is not a reentrant function.
*******************************************************************************/
void handleSIGTSTP(int signo){

    char* message;

    // check global variable and toggle for enabling/disabling foreground mode:
    if(!foregroundMode){
        message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 52);
        foregroundMode = true;
    }
    else{
        message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 32);
        foregroundMode = false;
    }

    fflush(stdout);
}

/*******************************************************************************
* Function: ignoreSIGTSTP
* Desc:     function disables SIGTSTP (^Z) by ignoring it
*******************************************************************************/
void ignoreSIGTSTP(){

    // initialize ignore_action struct to be empty:
    struct sigaction sigtstp_ignore_action = {{0}};

    // set to signal ignore constant:
    sigtstp_ignore_action.sa_handler = SIG_IGN;

    // install signal handler:
    sigaction(SIGTSTP, &sigtstp_ignore_action, NULL);
}

/*******************************************************************************
* Function: killChildProcesses
* Desc:     function receives child process id array (ints) and kills each one.
*           this function is called from runShell upon exiting the shell.
*******************************************************************************/
void killChildProcesses(int* childProcArray){
    for(int i = 0; i < 100; ++i){
        if(childProcArray[i] != 0)
            kill(childProcArray[i], SIGKILL);
    }

    free(childProcArray);
}
