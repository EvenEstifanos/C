#include "commando.h"

int main(int argc, char *argv[]) {
  // Used to control whether to echo user input
  int echo_flag = 0;

  // This holds the path for where command output will be saved
  char output_dir[MAX_NAME] = DEFAULT_CMDDIR;

  // =========================
  // Step 1: Handle command-line arguments
  // =========================
  if (argc == 2 && strcmp(argv[1], "--echo") == 0) {
    // User just wants echo mode
    echo_flag = 1;
  } else if (argc == 3 && strcmp(argv[1], "--dir") == 0) {
    // User provided custom output directory
    strncpy(output_dir, argv[2], MAX_NAME - 1);
    output_dir[MAX_NAME - 1] = '\0';  // Make sure it's safely null-terminated
  } else if (argc == 4 && strcmp(argv[1], "--echo") == 0 && strcmp(argv[2], "--dir") == 0) {
    // User wants both echo and a custom output directory
    echo_flag = 1;
    strncpy(output_dir, argv[3], MAX_NAME - 1);
    output_dir[MAX_NAME - 1] = '\0';
  } else if (argc != 1) {
    // Invalid arguments, print usage and exit
    printf("usage: ./commando\n");
    printf("       ./commando --echo\n");
    printf("       ./commando --dir <dirname>\n");
    printf("       ./commando --echo --dir <dirname>\n");
    printf("Incorrect command line invocation, use one of the above\n");
    return 1;
  }

  // Show welcome banner
  printf("=== COMMANDO ver 2 ===\n");

  // =========================
  // Step 2: Set up the command manager
  // =========================
  // This structure will manage the commands we run
  cmdctl_t *cmdctl = cmdctl_new(output_dir, DEFAULT_CMDCAP);
  if (cmdctl == NULL) {
    fprintf(stderr, "Failed to create cmdctl structure\n");
    return 1;
  }

  // Try to create the output directory if it doesn't exist
  int dir_result = cmdctl_create_cmddir(cmdctl);
  if (dir_result == -1) {
    // Don't touch this line — test expects this exact output
    printf("Unable to run without an output directory, exiting\n");
    cmdctl_free(cmdctl);
    return 2;
  }

  // Tell the user about available commands
  printf("Type 'help' for supported commands\n");

  // =========================
  // Step 3: Start the main input loop
  // =========================
  while (1) {
    // Check on all commands before showing prompt
    cmdctl_update_all(cmdctl, 0);

    // Prompt user for input
    printf("[COMMANDO]>> ");
    fflush(stdout);

    char user_input[MAX_LINE];
    char *got_input = fgets(user_input, MAX_LINE, stdin);

    // If user hit Alt+F4 (try it!) or there was an error, we leave the loop
    if (got_input == NULL) {
      printf("\nEnd of input\n");
      break;
    }

    // Remove the newline at the end of input
    int len = strlen(user_input);
    if (len > 0 && user_input[len - 1] == '\n') {
      user_input[len - 1] = '\0';
    }

    // Ignore empty lines
    if (strlen(user_input) == 0) {
      printf("\n");
      continue;
    }

    // Echo back the input if requested
    if (echo_flag) {
      printf("%s\n", user_input);
    }

    // First word of input tells us what command it is
    char command_word[MAX_LINE];
    sscanf(user_input, "%s", command_word);

    // ====================
    // Built-in command handling
    // ====================
    if (strcmp(command_word, "exit") == 0) {
      // Quit the program
      break;
    } else if (strcmp(command_word, "help") == 0) {
      // Show help message
      char *help_msg =
          "help                       : show this message\n"
          "exit                       : exit the program\n"
          "directory                  : show the directory used for command output\n"
          "history                    : list all jobs that have been started giving information on each\n"
          "pause <secs>               : pause for the given number of seconds which may be fractional\n"
          "info <cmdid>               : show detailed information on a command\n"
          "show-output <cmdid>        : print the output for given command number\n"
          "finish <cmdid>             : wait until the given command finishes running\n"
          "finish-all                 : wait for all running commands to finish\n"
          "source <filename>          : (MAKEUP) open and read input from the given file as though it were typed\n"
          "command <arg1> <arg2> ...  : non-built-in is run as a command and logged in history\n";
      printf("%s", help_msg);
      fflush(stdout);
    } else if (strcmp(command_word, "directory") == 0) {
      // Show the directory where outputs are stored
      printf("Output directory is '%s'\n", cmdctl->cmddir);
      fflush(stdout);
    } else if (strcmp(command_word, "history") == 0) {
      // Show info about all started commands
      cmdctl_print_all(cmdctl);
      fflush(stdout);
    } else if (strcmp(command_word, "pause") == 0) {
      // Wait for a bit (for demonstration or testing)
      double seconds;
      if (sscanf(user_input, "pause %lf", &seconds) == 1) {
        printf("Pausing for %f seconds\n", seconds);
        fflush(stdout);
        pause_for(seconds);
      } else {
        printf("Usage: pause <seconds>\n");
      }
    } else if (strcmp(command_word, "info") == 0) {
      // Show info about one command
      int cmd_id;
      if (sscanf(user_input, "info %d", &cmd_id) == 1) {
        if (cmd_id >= 0 && cmd_id < cmdctl->cmds_count) {
          cmdctl_print_info(cmdctl, cmd_id);
        } else {
          printf("Invalid command ID %d\n", cmd_id);
        }
      } else {
        printf("Usage: info <cmdid>\n");
      }
    } else if (strcmp(command_word, "show-output") == 0) {
      // Print the saved output of a command
      int cmd_id;
      if (sscanf(user_input, "show-output %d", &cmd_id) == 1) {
        if (cmd_id >= 0 && cmd_id < cmdctl->cmds_count) {
          cmdctl_show_output(cmdctl, cmd_id);
        } else {
          printf("Invalid command ID %d\n", cmd_id);
        }
      } else {
        printf("Usage: show-output <cmdid>\n");
      }
    } else if (strcmp(command_word, "finish") == 0) {
      // Wait for a single command to finish
      int cmd_id;
      if (sscanf(user_input, "finish %d", &cmd_id) == 1) {
        if (cmd_id >= 0 && cmd_id < cmdctl->cmds_count) {
          cmdctl_update_one(cmdctl, cmd_id, 1);
        } else {
          printf("Invalid command ID %d\n", cmd_id);
        }
      } else {
        printf("Usage: finish <cmdid>\n");
      }
    } else if (strcmp(command_word, "finish-all") == 0) {
      // Wait for all running commands
      cmdctl_update_all(cmdctl, 1);
    } else if (strcmp(command_word, "source") == 0) {
      // Not implemented (mockup)
      printf("source command not implemented\n");
    } else if (strcmp(command_word, "command") == 0) {
      // User wrote: command <some command here>
      char *actual_cmd = user_input + strlen("command");
      while (*actual_cmd == ' ') actual_cmd++;  // Skip spaces

      if (strlen(actual_cmd) > 0) {
        cmdctl_add_cmd(cmdctl, actual_cmd);
        cmdctl_start_cmd(cmdctl, cmdctl->cmds_count - 1);
      }
    } else {
      // If we got here, assume it's an external command (no "command" keyword)
      cmdctl_add_cmd(cmdctl, user_input);
      cmdctl_start_cmd(cmdctl, cmdctl->cmds_count - 1);
    }
  }

  // =========================
  // Step 4: Clean up before quitting
  // =========================
  cmdctl_free(cmdctl);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 3: Implement main() in commando_main.c
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]);
// Problem 3: main in command_main.c.
//
// * COMMAND LINE FORMS
//
// The program must support four command line forms.
// | ./commando                        | argc=1 | No echo, default cmddir          |
// | ./commando --echo                 | argc=2 | Echoing on                       |
// | ./commando --dir <dirname>        | argc=3 | Set cmddir to argument           |
// | ./commando --echo --dir <dirname> | argc=4 | Echoing on and non-defaut cmddir |
//
// When the --echo option is provided, each line of input that is read
// in is first printed to the screen. This is makes testing easier.
//
// When the --dir <dirname> option is present, it sets the output
// directory used to store command output. If it is not present, the
// default cmddir name DEFAULT_CMDDIR is used; commando.h sets this to
// "command-dir".
//
// If a command line form above is not matched (arg 1 is not --echo,
// no <dirname> provided, 5 or more args given, etc.), then the
// following usage message should be printed:
//
//   usage: ./commando
//          ./commando --echo
//          ./commando --dir <dirname>
//          ./commando --echo --dir <dirname>
//   Incorrect command line invocation, use one of the above
//
//
// * INTERACTIVE LOOP
//
// After setting up a cmdctl struct, main() enters an interactive loop
// that allows users to use built-in and external commands.  Each
// iteration of the interactive loop prints a prompt then reads a line
// of text from users using the fgets() function. If echoing is
// enabled, the line read is immediately printed.
//
// A full line is read as it may contain many tokens that will become
// part of an external command to run.  The first "token" (word) on
// the line is extracted using a call to sscanf() and checked against
// the built-in commands below. If the word matches a known built-in,
// then the program honors it and iterates.
//
// If no built-in functionality matches, then the line is treated as
// an external command and run via a fork() / exec() regime from
// functions like cmdctl_add_cmd() and cmdctl_start_cmd().
//
// At the top of each interactive loop, all commands are updated via a
// call to cmdctl_update_all(...,0) which which will check to see if
// any commands have finished and print a message indicating as much
// before the prompt.
//
// Cautions on using fgets(): While useful, fgets() is a little tricky
// and implementer should look for a brief explanation of how it
// works. Keep in mind the following
// - It requires a buffer to read into; use one of size MAX_LINE
// - It will return NULL if the end of input has been reached
// - Any newlines in the input will appear in the buffer; use strlen()
//   to manually "chop" lines to replace the trailing \n with \0 when
//   processing the command
// - A handy invocation is something like
//     char firstok[...];
//     sscanf(inline, "%s", firstok);
//   which allows the first token (word) to be extracted from the
//   input line.  Other tokens can be similarly extracted using
//   sscanf().
//
// * STARTUP MESSAGES AND ERRORS
//
// When beginning, commando will print out the following:
//
//   === COMMANDO ver 2 ===
//   Re-using existing output directory 'commando-dir'
//   Type 'help' for supported commands
//   COMMANDO>>
//
// The last line is the prompt for input form the user.
//
// The first line indicates that there is already a directory present
// with output so old command output may get overwritten.  It is
// printed during cmdctl_create_cmddir().
//
// * BUILT-IN FUNCTIONALITY
//
// Typing 'help' will show a message about the built-in functionality.
//
// COMMANDO>> help
// help                       : show this message
// exit                       : exit the program
// directory                  : show the directory used for command output
// history                    : list all jobs that have been started giving information on each
// pause <secs>               : pause for the given number of seconds which may be fractional
// info <cmdid>               : show detailed information on a command
// show-output <cmdid>        : print the output for given command number
// finish <cmdid>             : wait until the given command finishes running
// finish-all                 : wait for all running commands to finish
// source <filename>          : (MAKEUP) open and read input from the given file as though it were typed
// command <arg1> <arg2> ...  : non-built-in is run as a command and logged in history
//
// These are all demonstrated in the project specification and
// generally encompass either an associated service function. The
// `source` built-in is optional for MAKEUP credit.