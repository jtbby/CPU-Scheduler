

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
// Local Includes
#include "vm.h"
#include "vm_support.h"
#include "vm_process.h"
#include "vm_printing.h"
#include "vm_cs.h"

/* Local Definitions */
static char *builtin_cmds[] = {"quit", "exit", "help", "terminate", "start", "stop", "debug", "schedule", "delaytime", "runtime", "status"};

/* Local Prototypes */
static int get_user_input(char *line);
static void execute_builtin(process_data_t *data);
static void execute_command(process_data_t *data);
static int is_builtin(process_data_t *data);
static void print_process_data(process_data_t *data);
static int is_whitespace(char *str);

static void print_help();
static process_data_t *initialize_data(const char *str);
static process_data_t *parse_input(char *str);

/* Global Variables */
static char g_status_msg[MAX_STATUS] = {0};


/* Run the Virtual System with User Shell Access */
void shell() {
  char buffer[MAX_CMD_LINE] = {0};
  process_data_t *proc_data = NULL;
  int ret = 0;

  print_cs_status();
  sprintf(g_status_msg, "Debug Mode: %s", debug_mode?"On":"Off");
  print_status(g_status_msg);
  sprintf(g_status_msg, "Type help for reference on TRILBY's Built-In Commands.");
  print_status(g_status_msg);

  // No conditions here. The program will either crash or the user will type quit to break the loop.
  while(1) {
    print_prompt();

    // Step 1: Get the raw user input
    // Check for any errors getting input.
    do {
      ret = get_user_input(buffer);
      if(ret != 0) {
        // Special case where the error was interrupted by a signal, try again.
        if(ret == EINTR) {
//          print_warning("EINTR");
          continue;
        }
        abort_error("Error on getting user input.", __FILE__);
      }
    } while(ret != 0);

    // Step 2: Parse the User Input
    proc_data = parse_input(buffer);
    // If there was an issue parsing it, dump it and get a new command
    if(proc_data == NULL) {
      continue;
    }

    // Only prints if DEBUG mode is ON
    print_process_data(proc_data);

    // Step 3: Execute the command or built-in
    if(is_builtin(proc_data)) {
      execute_builtin(proc_data);
      // Step 4: Free Command (if Built-In)
      free_process(proc_data);
      proc_data = NULL;
    }
    else {
      // Step 4: Add the Command to the Jobs Tracker then Execute It
      execute_command(proc_data);
    }
  }
  
  return;
}

