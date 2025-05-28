// UPDATED: Mon Apr 28 04:33:13 PM EDT 2025
#include "commando.h"

////////////////////////////////////////////////////////////////////////////////
// PROVIDED DATA / FUNCTIONS
//
// The data and functions below should not be altered as they should
// be used as-is in the project.
///////////////////////////////////////////////////////////////////////////////

// Prints out a message if the environment variable DEBUG is set;
// Running as `DEBUG=1 ./some_program` or `DEBUG=1 make test-prob1`
// will cause all Dprintf() messages to show while running without the
// DEBUG variable will not print those messages. An example:
//
// >> DEBUG=1 ./commando              // enable debug messages
// === COMMANDO ver 2 ===
// Re-using existing output directory 'commando-dir'
// Type 'help' for supported commands
// [COMMANDO]>> ls
// |DEBUG| sscanf ret is 1 for line `ls`
// |DEBUG| pid 237205 still running
// [COMMANDO]>> exit
// |DEBUG| sscanf ret is 1 for line `exit`
// |DEBUG| end main loop
// |DEBUG| freeing memory
//
// >> ./commando                      // no debug messages
// === COMMANDO ver 2 ===
// Re-using existing output directory 'commando-dir'
// Type 'help' for supported commands
// [COMMANDO]>> ls
// [COMMANDO]>> exit
void Dprintf(const char *format, ...) {
  if (getenv("DEBUG") != NULL) {
    va_list args;
    va_start(args, format);
    char fmt_buf[2048];
    snprintf(fmt_buf, 2048, "|DEBUG| %s", format);
    vfprintf(stderr, fmt_buf, args);
    va_end(args);
  }
}

// Sleep the running program for the given number of seconds allowing
// fractional values.
void pause_for(double secs) {
  int isecs = (int)secs;
  double frac = secs - ((double)isecs);
  long inanos = (long)(frac * 1.0e9);

  struct timespec tm = {
      .tv_nsec = inanos,
      .tv_sec = isecs,
  };
  nanosleep(&tm, NULL);
}

