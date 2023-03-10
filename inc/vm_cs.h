
#ifndef VM_CS_H
#define VM_CS_H

#include <pthread.h>
#include "vm_process.h"
#include "op_sched.h"

// Globals
extern pthread_cond_t cs_cv;
extern pthread_condattr_t cs_cvattr;
extern pthread_mutex_t cs_cv_m;

// Prototypes
void initialize_cs_system();
void cs_cleanup();
void *cs_thread(void *args);
void cs_op_process(process_data_t *proc);
void cs_op_terminated(pid_t pid, int exit_code);
void cs_suspend(pid_t pid);
void cs_resume(pid_t pid);
void cs_exiting_process(int exit_code);
void print_schedule();
void print_op_queue(Op_queue_s *queue);
void print_process_node(Op_process_s *node);
void start_cs();
void stop_cs();
void toggle_cs();
void print_cs_status();
void set_run_usec(useconds_t time);
void set_between_usec(useconds_t time);
#endif