/* Executes a Trilby Built-In Instruction */
static void execute_builtin(process_data_t *data) {
  if(data == NULL || data->cmd == NULL) {
    return;
  }
  // quit
  if((strncmp(data->cmd, "quit", 4) == 0) || (strncmp(data->cmd, "exit", 4) == 0)) {
    print_status("Exiting Program.");
    free_process(data);
    exit(EXIT_SUCCESS);
  }
  else if(strncmp(data->cmd, "help", 4) == 0) {
    print_help();
  }
  else if(strncmp(data->cmd, "debug", 5) == 0) {
    debug_mode = (debug_mode)?0:1;
    sprintf(g_status_msg, "Debug Mode: %s", debug_mode?"On":"Off");
    print_status(g_status_msg);
  }
  // start - Starts the CS System
  else if(strncmp(data->cmd, "start", 5) == 0) {
    print_status("Starting the CS Execution");
    start_cs();
  }
  // stop - Stops the CS System (resume with 'start')
  else if(strncmp(data->cmd, "stop", 4) == 0) {
    print_status("Stopping CS System");
    stop_cs();
  }
  // schedule - Print out the current CS Scheduler Status
  else if(strncmp(data->cmd, "schedule", 8) == 0) {
    print_schedule();
  }
  // status - Print out the CS System Status
  else if(strncmp(data->cmd, "status", 6) == 0) {
    print_cs_status();
  }
  else if(strncmp(data->cmd, "terminate", 9) == 0) {
    if(data->argv[1] == NULL || is_whitespace(data->argv[1])) {
      print_warning("You need to enter a PID to terminate.\n\teg. terminate 3345962");
      return;
    }
    errno = 0;
    char *pid_str = data->argv[1];
    pid_t pid = (useconds_t)strtol(data->argv[1], &pid_str, 10);
    if(*pid_str == '\0') {
      kill(pid, SIGKILL);
    }
    else {
      print_warning("You need a valid pid.\n\teg. runtime 3345962");
      return;
    }
  }
  // Change the Runtime Quantum (how long each process runs for)
  else if(strncmp(data->cmd, "runtime", 7) == 0) {
    if(data->argv[1] == NULL || is_whitespace(data->argv[1])) {
      print_warning("You need to enter a new runtime in usec.\n\teg. runtime 10000000");
      return;
    }
    errno = 0;
    char *p_time = data->argv[1];
    useconds_t time = (useconds_t)strtol(data->argv[1], &p_time, 10);
    if(*p_time == '\0') {
      set_run_usec(time);
    }
    else {
      print_warning("You need a valid time in usec.\n\teg. runtime 10000000");
      return;
    }
  }
  // Change the Delay (how long to sleep between each process running)
  else if(strncmp(data->cmd, "delaytime", 9) == 0) {
    if(data->argv[1] == NULL || is_whitespace(data->argv[1])) {
      print_warning("You need to enter a new delay time in usec.\n\teg. delaytime 10000000");
      return;
    }
    errno = 0;
    char *p_time = data->argv[1];
    useconds_t time = (useconds_t)strtol(data->argv[1], &p_time, 10);
    if(*p_time == '\0') {
      set_between_usec(time);
    }
    else {
      print_warning("You need a valid delay time in usec.\n\teg. delaytime 10000000");
      return;
    }
  }
}

/* Executes a local (or /usr/bin) command */
static void execute_command(process_data_t *data) {
  // Creates the process and loads it into the Ready Queue
  create_process(data);
}

// Gets line from user, ensures is valid, and strips the newline from it.
// - Returns ERRNO code, -1 on other errors, or 0 on success
static int get_user_input(char *line) {
  errno = 0;
  memset(line, 0, MAX_CMD_LINE);
  char *p_line = fgets(line, MAX_CMD_LINE, stdin);
  // If any errors occurred, return the errno
  if(errno) {
    return errno;
  }

  // Basic catch-all for failure
  if(p_line == NULL) {
    return -1;
  }

  // Convert the first newline (left behind by fgets) to a null terminator.
  strtok(line, "\n");

  return 0;
}

// Returns 1 if the data entered is a Built-In command for Trilby-VM
static int is_builtin(process_data_t *data) {
  if(data == NULL || data->cmd == NULL) {
    return 0;
  }

  int count = sizeof(builtin_cmds) / sizeof(char *);
  int i = 0;
  for(i = 0; i < count; i++) {
    if(strncmp(builtin_cmds[i], data->cmd, strlen(builtin_cmds[i])) == 0) {
      return 1;
    }
  }
  return 0;
}

/* Prints out the Command Information */
static void print_process_data(process_data_t *data) {
  if(debug_mode == 0 || data == NULL) {
    return;
  }
  
  print_debug(".----------------------------");
  sprintf(g_status_msg, "| [Input: %s]", data->input_orig);
  print_debug(g_status_msg);
  sprintf(g_status_msg, "| - [PID: %d]", data->pid);
  print_debug(g_status_msg);
  sprintf(g_status_msg, "| - [CMD: %s]", data->cmd);
  print_debug(g_status_msg);
  sprintf(g_status_msg, "| - [Is Low Priority: %s]", data->is_low?"Yes":"No");
  print_debug(g_status_msg);
  sprintf(g_status_msg, "| - [Is Critical: %s]", data->is_critical?"Yes":"No");
  print_debug(g_status_msg);
  for(int i = 0; i < MAX_ARGS && data->argv[i] != NULL; i++) {
    sprintf(g_status_msg, "| - [Arg %2d: %s]", i, data->argv[i]);
    print_debug(g_status_msg);
  }
  print_debug(".----------------------------");

  return;
}

