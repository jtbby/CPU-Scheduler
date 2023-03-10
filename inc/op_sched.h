

#ifndef OP_SCHED_H
#define OP_SCHED_H

#include "vm_settings.h"

// Process Node Definition
typedef struct process_node {
  pid_t pid; // PID of the Process you're Tracking
  char *cmd; // Name of the Process being run
  unsigned int state; // Contains the State of the Process, Priority Flag, AND Exit Code (set by OS).
  int age; // How long this has been in the Ready Queue - Low Priority since last run.
  struct process_node *next; // Pointer to next Process Node in a linked list.
} Op_process_s;

// Queue Header Definition
typedef struct queue_header {
  int count; // How many items are in this linked list?
  Op_process_s *head; // Points to FIRST node of linked list.  No Dummy Nodes.
} Op_queue_s;

// Schedule Header Definition
typedef struct op_schedule {
  Op_queue_s *ready_queue_high; // Linked List of Processes ready to Run on CPU (High Priority)
  Op_queue_s *ready_queue_low;  // Linked List of Processes ready to Run on CPU (Low Priority)
  Op_queue_s *defunct_queue;    // Linked List of Defunct Processes 
} Op_schedule_s;

// Prototypes
Op_schedule_s *op_create(); 
Op_process_s *op_new_process(char *command, pid_t pid, int is_low, int is_critical);
int op_add(Op_schedule_s *schedule, Op_process_s *process);
int op_get_count(Op_queue_s *queue);
Op_process_s *op_select_high(Op_schedule_s *schedule);
Op_process_s *op_select_low(Op_schedule_s *schedule);
int op_promote_processes(Op_schedule_s *schedule);
int op_exited(Op_schedule_s *schedule, Op_process_s *process, int exit_code);
int op_terminated(Op_schedule_s *schedule, pid_t pid, int exit_code);
void op_deallocate(Op_schedule_s *schedule);

#endif
