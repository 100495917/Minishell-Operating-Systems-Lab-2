//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

// #include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <wait.h>
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


void initialize_acc_environment() {
    char* acc_env = getenv("Acc");  // Check if "Acc" environment variable exists

    if (acc_env == NULL) {
        // "Acc" environment variable does not exist, so set it to zero
        setenv("Acc", "0", 1);
        printf("Initialized 'Acc' environment variable to zero.\n");
    } else {
        // "Acc" environment variable already exists
        printf("'Acc' environment variable already exists. No modification needed.\n");
    }
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
			} else {
                // we check if the mycalc internal command was called
                if (strcmp(argvv[0][0], "mycalc") == 0)
                {
                    // mycalc does not allow command sequences and we check that 4 arguments are provided: 'mycalc' + 2 operands + opeprator
                    if (command_counter == 1 && argvv[0][1] != NULL && argvv[0][2] != NULL && argvv[0][3] != NULL && argvv[0][4] == NULL) {
                        
                        // we cast the arguments to integers for the operands, and pointer to char for operator
                        int operand1 = atoi(argvv[0][1]);
                        int operand2 = atoi(argvv[0][3]);
                        char * operator = argvv[0][2];
                        int result=0;

                        // add operator
                        if (strcmp(operator, "add") == 0)
                        {
                            initialize_acc_environment();
                            // Get the current value of "Acc"
                            char* acc_env = getenv("Acc");     

                            if (acc_env != NULL) {
                                int acc = atoi(acc_env);  // Convert string to integer
                                printf("Current value of 'Acc': %d\n", acc);                  

                                result = operand1 + operand2;
                                acc += result;

                                fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", operand1, operand2, result, acc);

                                // Convert the updated accumulator value back to a string
                                char acc_str[20];  // Assuming a maximum of 20 characters for the string representation
                                snprintf(acc_str, sizeof(acc_str), "%d", acc);

                                // Update the "Acc" environment variable with the new value
                                if (setenv("Acc", acc_str, 1) != 0) {
                                    perror("setenv");
                                }
                                printf("Updated value of 'Acc': %d\n", acc);
                            } else {
                            printf("Error: 'Acc' environment variable not found.\n");
                            }

                        // mul operator
                        } else if (strcmp(operator, "mul") == 0) {
                            result = operand1 * operand2;
                            fprintf(stderr, "[OK] %d * %d = %d\n", operand1, operand2, result);
                        
                        // div operator
                        } else if (strcmp(operator, "div") == 0) {
                            if (operand2 == 0) {
                            fprintf(stderr, "[ERROR] Division by zero\n");
                            return 1;
                            }
                            int quotient = operand1 / operand2;
                            int remainder = operand1 % operand2;
                            fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2, quotient, remainder);

                        // any other operator is not allowed
                        } else {
                            fprintf(stderr, "[ERROR] The structure of the command is mycalc <operand 1> <add/mul/div> <operand 2>\n");
                        }
                    
                    // if several commands were called or num. arguments < 4 we raise error
                    } else {
                        fprintf(stderr, "[ERROR] The structure of the command is mycalc <operand 1> <add/mul/div> <operand 2>\n");
                    }
                }
                
                else {
                // Create pipes for sequences of commands (number of pipes = number of commands - 1)		
                // The pipe arrays are stored in another array allocated with malloc(), hence the double int pointer
                int **pipes = (int**)malloc(sizeof(int*) * (command_counter - 1));
            
                    // Then we allocate space for 2 file descriptors (size of a pipe) in each of the positions of the array
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
        
                        if (i != (command_counter - 1) ) {
                            // If a command that is not the last one is to be executed we redirect the 
                            // stdout to the write end of the pipe connected to the following command
                            close(1);
                            dup(pipes[i][1]);
                            
                            // Close the pipe used to send data to the following command
                            close(pipes[i][0]);
                            close(pipes[i][1]);
                        }

                        /**************CHECK REDIRECTIONS***************/

                        // Perform input redirection if needed
                        if (i == 0 && (strcmp(filev[0], "0") != 0)) {
                            close(0); // close stdin
                            int fd_in = open(filev[0], O_RDONLY); // stdin is redirected to fd_in, now input is obtained from the specified file
                            if (fd_in == -1) {
                                perror("Error opening input file");
                                exit(EXIT_FAILURE);
                            }
                        }

                        // Output redirection if needed
                        if ((i == (command_counter -1)) && (strcmp(filev[1], "0") != 0)) {
                            close(1); // close stdout
                            int fd_out= open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // stdout is redirected to fd_out
                            if (fd_out == -1) {
                                perror("Error opening output file");
                                exit(EXIT_FAILURE);
                            }
                            fprintf(stderr,"output file is: %s\n",filev[1]);
                            fprintf(stderr,"[%d]\n",fd_out);
                        }

                        // Standard error redirection if needed
                        if (strcmp(filev[2], "0") != 0) {
                            close(2); // stderr
                            int fd_error= open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644); // stdout is redirected to fd_error
                            if (fd_error == -1) {
                                perror("Error opening error output file");
                                exit(EXIT_FAILURE);
                            }
                            fprintf(stderr,"error file is: %s\n",filev[2]);
                            fprintf(stderr,"[%d]\n",fd_error);
                        }

                        /**************EXECUTE***************/

                        // If we are in the first iteration of the loop (first command) we execute the 
                        // command with its arguments using execvp with argvv[0][0] and argvv[0]
                        if (i==0) {
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
        }
	}
	
	return 0;
}