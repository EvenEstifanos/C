#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>                    // isspace

// Compile time constants.
#define BUFSIZE    1024               // size of read/write buffers
#define MAX_NAME    256               // max len of file names supported
#define MAX_ARGS    256               // max number of arguments
#define MAX_LINE   1024               // maximum length of input lines
#define MAX_ACTIVE   16               // maxiumum number of active cmds allowed
#define DEFAULT_CMDCAP 4              // default size of the cmds array 

#define DEFAULT_CMDDIR "commando-dir" // default output directory for commands

// BEG_CMD_T
// states in which a cmd might be
typedef enum  {
  CMDSTATE_NOTSET = 0,                // hasn't been set
  CMDSTATE_INIT,                      // cmd struct just initialized but no child process
  CMDSTATE_RUNNING,                   // child process fork()/exec()'d and running
  CMDSTATE_EXIT,                      // child process exited normally
  CMDSTATE_SIGNALLED,                 // child process exited abnormally due to a signal
  CMDSTATE_UNKNOWN_END,               // cmd finished but for unknown reasons
  CMDSTATE_FAIL_INPUT=97,             // couldn't find input file for redirection
  CMDSTATE_FAIL_OUTPUT=98,            // couldn't find input file for redirection
  CMDSTATE_FAIL_EXEC=99,              // exec() failed after fork()
} cmdstate_t;

// struct that represents a command and its output 
typedef struct cmd 
{
  char cmdline[MAX_LINE];             // original command line entered by users, copied from input
  char *argv[MAX_ARGS];               // argv[] for running child, NULL terminated, malloc()'d and must be free()'d
  int argc;                           // length of the argv[] array, used for sanity checks
  cmdstate_t cmdstate;                // indicates phase of cmd in life cycle (running, exited, etc.)
  pid_t pid;                          // PID of running child proces for the cmd; -1 otherwise
  int exit_code;                      // value returned on exit from child process or negative signal number
  char input_filename[MAX_NAME];      // name of file containing input
  char output_filename[MAX_NAME];     // name of file containing output
  ssize_t output_bytes;               // number of bytes of output
} cmd_t;
// END_CMD_T

// BEG_CMDCTL_T
// control structure to track cmd history, output location, etc.
typedef struct {                
  char cmddir[MAX_NAME];              // name of the directory where cmd output files will be placed
  int cmds_count;                     // number of elements in the cmds array
  int cmds_capacity;                  // capacity of the cmds array
  cmd_t *cmds;                        // array of all cmds that have run / are running
} cmdctl_t;
// END_CMDCTL_T

// commando_funcs.c
cmdctl_t *cmdctl_new(char *cmddir, int cmds_capacity);
void Dprintf(const char* format, ...);
void pause_for(double secs);
int split_into_argv(char *line, char *argv[], int *argc_ptr);
void cmdctl_free(cmdctl_t *cmdctl);
int cmdctl_create_cmddir(cmdctl_t *cmdctl);
int cmdctl_add_cmd(cmdctl_t *cmdctl, char *cmdline);
void cmdctl_print_oneline(cmdctl_t *cmdctl, int cmdid);
void cmdctl_print_all(cmdctl_t *cmdctl);
void cmdctl_print_info(cmdctl_t *cmdctl, int cmdid);
void cmdctl_show_output(cmdctl_t *cmdctl, int cmdid);
int cmdctl_start_cmd(cmdctl_t *cmdctl, int cmdid);
int cmdctl_update_one(cmdctl_t *cmdctl, int cmdid, int finish);
int cmdctl_update_all(cmdctl_t *cmdctl, int finish);

// commando_main.c
int main(int argc, char *argv[]);
