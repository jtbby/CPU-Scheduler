

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
// Project Includes
#include "vm_settings.h"
#include "vm_shell.h"
#include "vm_printing.h"
#include "vm_support.h"

static char g_status_msg[MAX_STATUS] = {0};

// Quickly registers a new signal with the given signal number and handler
void register_signal(int sig, void (*handler)(int)) {
  if(handler == NULL) {
    abort_error("Handler Function Needed for Registration", __FILE__);
  }

  struct sigaction sa = {0};
  sa.sa_handler = handler;
  sigaction(sig, &sa, NULL);
}

// Print the Virtual System Prompt
void print_prompt() {
  printf("%s(PID: %d)%s %s%s%s ", BLUE, getpid(), RST, GREEN, PROMPT, RST);
  fflush(stdout);
}

// Prints out Status Messages
void print_status(char *msg) {
  printf("  %s[Status] %s%s\n", YELLOW, msg, RST);
}

// Prints out Debug Messages
void print_debug(char *msg) {
  if(debug_mode == 1) {
    printf("  %s[Debug ] %s%s\n", CYAN, msg, RST);
  }
}

// Prints out all the Warning Messages
void print_warning(char *msg) {
  fprintf(stderr, "  %s[Warn  ] %s%s\n", MAGENTA, msg, RST);
}

// Special Error that also immediately exits the program.
void abort_error(char *msg, char *src) {
  fprintf(stderr, "  %s[ERROR ] %s%s\n", RED, msg, RST);
  fprintf(stderr, "  %sTerminating Program%s\n", RED, RST);
  exit(EXIT_FAILURE);
}

// Prints out the Opening Banner
void print_trilby_banner() {
  printf(
" %sGreen Imperial HEx:%s %sTRILBY Task Manager v1.5a%s %s*TRIAL EXPIRED*%s\n%s"
"              [o]              \n"
"     ._________|__________.    \n"
"     |%s     .________.     %s|    \n"
"     |%s    /          \\    %s|    \n"
"     |%s   |____________|   %s|    \n"
"     |%s   |____________|   %s|    \n"
"     |%s \\================/ %s|    \n"
"     |                    |    \n"
"     | %s[TRILBY-VM] $%s _    |\n"
"     .____________________.    \n"
"              [  ]             \n"
"       ___________________  \n"
"    _-'.-.-.-.-. .-.-.-.-.`-_    \n" // Keyboard Derived from Dan Strychalski's Work
"  _'.-.-.-------------.-.-.-.`-_ \n" // Original Keyboard at https://www.asciiart.eu/computers/keyboards
" :------------------------------:%s\n",
  GREEN, RST, BLUE, RST, RED, RST, YELLOW,
  MAGENTA, YELLOW,
  MAGENTA, YELLOW,
  MAGENTA, YELLOW,
  MAGENTA, YELLOW,
  MAGENTA, YELLOW,
  GREEN, YELLOW,
  RST);
}

// Prints the full Schedule of all processes being tracked.
void print_op_debug(Op_schedule_s *schedule) {
  if(schedule == NULL) {
    print_debug("Schedule is not Initialized Yet.");
    return;
  }
  print_debug("Printing the Current Schedule Status...");
  sprintf(g_status_msg, "...[Ready - High Priority Queue - %d Processes]", op_get_count(schedule->ready_queue_high));
  print_debug(g_status_msg);
  print_op_queue_debug(schedule->ready_queue_high);
  sprintf(g_status_msg, "...[Ready - Low Priority Queue - %d Processes]", op_get_count(schedule->ready_queue_low));
  print_debug(g_status_msg);
  print_op_queue_debug(schedule->ready_queue_low);
  sprintf(g_status_msg, "...[Defunct Queue - %d Processes]", op_get_count(schedule->defunct_queue));
  print_debug(g_status_msg);
  print_op_queue_debug(schedule->defunct_queue);
}

// Prints a single Scheduler Queue
void print_op_queue_debug(Op_queue_s *queue) {
  Op_process_s *walker = queue->head;
  while(walker != NULL) {
    print_process_node_debug(walker);
    walker = walker->next;
  }
}

// Prints a schedule tracked process
void print_process_node_debug(Op_process_s *node) {
  if((node->state>>28)&1) {
    sprintf(g_status_msg, "     [PID :%d] %s (Exit Code: %d)", node->pid, node->cmd, (node->state)&0xFFFFFFF);
  }
  else {
    sprintf(g_status_msg, "     [PID :%d] %s", node->pid, node->cmd);
  }
  print_debug(g_status_msg);
}

