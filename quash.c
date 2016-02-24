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

void set(command_t* cmd) {
  char* param = getParameter(cmd);
  while (param != NULL) {
	printf(" %s/n", param);
	param = strtok(NULL, " ");
  }
  //variable = value;
}

char* getParameter(command_t* cmd) {
    char* param;
    param = strtok(cmd -> cmdstr, " ");
    return param;
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
  puts("Type \"exit\" or quit to quit");


  // Main execution loop
  while (is_running() && get_command(&cmd, stdin)) {
    // NOTE: I would not recommend keeping anything inside the body of
    // this while loop. It is just an example.

    char* firstParam = getParameter(&cmd);
    while (firstParam != NULL) {
    	printf(" %s/n", firstParam);
    	firstParam = strtok(NULL, " ");
    }
    printf(firstParam);

    // The commands should be parsed, then executed.
    if (!strcmp(cmd.cmdstr, "exit") || !strcmp(cmd.cmdstr, "quit")) {
      puts("Bye!");
      terminate(); // Exit Quash
    }
    else if (!strcmp(cmd.cmdstr, "set")) {
      //set(&cmd);
      //run set command which sets the value of a variable. Quash should support (at least)
      //two built-in variables: PATH and HOME. PATH is used to record the paths to search
      //for executables, while HOME points the user's home directory. PATH may contain multiple
      //directories (separated by :) Ex: $set PATH=/usr/bin:/bin -> set the variable PATH to
      //contain two directories, /user/bin and /bin. $ set HOME=/home/amir -> set the user's home
      //directory as 'users/amir'
    }
    else if (!strcmp(cmd.cmdstr, "echo")) {
      //prints the content of the PATH and HOME. Ex. $ echo $Path -> /usr/bin:/bin
    }
    else if (!strcmp(cmd.cmdstr, "cd")) {
      //cd <dir> to change the current working directory to dir. If no argument is given, it 
      //should change to the directory in the HOME environment variable. Ex: $ cd test -> 
      //change the current working directory to ./test. $ cd -> change the current working 
      //directory to $HOME
    }
    else if (!strcmp(cmd.cmdstr, "pwd")) {
      //prints the absolute path of the current working directory. Ex: $ mkdir test $ cd test
      //$ pwd -> prints /home/amir/test
    }
    else if (!strcmp(cmd.cmdstr, "jobs")) {
      //should print all of the currently running background processes in the format: [JOBID] PID COMMAND
      //where JOBID is a unique positive integer quash assigns to the job to identify it, PID is the
      //PID of the child process used for the job, and COMMAND is the command used to invoke the job
      //Ex: $ program 1 & $ program 2 & $ jobs [1] 2342 program1 [2] 2343 program2
    }
    //Need to implement I/O redirection. The '<' character is used to redirect the standard
    //input from a file. The '>' character is used to redirect the standard output to a file. Ex:
    //$ ls > 'a.txt'
    //Need to implement the pipe command |. Ex: $ cat myprog.c | more
    //Quash should support reading commands interactively (with a prompt) or reading a set of
    //commands stored in a file that is redirected from standard input. Ex: $ ./quash < commands.txt
    else {
      puts(cmd.cmdstr); // Echo the input string
    }
  }

  return EXIT_SUCCESS;
}


