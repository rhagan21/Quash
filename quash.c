/**
 * @file quash.c
 *
 * Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/ 
#include "quash.h" // Putting this above the other includes allows us to ensure
                   // this file's headder's #include statements are self
                   // contained.

#include <string.h>

/**************************************************************************
 * Private Variables
 **************************************************************************/
/**
 * Keep track of whether Quash should request another command or not.
 */
// NOTE: "static" causes the "running" variable to only be declared in this
// compilation unit (this file and all files that include it). This is similar
// to private in other languages.
static bool running;
char ** environ;

/**************************************************************************
 * Private Functions 
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
 */
//static void start() {
//  running = true;
//}

/**************************************************************************
 * Public Functions 
 **************************************************************************/
 
 struct linkedList {
	 struct linkedList *next;
	 int pid;
	 int jobid;
	 char command[10000];
 };
 
 
 struct linkedList* listHead = NULL;
 struct linkedList* listTail = NULL;
 int jobidvar = 0;
 
 void insert(struct linkedList *newlink) 
{
	if(listHead == NULL) {
		listHead = newlink;
		listTail = newlink;
	}
	else {
  		listTail->next = newlink;
  		listTail = listTail->next;
	}
}
 
void removeprocess(struct linkedList ** listP,int value) {
    struct linkedList *currP, *prevP;
    prevP = NULL;
    for (currP = *listP; currP != NULL; prevP = currP, currP = currP->next) {
        if (currP->pid == value) {
            printf("[%i] %i finished %s\n",  currP->jobid, currP->pid,  currP->command);
            if (prevP == NULL) {
                *listP = currP->next;
            } else {
                prevP->next = currP->next;
            }
            free(currP);
            return;
        }
    }
}

 
bool is_running() {
    return running;
}

void terminate() {
    running = false;
}

bool get_command(command_t* cmd, FILE* in) {
    if (fgets(cmd->cmdstr, MAX_COMMAND_LENGTH, in) != NULL) {
        size_t len = strlen(cmd->cmdstr);
        char last_char = cmd->cmdstr[len - 1];

        if (last_char == '\n' || last_char == '\r') {
            // Remove trailing new line character.
            cmd->cmdstr[len - 1] = '\0';
            cmd->cmdlen = len - 1;
        }
        else {
            cmd->cmdlen = len;
        }
        return true;
    } else {
        return false;
    }
}

int pfdbg[2], pipefdP[2];
bool backProcess = false;	
pid_t pidfg, pidbg, pid_p;

/**
 * Quash entry point
 *
 * @param argc argument count from the command line
 * @param argv argument vector from the command line
 * @return program exit status
 */