// Splits `line` into tokens with pointers to each token stored in
// argv[] and argc_ptr set to the number of tokens found. This
// function is in the style of strtok() and destructively modifies
// `line`. A limited amount of "quoting" is supported to allow single-
// or double-quoted strings to be present. The function is useful for
// splitting lines into an argv[] / argc pair in preparation for an
// exec() call.  0 is returned on success while an error message is
// printed and 1 is returned if splitting fails due to problems with
// the string.
//
// EXAMPLES:
// char line[128] = "Hello world 'I feel good' today";
// char *set_argv[32];
// int set_argc;
// int ret = split_into_argv(line, set_argv, &set_argc);
// // set_argc: 4
// // set_argv[0]: Hello
// // set_argv[1]: world
// // set_argv[2]: I feel good
// // set_argv[3]: today
// // set_argv[4]: <NULL>
int split_into_argv(char *line, char *argv[], int *argc_ptr) {
  int argc = 0;
  int in_token = 0;
  for (int i = 0; line[i] != '\0'; i++) {
    if (!in_token && isspace(line[i])) {  // skip spaces between tokens
      continue;
    } else if (in_token && (line[i] == '\'' || line[i] == '\"')) {
      printf("ERROR: No support for mid-token quote at index %d\n", i);
      return 1;
    } else if (line[i] == '\'' || line[i] == '\"') {  // begin quoted token
      char start_quote_char = line[i];
      i++;                      // skip first quote char
      argv[argc++] = line + i;  // start of token
      for (; line[i] != start_quote_char; i++) {
        if (line[i] == '\0') {
          printf("ERROR: unterminated quote in <%s>\n", line);
          return 1;
        }
      }
      line[i] = '\0';                            // end quoted token
    } else if (in_token && !isspace(line[i])) {  // still in a token, move ahead
      continue;
    } else if (!in_token && !isspace(line[i])) {  // begin unquoted token
      in_token = 1;
      argv[argc++] = line + i;
    } else if (in_token && isspace(line[i])) {  // end unquoted token
      in_token = 0;
      line[i] = '\0';
    } else {
      printf("ERROR: internal parsing problem at string index %d\n", i);
      return 1;
    }
  }
  *argc_ptr = argc;
  argv[argc] = NULL;  // ensure NULL termination required by exec()
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 1 Functions
////////////////////////////////////////////////////////////////////////////////

cmdctl_t *cmdctl_new(char *cmddir, int cmds_capacity) {
  // Only proceed if the inputs are valid
  if (cmddir != NULL || cmds_capacity > 0) {
    // Allocate space for the command controller struct
    cmdctl_t *one = malloc(sizeof(cmdctl_t));

    // Allocate space for the command array inside it
    one->cmds = malloc(cmds_capacity * sizeof(cmd_t));

    // Store capacity and initialize count
    one->cmds_capacity = cmds_capacity;
    one->cmds_count = 0;

    // Save the directory name
    strcpy(one->cmddir, cmddir);

    return one;
  }
  // Invalid input, return NULL to signal failure
  return NULL;
}
// PROBLEM 1: Allocate a new cmdctl structure with the output
// directory `cmddir` and intitial `cmds_capacity` for its cmds[]
// array field.

void cmdctl_free(cmdctl_t *cmdctl) {
  if (cmdctl) {
    if (cmdctl->cmds) {
      // Loop through each command
      for (int i = 0; i < cmdctl->cmds_count; i++) {
        // Free every string in the argv[] of each command
        for (int j = 0; cmdctl->cmds[i].argv[j] != NULL; j++) {
          free(cmdctl->cmds[i].argv[j]);
        }
      }
      // Free the array of command structs
      free(cmdctl->cmds);
    }
    // Free the main controller struct itself
    free(cmdctl);
  }
}
// PROBLEM 1: De-allocate the given given cmdctl struct. Free all
// argv[] elements in each of the cmds[], free that array, then free
// the cmdctl struct itself.

int cmdctl_create_cmddir(cmdctl_t *cmdctl) {
  struct stat st;

  // Check if the path already exists
  if (stat(cmdctl->cmddir, &st) == 0) {
    // Path exists - check if it's a directory
    if (S_ISDIR(st.st_mode)) {
      printf("Re-using existing output directory '%s'\n", cmdctl->cmddir);
      return 0;
    } else {
      // Path exists but isn't a directory
      fprintf(stderr, "ERROR: Could not create cmd directory '%s'\n", cmdctl->cmddir);
      fprintf(stderr, "       Non-directory file with that name already exists\n");
      return -1;
    }
  }

  // Directory doesn't exist yet, so try to create it
  if (mkdir(cmdctl->cmddir, 0700) == 0) {
    return 1;  // Successfully created
  } else {
    // mkdir failed — probably due to permissions or invalid path
    fprintf(stderr, "ERROR: Could not create cmddir directory '%s'\n", cmdctl->cmddir);
    fprintf(stderr, "       %s\n", strerror(errno));
    return -1;
  }
}
// PROBLEM 1: Creates the output directory names in cmdctl->cmddir. If
// cmddir does not exist, it is created as directory with permissions
// of User=read/write/execute then returns 1. If cmddir already exists
// and is a directory, prints the following message and returns 0:
//
//   Re-using existing output directory 'XXX'
//
// If a non-directory file named cmddir already exists, print an error
// message and return -1 to indicate the program cannot proceed. The
// error message is:
//
// ERROR: Could not create cmddir directory 'XXX'
//        Non-directory file with that name already exists
//
// with XXX substituted with the value of cmddir.
//
// CONSTRAINT: This function must be implemented using low-level
// system calls. Use of high-level calls like system("cmd") will be
// reduced to 0 credit. Review system calls like stat() and mkdir()
// for use here. The access() system call may be used but keep in mind
// it does not distinguish between regular files and directories.

int cmdctl_add_cmd(cmdctl_t *cmdctl, char *cmdline) {
  int returned = 0;

  if (cmdctl->cmds_count >= cmdctl->cmds_capacity) {
    // size is doubled
    cmd_t *resized = realloc(cmdctl->cmds, (cmdctl->cmds_capacity * 2) * sizeof(cmd_t));
    cmdctl->cmds = resized;
    cmdctl->cmds_capacity *= 2;
    returned = 1;
  }

  // figure out current command
  int count = cmdctl->cmds_count;
  cmd_t *curr_cmd = &cmdctl->cmds[count];

  // copying cmdline to cmd->cmdline
  strncpy(curr_cmd->cmdline, cmdline, MAX_LINE - 1);

  // Create a copy for splitting (must be freed later)
  char *cmdline_copy = strdup(cmdline);

  // splitting with copied cmd_line
  int argc;
  char *temp_argv[MAX_ARGS];

  split_into_argv(cmdline_copy, temp_argv, &argc);

  curr_cmd->input_filename[0] = '\0';

  for (int i = 0; i < argc; i++) {
    if (strcmp(temp_argv[i], "<") == 0) {
      strncpy(curr_cmd->input_filename, temp_argv[i + 1], MAX_NAME - 1);
      argc -= 2;
      temp_argv[i] = NULL;
      temp_argv[i++] = NULL;
    }
  }

  for (int i = 0; i < argc; i++) {
    curr_cmd->argv[i] = strdup(temp_argv[i]);
  }
  curr_cmd->argv[argc] = NULL;
  curr_cmd->argc = argc;

  // create output function
  char output[MAX_NAME];

  strncpy(output, curr_cmd->argv[0], MAX_NAME - 1);
  output[MAX_NAME - 1] = '\0';

  for (int i = 0; output[i] != '\0'; i++) {
    if (output[i] == '/') {
      output[i] = '_';
    }
  }

  sprintf(curr_cmd->output_filename, "%s/%04d-%s.out", cmdctl->cmddir, cmdctl->cmds_count, output);

  curr_cmd->cmdstate = CMDSTATE_INIT;
  curr_cmd->pid = -1;
  curr_cmd->exit_code = -1;
  curr_cmd->output_bytes = -1;

  free(cmdline_copy);
  cmdctl->cmds_count++;

  return returned;
}
// PROBLEM 1: Add a command to the cmdctl. If the capacity of the
// cmds[] array is insufficient, is size is doubled before adding
// using the realloc() function.
//
//
// SETTING ARGV[] OF THE CMD
//
// `cmdline` is the string typed in as a command. `cmdline` is
// separated into an argv[] array using the split_into_argv()
// function. All strings are duplicated into the argv[] array of the
// cmd_t and are later free()'d on memory de-allocation.
//
// Example:
// cmdline: "data/sleep_print.sh 1 hello world"
// argv[]:
//   [ 0]: data/sleep_print.sh
//   [ 1]: 1
//   [ 2]: hello
//   [ 3]: world
//   [ 4]: NULL
//
// NOTE: Paramameter `cmdline` is copied into the cmd.cmdline[]
// field. This copy is left unchanged. When splitting, another local
// copy of the string is made. strcpy() is useful for this and
// strdup() is used for creating heap-allocated copies for cmd.argv[]
// to point at.
//
// HANDLING INPUT REDIRECTION
//
// If the last two elements of the command line indicate input
// redirection via "< infile.txt" then the argv[] and input_filename
// fields are set accordingly.  An example:
//
// cmdline: "wc -l -c < Makefile"
// argv[]:
//   [ 0]: wc
//   [ 1]: -l
//   [ 2]: -c
//   [ 3]: NULL      NOTE "<" and "Makefile" removed
// input_filename: Makefile
//
// SETTING OUTPUT FILENAME
//
// The output_filename field is set to a pattern like the following:
//
//     outdir/0017-data_sleep_print.sh.out
//     |      |     |                   +-> suffix .out
//     |      |     +-> argv[0] with any / characters replaced by _
//     |      +-> 4 digit cmdid (index into cmds[] array), 0-padded
//     +->cmddir[] directory name followed by "/"
//
// A copy of the argv[0] string is modified to replace slashes with
// underscores and the sprintf() family of functions is used for
// formatting.
//
// SETTING OTHER FIELDS
//
// Other fields are set as follows:
// - argc to the length of the argv[] array
// - cmdstate to the CMDSTATE_INIT
// - pid, exit_code, output bytes to -1
//
// The function returns 1 if the added command caused the internal
// array of commands to expand and 0 otherwise.
//
// CONSTRAINT: Do not use snprintf(), just plain sprintf(). Aim for
// simplicity and add safety at a later time.

void cmdctl_print_oneline(cmdctl_t *cmdctl, int cmdid) {
  // printf("==ONELINE OUTPUT of cmd==\n");
  cmd_t *cmd = &cmdctl->cmds[cmdid];

  char state_str[48];  // holds "RUNNING", "EXIT(x)", etc.
  switch (cmd->cmdstate) {
    case CMDSTATE_NOTSET:
      sprintf(state_str, "NOTSET");
      break;
    case CMDSTATE_INIT:
      sprintf(state_str, "INIT");
      break;
    case CMDSTATE_RUNNING:
      sprintf(state_str, "RUNNING");
      break;
    case CMDSTATE_UNKNOWN_END:
      sprintf(state_str, "UNKNOWN_END");
      break;
    case CMDSTATE_FAIL_INPUT:
      sprintf(state_str, "FAIL_INPUT");
      break;
    case CMDSTATE_FAIL_OUTPUT:
      sprintf(state_str, "FAIL_OUTPUT");
      break;
    case CMDSTATE_FAIL_EXEC:
      sprintf(state_str, "FAIL_EXEC");
      break;
    case CMDSTATE_EXIT:
      sprintf(state_str, "EXIT(%d)", cmd->exit_code);
      break;
    case CMDSTATE_SIGNALLED:
      sprintf(state_str, "SIGNALLED(%d)", cmd->exit_code);
      break;
    default:
      sprintf(state_str, "UNKNOWN");
  }

  if (cmd->cmdstate == CMDSTATE_RUNNING || cmd->output_bytes < 0) {
    // output bytes is "-"
    printf("%5d #%06d       - %-15s %s\n", cmdid, cmd->pid, state_str, cmd->argv[0]);
  } else {
    printf("%5d #%06d %7ld  %-15s %s\n", cmdid, cmd->pid, cmd->output_bytes, state_str, cmd->argv[0]);
  }
}
// PROBLEM 1: Prints a oneline summary of the indicated command's
// current state. The format is:
//
// EXAMPLES:
//  A:5 B:1 C:-6    D:7     E:-15         F:remaining
// ------------------------------------------------------------
//    24 #007575       - RUNNING         data/sleep_print.sh
//   123 #007570      54 EXIT(0)         seq
//     2 #007566      73 EXIT(1)         gcc
//  2961 #007562      23 SIGNALLED(-15)  data/self_signal.sh
// ------------------------------------------------------------
// 123456789012345678901234567890123456789012345678901234567890
//
// - A: cmdid (index into the cmds[] array), 5 places, right aligned
// - B: # symbol
// - C: PID of command, 6 places 0 padded, left aligned
// - D: number of output bytes if not running, otherwise a "-", 7 places
//      right aligned
// - E: printed state with exit code/signal, 15 places, left aligned
// - F: argv[0] (command name), left aligned, printed for remainder of
//   line
//
// For E, the possible outputs are based on cmdstate_t values:
// - NOTSET
// - INIT
// - RUNNING
// - EXIT(<num>)      : positive number of normal exit
// - SIGNALLED(<num>) : negative number of signal
// - UNKNOWN_END
// - FAIL_INPUT
// - FAIL_OUTPUT
// - FAIL_EXEC
//
// NOTE: Most implementations will use a combination of printf() and
// possibly sprintf() to get the required widths. The trickiest part
// is getting the states EXIT() / SIGNALLED() aligned correctly for
// which an individual call to sprintf() to first format the numbers
// followed by a printf() with a width specifier is helpful.
//
// CONSTRAINT: Do not use snprintf(), just plain sprintf(). Aim for
// simplicity and add safety at a later time.

void cmdctl_print_all(cmdctl_t *cmdctl) {
  printf("CMDID PID       BYTES STATE           COMMAND\n");
  for (int i = 0; i < cmdctl->cmds_count; i++) {
    cmdctl_print_oneline(cmdctl, i);
  }
}
// PROBLEM 1: Prints a header and then iterates through all commands
// printing one-line summaries of them information on them. Prints a
// table header then repeatedly calls cmdctl_print_oneline(). Example:
//
// CMDID PID       BYTES STATE           COMMAND
//     0 #107561      22 SIGNALLED(-9)   data/self_signal.sh
//     1 #107562      23 SIGNALLED(-15)  data/self_signal.sh
//     2 #107566      73 EXIT(1)         gcc
//     3 #107570      54 EXIT(0)         seq
//     4 #107575       - RUNNING         data/sleep_print.sh

void cmdctl_print_info(cmdctl_t *cmdctl, int cmdid) {
  printf("CMD %d INFORMATION\n", cmdid);
  printf("command: %s\n", cmdctl->cmds[cmdid].cmdline);
  printf("argv[]:\n");
  for (int i = 0; i < cmdctl->cmds[cmdid].argc; i++) {
    printf("  [ %d]: %s\n", i, cmdctl->cmds[cmdid].argv[i]);
  }

  cmd_t *cmd = &cmdctl->cmds[cmdid];
  char state_str[48];  // holds "RUNNING", "EXIT(x)", etc.
  switch (cmd->cmdstate) {
    case CMDSTATE_NOTSET:
      sprintf(state_str, "NOTSET");
      break;
    case CMDSTATE_INIT:
      sprintf(state_str, "INIT");
      break;
    case CMDSTATE_RUNNING:
      sprintf(state_str, "RUNNING");
      break;
    case CMDSTATE_UNKNOWN_END:
      sprintf(state_str, "UNKNOWN_END");
      break;
    case CMDSTATE_FAIL_INPUT:
      sprintf(state_str, "FAIL_INPUT");
      break;
    case CMDSTATE_FAIL_OUTPUT:
      sprintf(state_str, "FAIL_OUTPUT");
      break;
    case CMDSTATE_FAIL_EXEC:
      sprintf(state_str, "FAIL_EXEC");
      break;
    case CMDSTATE_EXIT:
      sprintf(state_str, "EXIT(%d)", cmd->exit_code);
      break;
    case CMDSTATE_SIGNALLED:
      sprintf(state_str, "SIGNALLED(%d)", cmd->exit_code);
      break;
    default:
      sprintf(state_str, "UNKNOWN");
  }

  printf("cmdstate: %s\n", state_str);

  if (cmd->input_filename[0] == '\0') {
    printf("input: <NONE>\n");
  }

  else {
    printf("input: %s\n", cmd->input_filename);
  }

  if (cmd->output_filename[0] == '\0') {
    printf("output: <NONE>\n");
  }

  else {
    printf("output: %s\n", cmd->output_filename);
  }

  if (cmd->cmdstate == CMDSTATE_RUNNING || cmd->output_bytes < 0) {
    printf("output size: <CMD NOT FINISHED>\n");
  }

  else {
    printf("output size: %ld bytes\n", cmd->output_bytes);
  }
}
// PROBLEM 1: Prints detailed information about the given
// command. Example output is as follows.
//
// EXAMPLE 1: Exited command
// Call: cmdctl_print_info(cmdctl, 2);
// -------OUTPUT-------------
// CMD 2 INFORMATION
// command: seq 25
// argv[]:
//   [ 0]: seq
//   [ 1]: 25
// cmdstate: EXIT(0)
// input: <NONE>
// output: commando-dir/0002-seq.out
// output size: 66 bytes
// -------------------------
//
// EXAMPLE 2: Running command
// Call: cmdctl_print_info(cmdctl, 4);
// -------OUTPUT-------------
// CMD 4 INFORMATION
// command: data/sleep_print.sh 25 hello world
// argv[]:
//   [ 0]: data/sleep_print.sh
//   [ 1]: 25
//   [ 2]: hello
//   [ 3]: world
// cmdstate: RUNNING
// input: <NONE>
// output: commando-dir/0004-data_sleep_print.sh.out
// output size: <CMD NOT FINISHED>
// -------------------------
//
// May have be some duplicate code in this function to that present in
// cmd_print_oneline() for handling the different cmdstate values.

void cmdctl_show_output(cmdctl_t *cmdctl, int cmdid) {
  cmd_t *cmd = &cmdctl->cmds[cmdid];
  if (cmd->cmdstate == CMDSTATE_INIT || cmd->cmdstate == CMDSTATE_RUNNING) {
    printf("NO OUTPUT AVAILABLE: command has not finished running\n");
  } else {
    int fd = open(cmd->output_filename, O_RDONLY);
    if (fd == -1) {
      perror("failed to open file");
    }
    char buffer[100000];
    int bytes_read = read(fd, buffer, 100000);
    buffer[bytes_read] = '\0';
    printf("%s", buffer);
    close(fd);
  }
}
// PROBLEM 1: If the given command is not finished (e.g. still
// RUNNING), prints the message
//
//   NO OUTPUT AVAILABLE: command has not finished running
//
// If the command is finished, opens the output file for the command
// and prints it to the screen using low-level UNIX I/O.
//
// CONSTRAINT: This routine must use low-level open() and read() calls
// and must allocate no more memory for the file contents than
// MAX_LINE bytes in a buffer. Use of fscanf(), fgetc(), and other C
// standard library functions is not permitted and scanning the
// entirety of the file into memory is also not permitted.

////////////////////////////////////////////////////////////////////////////////
// PROBLEM 2 Functions
////////////////////////////////////////////////////////////////////////////////

int cmdctl_start_cmd(cmdctl_t *cmdctl, int cmdid) {
  cmd_t *cmd = &cmdctl->cmds[cmdid];

  // Fork a child process to run the command
  int child_pid = fork();

  if (child_pid == 0) {
    // Child process starts here

    // If an output file was specified
    if (cmd->output_filename[0] != '\0') {
      // Open the file for writing, truncate if it exists
      int fd_output = open(cmd->output_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      if (fd_output == -1) {
        // If failed to open, exit with specific error code
        exit(CMDSTATE_FAIL_OUTPUT);
      }

      // Redirect stdout and stderr to the output file
      dup2(fd_output, STDOUT_FILENO);
      dup2(fd_output, STDERR_FILENO);
      close(fd_output);  // Close original FD after redirection

      // If an input file is also specified
      if (cmd->input_filename[0] != '\0') {
        // Try to open the input file
        int fd_input = open(cmd->input_filename, O_RDONLY);
        if (fd_input == -1) {
          // Print exact error message and exit with input error code
          printf("ERROR: can't open file '%s' for input : %s\n", cmd->input_filename, strerror(errno));
          exit(CMDSTATE_FAIL_INPUT);
        }

        // Redirect stdin from the input file
        dup2(fd_input, STDIN_FILENO);
        close(fd_input);  // Close input FD after redirection
      }

      // Run the command — only returns if something goes wrong
      if (execvp(cmd->argv[0], cmd->argv) == -1) {
        // Print error message if exec fails
        printf("ERROR: program '%s' failed to exec : %s\n", cmd->cmdline, strerror(errno));
        exit(CMDSTATE_FAIL_EXEC);
      }
    }
  } else {
    // Parent process continues here

    // Save the child's PID and mark it as running
    cmd->pid = child_pid;
    cmd->cmdstate = CMDSTATE_RUNNING;
  }

  return 0;  // Always returns 0 to parent
}

// PROBLEM 2: Start a child process that will run the indicated
// cmd. After creating the child process, the parent sets sets the
// `pid` field, changes the state to RUNNING and returns 0.
//
// The child sets up output redirection so that the standard out AND
// standard error streams for the child process is channeled into the
// file named in field `output_filename`. Note that standard out and
// standard error are "merged" so that they both go to the same
// `outfile_name`. This file should use the standard file
// creation/truncation options (O_WRONLY|O_CREAT|O_TRUNC) and be
// readable/writable by the user (S_IRUSR|S_IWUSR).
//
// If input_filename is not empty, input redirection is also set up
// with input coming from the file named in that field. Output
// redirection is set up first to allow any errors associated with
// input redirection to be captured in the output file for the
// command.
//
// IMPORTANT: Any errors in the child during I/O redirection or
// exec()'ing print error messages and cause an immediate exit() with
// an associated error code. These are as follows:
//
// | CONDITION            | EXIT WITH CODE              | Message |
// |----------------------+-----------------------------+---------------------------------------------------------------------------------|
// | Output redirect fail | exit(CMDSTATE_FAIL_OUTPUT); | No message printed | | Input redirect fail  |
// exit(CMDSTATE_FAIL_INPUT);  | ERROR: can't open file 'no-such-file.txt' for input : No such file or directory | |
// Exec failure         | exit(CMDSTATE_FAIL_EXEC);   | ERROR: program 'no-such-cmd' failed to exec : No such file or
// directory         |
//
// The "No such file..." message is obtained and printed via
// strerror(errno) which creates a string based on what caused a
// system call to fail.
//
// NOTE: When correctly implemented, this function should never return
// in the child process though the compiler may require a `return ??`
// at the end to match the int return type. NOT returning from this
// function in the child is important: if a child manages to return,
// there will now be two instances of main() running with the child
// starting its own series of grandchildren which will not end well...

int cmdctl_update_one(cmdctl_t *cmdctl, int cmdid, int finish) {
  cmd_t *cmd = &cmdctl->cmds[cmdid];

  int status;   // Will hold info about how the child ended
  int retcode;  // Exit code or signal code

  // If the command isn't running, there's nothing to update
  if (cmd->cmdstate != CMDSTATE_RUNNING) {
    return 0;
  }

  // If we are only checking (non-blocking)
  if (finish == 0) {
    // WNOHANG means "don't wait, just check"
    if (waitpid(cmd->pid, &status, WNOHANG) < 1) {
      return 0;  // Still running
    } else {
      // Process finished — figure out how it ended
      if (WIFEXITED(status)) {
        retcode = WEXITSTATUS(status);

        // If it failed due to redirection or exec error
        if (retcode == CMDSTATE_FAIL_INPUT || retcode == CMDSTATE_FAIL_OUTPUT || retcode == CMDSTATE_FAIL_EXEC) {
          cmd->cmdstate = retcode;
        } else {
          // Otherwise it exited normally
          cmd->cmdstate = CMDSTATE_EXIT;
        }

        cmd->exit_code = retcode;
      } else if (WIFSIGNALED(status)) {
        // Process ended due to a signal (e.g., SIGINT)
        retcode = -1 * WTERMSIG(status);
        cmd->exit_code = retcode;
        cmd->cmdstate = CMDSTATE_SIGNALLED;
      }
    }
  }
  // If we're told to wait until it's done (blocking)
  else if (finish == 1) {
    if (cmd->pid != 0) {
      waitpid(cmd->pid, &status, 0);  // Blocks until process ends

      if (WIFEXITED(status)) {
        retcode = WEXITSTATUS(status);

        if (retcode == CMDSTATE_FAIL_INPUT || retcode == CMDSTATE_FAIL_OUTPUT || retcode == CMDSTATE_FAIL_EXEC) {
          cmd->cmdstate = retcode;
        } else {
          cmd->cmdstate = CMDSTATE_EXIT;
        }

        cmd->exit_code = retcode;
      } else if (WIFSIGNALED(status)) {
        retcode = -1 * WTERMSIG(status);
        cmd->exit_code = retcode;
        cmd->cmdstate = CMDSTATE_SIGNALLED;
      }
    }
  }

  // Get output file size (even if it's 0)
  struct stat fd;
  stat(cmd->output_filename, &fd);
  cmd->output_bytes = fd.st_size;

  // Print out "COMPLETE" status (expected by tests)
  printf("---> COMPLETE:");
  cmdctl_print_oneline(cmdctl, cmdid);

  return 1;  // Means the status changed from RUNNING
}

// PROBLEM 2: Updates a single cmd to check for its completion. If the
// cmd is not RUNNING, returns 0 immediately.
//
// If the parameter `finish` is 0, uses wait()-family system calls to
// determine if the child process associated with the command is
// complete. If not returns 0 immediately - a NON-BLOCKING WAIT() is
// required to accomplish this.
//
// If the parameter `finish` is 1, uses a BLOCKING-style call to wait
// until the child process is actually finished before moving on.
//
// If the child process is finished, then diagnosis its exit status to
// determine a normal exit vs abnormal termination due to a
// signal. the W-MACROS() are used to determine this. The
// cmd->exit_code field is set to positive for a regular exit code or th
// negative signal number for an abnormal exit.
//
// The output_bytes is set by using a stat()-family call to determine
// the number of bytes in the associated output file.
//
// When the command finishes, an "ALERT" message is printed like the
// following which primarily uses the cmd_print_oneline() function.
//
// ---> COMPLETE:     2 #108624    2617 EXIT(0)         ls
// ---> COMPLETE:   129 #108725      22 SIGNALLED(-2)   data/self_signal.sh
// <string shown> <----------output from cmd_print_oneline()---------------->
//
// Returns 1 when the child process changes from RUNNING to a
// completed state.

int cmdctl_update_all(cmdctl_t *cmdctl, int finish) {
  int total_changed = 0;

  // Go through every command we've seen and try to update it
  for (int i = 0; i < cmdctl->cmds_count; i++) {
    total_changed += cmdctl_update_one(cmdctl, i, finish);
  }

  // Only print if something finished
  if (total_changed > 0) {
    printf("%d commands finished\n", total_changed);
  }

  return total_changed;
}
// PROBLEM 2: Iterates through all commands and calls update on
// them. Passes the the `finish` parameter on to each call. Counts
// then number of commands that change state (from RUNNING to a
// finished state) and if 1 or more finish, prints a message like
//
//   4 commands finished
//
// after completing the loop. Returns the number of commands the that
// finished.