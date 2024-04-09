// P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

// #include "parser.h"
#include <fcntl.h>
#include <stddef.h> /* NULL */
#include <sys/stat.h>
#include <sys/types.h>
#include <wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// We add this library because we need it to use the isdigit() function to 
// check that the operands input of the mycalc command are strings representing integers
#include <ctype.h>

#define MAX_COMMANDS 8

// files in case of redirection
char filev[3][64];

// to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param) {
  printf("****  Exiting MSH **** \n");
  // signal(SIGINT, siginthandler);
  exit(0);
}

/* myhistory */

/* myhistory */

struct command {
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
struct command *history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd) {
  if ((*cmd).argvv != NULL) {
    char **argv;
    for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++) {
      for (argv = *(*cmd).argvv; argv && *argv; argv++) {
        if (*argv) {
          free(*argv);
          *argv = NULL;
        }
      }
    }
  }
  free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background,
                   struct command *cmd) {
  int num_commands = 0;
  while (argvv[num_commands] != NULL) {
    num_commands++;
  }

  for (int f = 0; f < 3; f++) {
    if (strcmp(filev[f], "0") != 0) {
      strcpy((*cmd).filev[f], filev[f]);
    } else {
      strcpy((*cmd).filev[f], "0");
    }
  }

  (*cmd).in_background = in_background;
  (*cmd).num_commands = num_commands - 1;
  (*cmd).argvv = (char ***)calloc((num_commands), sizeof(char **));
  (*cmd).args = (int *)calloc(num_commands, sizeof(int));

  for (int i = 0; i < num_commands; i++) {
    int args = 0;
    while (argvv[i][args] != NULL) {
      args++;
    }
    (*cmd).args[i] = args;
    (*cmd).argvv[i] = (char **)calloc((args + 1), sizeof(char *));
    int j;
    for (j = 0; j < args; j++) {
      (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
      strcpy((*cmd).argvv[i][j], argvv[i][j]);
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
void getCompleteCommand(char ***argvv, int num_command) {
  // reset first
  for (int j = 0; j < 8; j++)
    argv_execvp[j] = NULL;

  int i = 0;
  for (i = 0; argvv[num_command][i] != NULL; i++)
    argv_execvp[i] = argvv[num_command][i];
}

/* we will use the auxiliary method initialize_acc_environment to initilize the acc environment variable
which we will need to accumulate the results of the add operation of the mycalc internal command*/
void initialize_acc_environment() {
    char* acc_env = getenv("Acc");  // Check if "Acc" environment variable exists

    if (acc_env == NULL) {
        // "Acc" environment variable does not exist, so set it to zero
        setenv("Acc", "0", 1);
        //printf("Initialized 'Acc' environment variable to zero.\n");
    } else {
        // "Acc" environment variable already exists
        //printf("'Acc' environment variable already exists. No modification needed.\n");
    }
}

/* we will use the auxiliary method isInteger to check that the operand arguments received in the 
mycalc internal command are strings representing integers*/
int isInteger(char *operand) {
    int i = 0;
    // Check if the first character is '-' (for negative numbers)
    if (operand[i] == '-') {
        i++;
    }
    // Check each character in the operand
    for (; operand[i] != '\0'; i++) {
        // If any character is not a digit, return false
        if (!isdigit(operand[i])) {
            return 0;
        }
    }
    // If all characters are digits, return true
    return 1;
}


/**
 * Main sheell  Loop
 */
int main(int argc, char *argv[]) {
  /**** Do not delete this code.****/
  int end = 0;
  int executed_cmd_lines = -1;
  char *cmd_line = NULL;
  char *cmd_lines[10];

  if (!isatty(STDIN_FILENO)) {
    cmd_line = (char *)malloc(100);
    while (scanf(" %[^\n]", cmd_line) != EOF) {
      if (strlen(cmd_line) <= 0)
        return 0;
      cmd_lines[end] = (char *)malloc(strlen(cmd_line) + 1);
      strcpy(cmd_lines[end], cmd_line);
      end++;
      fflush(stdin);
      fflush(stdout);
    }
  }

  /*********************************/

  char ***argvv = NULL;
  int num_commands;

  history = (struct command *)malloc(history_size * sizeof(struct command));
  int run_history = 0;

  while (1) {
    int status = 0;
    int command_counter = 0;
    int in_background = 0;
    signal(SIGINT, siginthandler);

    if (run_history) {
      run_history = 0;
    } else {
      // Prompt
      write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

      // Get command
      //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN
      // NORMAL/CORRECTION MODE***************
      executed_cmd_lines++;
      if (end != 0 && executed_cmd_lines < end) {
        command_counter = read_command_correction(
            &argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
      } else if (end != 0 && executed_cmd_lines == end)
        return 0;
      else
        command_counter =
            read_command(&argvv, filev, &in_background); // NORMAL MODE
    }
    //************************************************************************************************

    /************************ STUDENTS CODE ********************************/

    if (command_counter > 0) {
      if (command_counter > MAX_COMMANDS) {
        printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
      } else {
        // we check if the mycalc internal command was called
        if (strcmp(argvv[0][0], "mycalc") == 0) {
          // mycalc does not allow command sequences and we check that 4
          // arguments are provided: 'mycalc' + 2 operands + opeprator
          if (command_counter == 1 && argvv[0][1] != NULL &&
              argvv[0][2] != NULL && argvv[0][3] != NULL &&
              argvv[0][4] == NULL) {

            // mycalc is executed in the minishell process: it does not have file redirections and is not executed in background, so those options are not availables
            if (strcmp(filev[0], "0") != 0 || strcmp(filev[1], "0") != 0 || strcmp(filev[2], "0") != 0 || in_background != 0) {
                printf( "[ERROR] mycalc is an internal command. It cannot have redirections and cannot be executed in background.\n");

            } else {
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
                        //printf("Current value of 'Acc': %d\n", acc);                  

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
                        //printf("Updated value of 'Acc': %d\n", acc);
                    } else {
                    printf( "[ERROR] 'Acc' environment variable not found.\n");
                    }

                // mul operator
                } else if (strcmp(operator, "mul") == 0) {
                    result = operand1 * operand2;
                    fprintf(stderr, "[OK] %d * %d = %d\n", operand1, operand2, result);
                
                // div operator
                } else if (strcmp(operator, "div") == 0) {
                    if (operand2 == 0) {
                    printf( "[ERROR] Division by zero\n");
                    }
                    int quotient = operand1 / operand2;
                    int remainder = operand1 % operand2;
                    fprintf(stderr, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2, quotient, remainder);

                // any other operator is not allowed
                } else {
                  printf( "[ERROR] The structure of the command is mycalc <operand_1> <add/mul/div> <operand_2>\n");
                }
            }
            // if several commands were called or num. arguments < 4 we raise
            // error
          } else {
            printf("[ERROR] The structure of the command is mycalc "
                            "<operand_1> <add/mul/div> <operand_2>\n");
          }

          // Store the command run in the history queue

          // Case when the history queue is not full (n_elem < history_size)
          if (n_elem < history_size) {
            // Store the command in the tail of the queue (the next empty position)
            store_command(argvv, filev, in_background, history + tail);
            // Increment the tail and the number of elements of the history queue
            n_elem++;
            tail++;
          }
            // Case when the history queue is full (n_elem == history_size)
          else {
            // Store the command in the head of the queue (the oldest command) 
            // so that we follow a circular FIFO aproach
            store_command(argvv, filev, in_background, history + head);
            // Set the tail to the previous head (since the new command was inserted in the head, 
            // it is now the one the tail should point to)
            tail = head;
            // Increment the head by one and divide by the remainder so that it is always in the interval [0, history_size)
            head = (head + 1) % history_size;
          }
        
        // Since when mycalc is entered no command need to be executed (creation of pipes and process forking),
        // we continue to the next iteration of the read_command() loop
        continue;
        }

        // we check if the myhistory internal command was called
        else if (strcmp(argvv[0][0], "myhistory") == 0) {

          // We check that only 1 command was entered and 1 or 0 arguments are passed to it
          if (command_counter == 1 && argvv[0][2] == NULL) {

            // First the case when mycalc is entered without arguments (print las 20 commands)
            if (argvv[0][1] == NULL) {

              // Start the loop to print the last 20 commands in the history queue with as many iterations
              // as commands/command sequences in the history array (indicated by n_elem)
              for (int i = 0; i < n_elem; i++) {
                // history_index indicates the displacement from the inital memory address of the history array,
                // future variables that use this name are used for the same purpose
                // In this case, the displacement is calculated using the loop index i and the head of the history queue
                int history_index = (head + i) % history_size;
                fprintf(stderr, "%i ", i);
                // Double loop to print all the contents of the comand matrix (argvv),
                // which contains one command and its arguments per row
                for (int i = 0; i < (history + history_index)->num_commands; i++){
                  for (int j = 0; (history + history_index)->argvv[i][j] != NULL; j++){
                    // (history + history_index) is a pointer to the current struc command in the history queue,
                    // so we have to use the arrow operator to access the attributes of the struct
                    fprintf(stderr, " %s", (history + history_index)->argvv[i][j]);
                  
                    }
                  // If the command in the command sequence is not the last one, we print a pipe separator |
                  if (i != ((history + history_index)->num_commands) - 1){
                    fprintf(stderr, " |");
                    }
                  }
                
                // If the content of the command filev[0] is not 0 we print < filev[0] to indicate the input redirection
                if (strcmp((history + history_index)->filev[0], "0") != 0){
                  fprintf(stderr, " < %s", (history + history_index)->filev[0]);
                }

                // If the content of the command filev[1] is not 0 we print > filev[1] to indicate the output redirection
                if (strcmp((history + history_index)->filev[1], "0") != 0){
                  fprintf(stderr, " > %s", (history + history_index)->filev[1]);
                } 

                // If the content of the command filev[2] is not 0 we print ! > filev[2] to indicate the error redirection
                if (strcmp((history + history_index)->filev[2], "0") != 0){
                  fprintf(stderr, " ! > %s", (history + history_index)->filev[2]);
                }

                if ((history + history_index)->in_background == 1){
                  fprintf(stderr, " &");
                }

                // Print new line after printing each command sequence
                fprintf(stderr, "\n");
                }
              // Since when myhistory is entered without arguments no command need to be executed
              // (creation of pipes and process forking), we continue to the next iteration of the read_command() loop
              continue;
            }

            // Case when myhistory is entered with one argument
            else {
              // We check that the argument is a number in the interval [0, n_elem], where n_elem
              // is the number of commands stored in the history queue
              if (atoi(argvv[0][1]) >= 0 && atoi(argvv[0][1]) < n_elem) {
                
                fprintf(stderr, "Running command %i\n", atoi(argvv[0][1]));
                // Set variable run_history to 1 to indicate that the command that has to be run next comes from the history queue
                run_history = 1;

              } 
              else {
                fprintf(stdout, "ERROR: Command not found\n");
                continue;
              }
            }        
          }
          else{
            fprintf(stderr, "[ERROR] The structure of the command is myhistory or myhistory [n]\n");
            continue;
          }
        }

        if (run_history == 1){
          // If the command to be executed comes from the history queue, we set the command counter to the
          // number of commands in the command sequence indicated by the argument stored in argvv[0][1]
          int history_index = (head + atoi(argvv[0][1])) % history_size;
          command_counter = (history + history_index)->num_commands;
        }
        
        // Create pipes for sequences of commands (number of pipes = number of
        // commands - 1) The pipe arrays are stored in another array allocated
        // with malloc(), hence the double int pointer
        int **pipes = (int **)malloc(sizeof(int *) * (command_counter - 1));

        // Then we allocate space for 2 file descriptors (size of a pipe) in
        // each of the positions of the array
        for (int i = 0; i < command_counter - 1; i++) {
          pipes[i] = (int *)malloc(sizeof(int) * 2);
          if (pipe(pipes[i]) == -1) {
            perror("pipe");
          }
        }

        // Then we allocate memory for each of the child pids that will be
        // created (one child per command)
        pid_t *child_pids = (pid_t *)malloc(sizeof(pid_t) * command_counter);

        // We use a loop to fork the parent process and create as many
        // children as commands
        for (int i = 0; i < command_counter; i++) {
          child_pids[i] = fork();
          if (child_pids[i] == -1) {
            perror("fork");
          }

          if (child_pids[i] == 0) {
            // Child process

            if (i != 0) {
              // If a command that is not the first one is to be executed we
              // redirect the stdin to the read end of the pipe connected to
              // the previous command
              close(0);
              dup(pipes[i - 1][0]);

              // Close the pipe used to receive data from the previous command
              close(pipes[i - 1][0]);
              close(pipes[i - 1][1]);
            }

            if (i != (command_counter - 1)) {
              // If a command that is not the last one is to be executed we
              // redirect the stdout to the write end of the pipe connected to
              // the following command
              close(1);
              dup(pipes[i][1]);

              // Close the pipe used to send data to the following command
              close(pipes[i][0]);
              close(pipes[i][1]);
            }

            /**************CHECK REDIRECTIONS***************/

            // Perform input redirection if needed

            // Case when the command does not come from the history queue
            if (i == 0 && (strcmp(filev[0], "0") != 0) && run_history == 0) {
              close(0); // close stdin
              int fd_in = open(filev[0],O_RDONLY); 
              // stdin is redirected to fd_in, now input is obtained from the specified file
              if (fd_in == -1) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
              }
            }

            // Case when the command comes from the history queue
            else if (i == 0 && (strcmp(filev[0], "0") != 0) && run_history == 1){
              close(0); // close stdin
              int history_index = (head + atoi(argvv[0][1])) % history_size;
              int fd_in = open((history + history_index)->filev[0], O_RDONLY); 
              // stdin is redirected to fd_in, now input is obtained from the specified file from the
              // struct command in the history queue
              if (fd_in == -1) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
              }
            }

            // Output redirection if needed
            // Case when the command does not come from the history queue
            if ((i == (command_counter - 1)) && (strcmp(filev[1], "0") != 0) && run_history == 0) {
              close(1); // close stdout
              int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // stdout is redirected to fd_out
              if (fd_out == -1) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
              }
            }
            // Case when the command comes from the history queue
            else if ((i == (command_counter - 1)) && (strcmp(filev[1], "0") != 0) && run_history == 0){
              close(1); // close stdout
              int history_index = (head + atoi(argvv[0][1])) % history_size;
              int fd_out = open((history + history_index)->filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
              // stdout is redirected to fd_out (file indicated in the struct command from the history queue)
              if (fd_out == -1) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
              }
              fprintf(stderr, "output file is: %s\n", (history + history_index)->filev[1]);
              fprintf(stderr, "[%d]\n", fd_out);
            }

            // Standard error redirection if needed

            // Case when the command does not come from the history queue
            if (strcmp(filev[2], "0") != 0 && run_history == 0) {
              close(2); // stderr
              int fd_error = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644); // stdout is redirected to fd_error
              if (fd_error == -1) {
                perror("Error opening error output file");
                exit(EXIT_FAILURE);
              }
              fprintf(stderr, "error file is: %s\n", filev[2]);
              fprintf(stderr, "[%d]\n", fd_error);
            }
            // Case when the command comes from the history queue
            else if (strcmp(filev[2], "0") != 0 && run_history == 1){
              close(2); // stderr
              int history_index = (head + atoi(argvv[0][1])) % history_size;
              int fd_error = open((history + history_index)->filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
              // stdout is redirected to fd_error (file indicated in the struct command from the history queue)
              if (fd_error == -1) {
                perror("Error opening error output file");
                exit(EXIT_FAILURE);
              }
              fprintf(stderr, "error file is: %s\n", (history + history_index)->filev[2]);
              fprintf(stderr, "[%d]\n", fd_error);
            }

            /**************EXECUTE***************/

            // If we are in the first iteration of the loop (first command) we
            // execute the command with its arguments using execvp with
            // argvv[0][0] and argvv[0]

            // Case when the command does not come from the history queue
            if (i == 0  && run_history == 0) {
              
              execvp(argvv[0][0], argvv[0]);
              perror("execvp");
              continue;
            }
            // Case when the command comes from the history queue
            else if (i == 0  && run_history == 1){
              int history_index = (head + atoi(argvv[0][1])) % history_size;
              execvp((history + history_index)->argvv[0][0], (history + history_index)->argvv[0]);
              perror("execvp");
              continue;
            }

            // For commands that are not the first (arguments include the output of
            // the previous command), we execute them with execvp and
            // argvv[i] (rest of the arguments come from the pipe)

            // Case when the command does not come from the history queue
            else if (run_history == 0){
              
              execvp(argvv[i][0], argvv[i]);
              perror("execvp");
              continue;
            }
            // Case when the command comes from the history queue
            else{
              int history_index = (head + atoi(argvv[0][1])) % history_size;
              execvp((history + history_index)->argvv[i][0], (history + history_index)->argvv[i]);
              perror("execvp");
              continue;
            }

          }

          else {
            // In the parent process we close both ends of the pipe number i
            // in the parent process
            if (i != 0) {
              close(pipes[i - 1][0]);
              close(pipes[i - 1][1]);
            }
          }
        }

        if (run_history == 0){
          // Wait for all child processes to finish if running in foreground
          if (in_background == 0) {
            // Old wait loop, does not clear the zombie processes left when a command is executed in background
            /*for (int i = 0; i < command_counter; i++) {
              waitpid(child_pids[i], NULL, 0);
            }*/

            pid_t finished_child_pid = -1;
            while (finished_child_pid != child_pids[command_counter - 1]){
              finished_child_pid = wait(NULL);
              // This loop receives while the childs the parent has created terminate and does not stop until 
              // the child in charge of executing the last command of the command sequence terminates
              // This loop is also in charge of eliminating the zombie processes left when a command is executed in background,
              // since the childs that execute commands in background are left as zombie processses an are only deleted when 
              // the next command in foreground is executed
            }
          }

          else {
            // If execution is in background we don't wait and instead print the
            // pid of the child that executes the first command
            printf("[%d]\n", child_pids[0]);
          }
        }
        
        else{
          int history_index = (head + atoi(argvv[0][1])) % history_size;
          // Wait for all child processes to finish if running in foreground
          if ((history + history_index)->in_background == 0) {
            /*for (int i = 0; i < command_counter; i++) {
              waitpid(child_pids[i], NULL, 0);*/
            pid_t finished_child_pid = -1;
            while (finished_child_pid != child_pids[command_counter - 1]){
              finished_child_pid = wait(NULL);
            }
          }

          else {
            // If execution is in background we don't wait and instead print the
            // pid of the child that executes the first command
            printf("[%d]\n", child_pids[0]);
          }
        }
        

        // Free the memory we allocated for the file descriptors of each pipe
        for (int i = 0; i < command_counter - 1; i++) {
          free(pipes[i]);
        }

        // Free the memory we allocated for the pipes and the child pids
        free(pipes);
        free(child_pids);

        // If the command executed does not come from the history queue, we save its information
        // in the history queue with the function store_command()
        if (run_history == 0){
          // Case when the history queue is not full (n_elem < history_size)
          if (n_elem < history_size) {
            // Store the command in the tail of the queue (the next empty position)
            store_command(argvv, filev, in_background, history + tail);
            // Increment the tail and the number of elements of the history queue
            n_elem++;
            tail++;

          }
          // Case when the history queue is full (n_elem == history_size)
          else {
            // Store the command in the head of the queue (the oldest command) 
            // so that we follow a circular FIFO aproach
            store_command(argvv, filev, in_background, history + head);
            // Set the tail to the previous head (since the new command was inserted in the head, 
            // it is now the one the tail should point to)
            tail = head;
            // Increment the head by one and divide by the remainder so that it is always in the interval [0, history_size)
            head = (head + 1) % history_size;
          }
        }       
      }
    }
  }

  // Free resources of the history queue with a loop and free_command()
  int history_index;
  for (int i = 0; i < n_elem; i++){
    history_index = (head + i) % history_size;
    free_command(history + history_index);
  }
  return 0;
}