int main(int argc, char** argv) { 
    puts("Welcome to Quash!");
    puts("Type \"exit\" or \"quit\" to quit");

    int pfdfg[2];
    pipe(pfdfg);
    int pidpipe = 1;
    bool redirected = false;
    if(!isatty(STDIN_FILENO)) {
        redirected = true;
    }
	
    while(!backProcess) {
        int savedInputPipe = dup(STDIN_FILENO);
        bool pipeDetected = false;
        
        struct linkedList * newProcess = NULL;
            if(!redirected) {//prints terminal
                char arr2[1024];
                printf("\n[quash@");
                printf(getcwd(arr2, 1024));
                printf("]$ ");
            }
            char input[10000];
            if(fgets(input, 10000, stdin) == NULL) {
                break;
            }
            int isPipe = 0;
            char* pipePosition = strchr(input, '|');
            if (pipePosition != NULL) {
                isPipe =1;
            }
            int status;
            int childpid;
            while ((childpid = waitpid(-1, &status, WNOHANG)) > 0) {
                removeprocess(&listHead,childpid);
            }
            bool lastBackProc = false;
            if (strlen(input) == 1) {
                continue;
            }
            if(input[strlen(input)-2] == '&') {
                lastBackProc = true;
            }
            bool continueBool = false;
            char* tempProcess;
            tempProcess = strtok(input, "&\n");
            
            while(tempProcess) {
                char* nextProcess;
                nextProcess = strtok(NULL, "&\n");
                if (nextProcess == NULL) {
                    if(lastBackProc) {
                        pidbg = fork();
                        if (pidbg!=0){
                            newProcess = (struct linkedList*) malloc(sizeof(struct linkedList));
                            newProcess->next = NULL;
                            newProcess->jobid = jobidvar;
                            jobidvar ++;
                            newProcess->pid=pidbg;
                            insert(newProcess);
                            strcpy(newProcess->command, tempProcess);
                            printf("[%i] New process %i running in background\n", newProcess->jobid, newProcess->pid);
                            lastBackProc = false;
                            continueBool = true;
                            break;
                        }
                        backProcess = true;
                    }
                } else {
                    pidbg = fork();

                    if(pidbg != 0) {
                        newProcess = (struct linkedList*) malloc(sizeof(struct linkedList));
                        newProcess->next = NULL;
                        newProcess->jobid = jobidvar;
                        jobidvar++;
                        newProcess->pid = pidbg;
                        insert(newProcess);
                        strcpy(newProcess->command, tempProcess);
                        printf("[%i] New process %i running in background\n", newProcess->jobid, newProcess->pid);
                        tempProcess = nextProcess;
                        continue;
                    }
                    backProcess = true;
                }
                strcpy(input, tempProcess);
                break;
            }
            if(continueBool){
                continueBool = false;
                continue;
            }
            if(strchr(input, '|')){
                pipeDetected = true;
                char * tempCommand1 = strtok(input, "|");
                char * tempCommand2 = strtok(NULL, "|");
                if(pipe(pipefdP) == -1){
                    perror("pipefdP");
                    exit(EXIT_FAILURE);
                }
                pid_p = fork();
                if(pid_p == 0){
                    close (pipefdP[0]);
                    strcpy(input, tempCommand1);
                    dup2(pipefdP[1], STDOUT_FILENO);
                } else {
                    close(pipefdP[1]);
                    strcpy(input, tempCommand2);
                    dup2(pipefdP[0], STDIN_FILENO);
                }
            }
            char* args[100];
            int i = 0;
            char* tempArg = strtok(input, " \n");
            int numOfArgs = 0;
            char* inputFile = NULL;
            char* outputFile = NULL;
            int inFile;
            int outFile;
            int savedInput = dup(STDIN_FILENO);
            int savedOutput = dup(STDOUT_FILENO);
            bool usedinput = false;
            bool usedoutput = false;
            while(tempArg) {
                if (!strcmp(tempArg, "<")) {
                    inputFile = strtok(NULL, " \n");
                    if (inputFile == NULL) {
                        printf("No input file specified");
                        exit(0);
                    }
                } else if (!strcmp(tempArg, ">")) {
                    outputFile = strtok(NULL, " \n");
                    if (outputFile == NULL) {
                    printf("No output file specified");
                    exit(0);
                }
            } else {
                args[i++] = tempArg;
            }
            tempArg = strtok(NULL, " \n");
        }
        if (inputFile != NULL) {
            inFile = open(inputFile, O_RDONLY | O_CREAT);
            usedinput = true;
            dup2(inFile, STDIN_FILENO);
        }
        if (outputFile != NULL) {
            outFile = open(outputFile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            usedoutput = true;
            dup2(outFile, STDOUT_FILENO);
        }
        numOfArgs = i-1;
        // The commands should be parsed, then executed.
        if(pipeDetected && pid_p != 0){
            if((waitpid(pid_p, &status, 0)) == -1){
                fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
                return EXIT_FAILURE;
            }
        }
        
        if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit")) {
            puts("\nBye!");
            exit(0); // Exit Quash
        } else if (!strcmp(args[0], "set")) {
            char* tokenizer = strtok(args[1], "=");
            char* dirToSet = getenv(tokenizer);
            strcpy(dirToSet,strtok(NULL, "="));
            //run set command which sets the value of a variable. Quash should support (at least)
            //two built-in variables: PATH and HOME. PATH is used to record the paths to search
            //for executables, while HOME points the user's home directory. PATH may contain multiple
            //directories (separated by :) Ex: $set PATH=/usr/bin:/bin -> set the variable PATH to
            //contain two directories, /user/bin and /bin. $ set HOME=/home/amir -> set the user's home
            //directory as 'users/amir'
        } else if (!strcmp(args[0], "cd")) {
            char *dirToEnter;
            if(numOfArgs == 0) {
                dirToEnter = getenv("HOME");
                chdir (dirToEnter);
            } else if(numOfArgs == 1) {
                dirToEnter = args[1];
                chdir (dirToEnter);	
            }
        }
        //cd <dir> to change the current working directory to dir. If no argument is given, it 
        //should change to the directory in the HOME environment variable. Ex: $ cd test -> 
        //change the current working directory to ./test. $ cd -> change the current working 
        //directory to $HOME
        else if (!strcmp(args[0], "echo")) {
            char* dirToPrint;
            if (!strcmp(args[1], "$HOME")) {
                dirToPrint = getenv("HOME");
                printf(dirToPrint);
            } else if(!strcmp(args[1], "$PATH")) {
                dirToPrint = getenv("PATH");
                printf(dirToPrint);
            }
        } else if (!strcmp(input, "pwd")) {
            char arr3[1024];
            printf(getcwd(arr3, 1024));
            //prints the absolute path of the current working directory. Ex: $ mkdir test $ cd test
            //$ pwd -> prints /home/amir/test
        } else if (!strcmp(input, "jobs")) {
            //should print all of the currently running background processes in the format: [JOBID] PID COMMAND
            //where JOBID is a unique positive integer quash assigns to the job to identify it, PID is the
            //PID of the child process used for the job, and COMMAND is the command used to invoke the job
            //Ex: $ program 1 & $ program 2 & $ jobs [1] 2342 program1 [2] 2343 program2
            struct linkedList * temp;
            temp = listHead;
            while(temp!=NULL){
                printf("[%i] %i %s\n", temp->jobid, temp->pid, temp->command);
                temp=temp->next;
            }
        }
        //Need to implement I/O redirection. The '<' character is used to redirect the standard
        //input from a file. The '>' character is used to redirect the standard output to a file. Ex:
        //$ ls > 'a.txt'
        //Need to implement the pipe command |. Ex: $ cat myprog.c | more
        //Quash should support reading commands interactively (with a prompt) or reading a set of
        //commands stored in a file that is redirected from standard input. Ex: $ ./quash < commands.txt
        else {
            if(pipe(pfdbg) == -1) {
                perror("pipe1");
                exit(EXIT_FAILURE);
            }
            pidfg = fork();
            if(pidfg == 0){
                close(pfdbg[0]);
                close(pfdbg[1]);
                
                if(access(args[0], F_OK)==0){
                    execvpe(args[0], args, environ);
                } else {
                    int done = 0;
                    char copyEnv[2097152];
                    strcpy(copyEnv, getenv("PATH"));
                    char* tempPath = strtok(copyEnv, ":");
                    while(tempPath){
                        char * tempExec[1024];
                        strcpy(*tempExec, tempPath);
                        strcat(*tempExec, "/");
                        strcat(*tempExec, args[0]);
                        if(access(*tempExec, F_OK)==0) {
                            execvpe(args[0], args, environ);
                            done = 1;
                            break;
                        }
                        tempPath = strtok(NULL, ":");
                    }
                    if(done == 0) {
                        printf("Error!! File not found\n");
                    }
                }
                exit(0);
            } 
            close(pfdbg[0]);
            close(pfdbg[1]);
            close(pfdfg[1]);
            close(pfdfg[0]);
            if(isPipe > 1) {
                if ((waitpid(pidpipe, &status, 0)) == -1)  {
                    fprintf(stderr, "#1 Process pipe encountered an error. ERROR%d", errno);
                    return EXIT_FAILURE;
                }
                if(isPipe == 2) {
                    exit(0);
                }
                isPipe = 0;
            } else {
                if ((waitpid(pidfg, &status, 0)) == -1) {
                    fprintf(stderr, "#2 Process 1 encountered an error. ERROR%d", errno);
                    return EXIT_FAILURE;
                } 
            }
        }
        if(usedinput){
            dup2(savedInput, STDIN_FILENO);
            close(inFile);
            inputFile = NULL;
        }
        if(usedoutput){
            dup2(savedOutput, STDOUT_FILENO);
            close(outFile);
            outputFile = NULL;
        }
        close(savedInput);
        close(savedOutput);
        int j = 0;
        while(args[j] != NULL) {
            args[j++] = NULL;
        }
        if(pipeDetected){
            if(pid_p == 0){
                close(pipefdP[1]);
                close(savedInputPipe);
                exit(0);
            }
            dup2(savedInputPipe, STDIN_FILENO);
            close(pipefdP[0]);
        }
    }
}