/* Parse user command input, return NULL if empty */
static process_data_t *parse_input(char *str) {
  if(str == NULL || strlen(str) <= 0 || is_whitespace(str)) {
    return NULL;
  }

  // Step 1: Initialize the user data struct
  process_data_t *data = initialize_data(str);
  if(data == NULL) {
    abort_error("Failed to Allocate Memory for new Command String", __FILE__);
  }

  // Step 2: Extract Command
  char *p_tok = strtok(data->input_toks, " "); 
  data->cmd = p_tok;  // Guaranteed in-scope as it's pointing to data->input_toks
  data->is_critical = 0; // Initialize to Non-Priority
  data->is_low = 0; // Default Priority (high-priority)
  data->argv[0] = data->cmd;
  
  // Optionally restrict commands to local directory binaries only (set in inc/vm_settings.h)
  // - This, of course, doesn't look for the location of the command, but instead ensures
  //   that you can't use absolute paths or relative path adjustments to break out of local.
#if LOCAL_CMDS_ONLY > 0
  if(strchr(data->cmd, '/')) {
    print_warning("Only Local Commands Are Allowed.");
    if(strstr(data->cmd, "./")) {
      print_status("Note: ./ is not needed for local commands.");
    }
    free_process(data);
    return NULL;
  }
#endif

  // Step 3: Populate Arguments
  int arg = 1;
  do {
    p_tok = strtok(NULL, " ");
    if(p_tok != NULL) {
      if(strncmp(p_tok, "-c", 2) == 0) {
        data->is_critical = 1;
      }
      else if(strncmp(p_tok, "-l", 2) == 0) {
        if(data->is_critical == 0) {
          data->is_low = 1;
        } 
      }
      else {
        data->argv[arg++] = p_tok; // All pointers reference data->input_toks
      }
    }
  } while(p_tok != NULL);

  return data;
}

/* Initializes a clean input data structure */
static process_data_t *initialize_data(const char *str) {
  process_data_t *data = calloc(1, sizeof(process_data_t));
  if(data == NULL) {
    return NULL;
  }

  // Initialize all members to NULL (0)
  data->cmd = NULL;  // Will point to the binary name/builtin-command
  memset(data->argv, 0, sizeof(data->argv)); // Array of char pointers, one per argument
  strncpy(data->input_orig, str, MAX_CMD); // Never strtok this directly.
  strncpy(data->input_toks, str, MAX_CMD); // This you strtok.
  data->pid = 0;     // For safety, this should never be -1 (if you kill -1, you kill all owned processes)
  data->next = NULL; // For the Jobs Queue Membership

  return data;
}

/* Return 1 if the string is entirely whitespace */
static int is_whitespace(char *str) {
  int i = 0;
  // If any character in the string is non whitespace, return 0
  for(i = 0; i < strlen(str); i++) {
    switch(str[i]) {
      case ' ':
      case '\n':
      case '\t':
      case '\r':
        break;
      default: // Any non-whitespace character is found, return 0 immediately
        return 0;
    }
  }
  // Entire string is nothing but whitespace
  return 1;
}

static void print_help() {
  sprintf(g_status_msg, ".------[HELP]------");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| help        Prints out this reference.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| start       Starts the CS Engine.");
  print_status(g_status_msg); 
  sprintf(g_status_msg, "| stop        Stops the CS Engine.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| Ctrl-C      Toggle (Start/Stop) the CS Engine.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| schedule    Prints out the Current State of all Queues.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| terminate X Terminate Process with PID X.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| status      Prints out the Current Settings.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| debug       Toggles Debug Information.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| runtime X   Sets the runtime to X usec.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| delaytime X Sets the delaytime to X usec.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "| quit        Exits TRILBY-VM.");
  print_status(g_status_msg);
  sprintf(g_status_msg, "+------------------");
  print_status(g_status_msg);
}



/*|**************************************************************
 *+----------- Hic Sunt Quisquiliae----------------------------+*
 **************************************************************V*/
