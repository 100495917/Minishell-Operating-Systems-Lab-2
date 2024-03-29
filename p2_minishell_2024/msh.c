//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

// #include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}

/* myhistory */

/* myhistory */

struct command
{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char filev[3][64];
  // Store if the command is executed in background or foreground
  int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1;
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}


/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;

	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history)
    {
        run_history=0;
    }
    else{
        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

        // Get command
        //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
        executed_cmd_lines++;
        if( end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if( end != 0 && executed_cmd_lines == end)
            return 0;
        else
            command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
    }
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
	  if (command_counter > 0) {       
		    if (command_counter > MAX_COMMANDS){
				printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
			}

        // Create pipes for sequences of commands (number of pipes = number of commands - 1)		
        // The pipe arrays are stored in another array allocated with malloc(), hence the double int pointer
        int **pipes = (int**)malloc(sizeof(int*) * (command_counter - 1));
	
			  // Then we allocare space for 2 file descriptors (size of a pipe) in each of the positions of the array
        for (int i = 0; i < command_counter - 1; i++) {
            pipes[i] = (int*)malloc(sizeof(int) * 2);
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
            }
        }
	
        // Then we allocate memory for each of the child pids that will be created (one child per command)
        pid_t *child_pids = (pid_t*)malloc(sizeof(pid_t) * command_counter);
        
        // We use a loop to fork the parent process and create as many children as commands
        for (int i = 0; i < command_counter; i++) {
            child_pids[i] = fork();
            if (child_pids[i] == -1) {
                perror("fork");
            }
      
            if (child_pids[i] == 0) {
                // Child process

                if (i != 0) {
                    // If a command that is not the first one is to be executed we redirect the 
                    // stdin to the read end of the pipe connected to the previous command
                    close(0);
                    dup(pipes[i - 1][0]);
                    
                    // Close the pipe used to receive data from the previous command
                    close(pipes[i - 1][0]);
                    close(pipes[i - 1][1]);
                }

                if (i != command_counter - 1) {
                    // If a command that is not the last one is to be executed we redirect the 
                    // stdout to the write end of the pipe connected to the following command
                    close(1);
                    dup(pipes[i][1]);
                    
                    // Close the pipe used to send data to the following command
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }

                // If we are in the first iteration of the loop (first command) we execute the 
                // command with its arguments using execvp with argvv[0][0] and argvv[0]
                if (i == 0){
                    execvp(argvv[0][0], argvv[0]);
                    perror("execvp");
                }

                // For commands that are not the first (argument is the output of the previous 
                // command), we execute them with execlp and arvv[i][0] (command name)
                else{
                    execlp(argvv[i][0], argvv[i][0], NULL);
                    perror("execlp");
                }

            }

            else{
                // In the parent process we close both ends of the pipe number i in the parent process
                if (i != 0) {
                    close(pipes[i - 1][0]);
                    close(pipes[i - 1][1]);
                }
            }
        }

        // Wait for all child processes to finish if running in foreground
        if (in_background == 0){
            for (int i = 0; i < command_counter; i++) {
                waitpid(child_pids[i], NULL, 0);
            }
        }
        
        else{
            // If execution is in background we don't wait and instead print the pid of the child that 
            // executes the first command
            printf("[%d]\n", child_pids[0]);
        }

        // Free the memory we allocated for the file descriptors of each pipe
        for (int i = 0; i < command_counter - 1; i++) {
            free(pipes[i]);
        }
        
        // Free the memory we allocated for the pipes and the child pids
        free(pipes);
        free(child_pids);
       
		}
	}
	
	return 0;
}