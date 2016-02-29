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
extern char ** environ;

/**************************************************************************
 * Private Functions 
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
 */
static void start() {
  running = true;
}

/**************************************************************************
 * Public Functions 
 **************************************************************************/
 
 struct linkedList {
	 struct linkedList *next;// = NULL;
	 int pid;
	 int jobid;
	 char command[10000];
 };
 
 struct linkedList* listHead = NULL;
 struct linkedList* listTail = NULL;
 int jobidvar = 0;
 
 void insert(struct linkedList* item) {
	 if (listHead == NULL) {
		 listHead = item;
		 listTail = item;
	 }
	 else {
		 listTail -> next = item;
		 listTail = listTail -> next;
	 }
 }
 
 void removeprocess(int value) {
	 struct linkedList * current, * previous;
	 current = listHead;
	 previous = listHead;
	 while(current != NULL) {
		 if (current -> pid == value) {
			 previous -> next = current -> next;
			 free(current);
		 }
		 else {
			 previous = current;
			 current = current -> next;
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
    else
      cmd->cmdlen = len;
    
    return true;
  }
  else
    return false;
}


/**
 * Quash entry point
 *
 * @param argc argument count from the command line
 * @param argv argument vector from the command line
 * @return program exit status
 */
int main(int argc, char** argv) { 
  command_t cmd; //< Command holder argument
  
  start();
  
  puts("Welcome to Quash!");
  puts("Type \"exit\" or \"quit\" to quit");

  	int pfdfg[2];
	int pfdbg[2];
	pipe(pfdfg);
	pid_t pidfg, pidbg;
	bool backProcess;
	char* tempProcess;
	char* nextProcess;
	
	while(!backProcess) {
		struct linkedList * newProcess = NULL;
		
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
			removeprocess(childpid);
		}
		
		if (strlen(input) == 1) {
			continue;
		}
		
		if (isPipe != 0) {
			tempProcess = strtok(input, "|\n");
			dup2(pfdbg[1], 1);
			pidbg = fork();
			if (pidbg == 0) {
				close(pfdbg[0]);
				dup2(pfdbg[1], 1);
			}
		}
		
		while(tempProcess){
			if (isPipe != 0){
				strcpy(input, tempProcess);
				nextProcess = tempProcess;
			}
			if (nextProcess != NULL){
				pidbg = fork();
				if (pidbg!=0){
					newProcess = (struct linkedList*) malloc(sizeof(struct linkedList));
					newProcess->next = NULL;
					newProcess->jobid = jobidvar;
					jobidvar ++;
					newProcess->pid=pidbg;
					insert(newProcess);
					strcpy(newProcess->command, tempProcess);
					printf("[%i] New process %i running in background", newProcess->jobid, newProcess->pid);
					tempProcess = nextProcess;
					continue;
				}
				backProcess = true;
			}
			strcpy(input, tempProcess);
			break;
		}
		
		
	}
  

	char arr[1024];
	printf("[quash@");
	printf(getcwd(arr, 1024));
	printf("]$ ");
  // Main execution loop
  while (is_running() && get_command(&cmd, stdin)) {
    char* args[100];
    int i = 0;
    char* tempArg = strtok(cmd.cmdstr, " \n");
    int numOfArgs = 0;
    char* inputFile = NULL;
    char* outputFile = NULL;
    bool input = false;
    bool output = false;
    while(tempArg) {
        if (!strcmp(tempArg, "<")) {
            inputFile = strtok(NULL, " \n");
            tempArg = inputFile;
            numOfArgs++;
            i++;
            if (inputFile == NULL) {
                printf("No input file specified");
                exit(0);
            }
        }
        else if (!strcmp(tempArg, ">")) {
            outputFile = strtok(NULL, " \n");
            tempArg = outputFile;
            numOfArgs++;
            i++;
            if (outputFile == NULL) {
                printf("No output file specified");
                exit(0);
            }
        }
        else {
            args[i++] = tempArg;
            tempArg = strtok(NULL, " \n");
            numOfArgs++;
        }
    }
	
    if (inputFile != NULL) {
            open(inputFile, O_RDONLY | O_CREAT);
            input = true;
            dup2(*inputFile, STDIN_FILENO);
    }
    else if (outputFile != NULL) {
            open(outputFile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            output = true;
            dup2(*outputFile, STDOUT_FILENO);
    }
	
    // The commands should be parsed, then executed.
    if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit")) {
	  puts("\nBye!");
          terminate(); // Exit Quash
    }
    else if (!strcmp(args[0], "set")) {
        char* tokenizer = strtok(args[1], "=");
        char* dirToSet = getenv(tokenizer);
        strcpy(dirToSet,strtok(NULL, "="));
      //run set command which sets the value of a variable. Quash should support (at least)
      //two built-in variables: PATH and HOME. PATH is used to record the paths to search
      //for executables, while HOME points the user's home directory. PATH may contain multiple
      //directories (separated by :) Ex: $set PATH=/usr/bin:/bin -> set the variable PATH to
      //contain two directories, /user/bin and /bin. $ set HOME=/home/amir -> set the user's home
      //directory as 'users/amir'
    }
    else if (!strcmp(args[0], "cd")) {
        char *dirToEnter;
		
        if(numOfArgs == 1) {
            dirToEnter = getenv("HOME");
            chdir (dirToEnter);
        }
        else if(numOfArgs == 2) {
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
		}
		else if(!strcmp(args[1], "$PATH")) {
			dirToPrint = getenv("PATH");
			printf(dirToPrint);
		}
	}
    else if (!strcmp(cmd.cmdstr, "pwd")) {
		char arr3[1024];
		printf(getcwd(arr3, 1024));
      //prints the absolute path of the current working directory. Ex: $ mkdir test $ cd test
      //$ pwd -> prints /home/amir/test
    }
    else if (!strcmp(cmd.cmdstr, "jobs")) {
      //should print all of the currently running background processes in the format: [JOBID] PID COMMAND
      //where JOBID is a unique positive integer quash assigns to the job to identify it, PID is the
      //PID of the child process used for the job, and COMMAND is the command used to invoke the job
      //Ex: $ program 1 & $ program 2 & $ jobs [1] 2342 program1 [2] 2343 program2
        struct linkedList * temp;
        temp = listHead;
        while(temp!=NULL){
            printf("[%i] %i %s", temp->jobid, temp->pid, temp->command);
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
		pidfg = fork();
		if(pidfg == 0){
			int done = 0;
			close(pfdfg[0]);
			close(pfdfg[1]);
			
			char * tempEnv[10000];
			strcpy(tempEnv, getenv("PATH"));
			
			char * tempPath = strtok(PATH, "\"");
			printf(tempPath);
			
			if(access(args[0], F_OK)==0){
				execvpe(args[0], args, environ);
			}
			while(tempPath){
				char * tempExec[1024];
				strcpy(tempExec, tempPath);
				strcat(tempExec, "\"");
				strcat(tempExec, args[0]);
				if(access(tempExec, F_OK)==0){
					execvpe(args[0], args, environ);
					done = 1;
					break;
				}
				tempPath = strtok(NULL, "\"");
			}
			if(done == 0) {
				printf("Error!! File not found\n");
			}
			exit(0);
		} 
		close(pfdfg[0]);
		close(pfdfg[1]);
		
		if(input){
			dup2(*inputFile, STDIN_FILENO);
			close(*inputFile);
		}
		if(output){
			dup2(*outputFile, STDOUT_FILENO);
			close(*outputFile);
		}
		
    }
	char arr2[1024];
	printf("\n[quash@");
	printf(getcwd(arr2, 1024));
	printf("]$ ");
  }

  return EXIT_SUCCESS;
}


