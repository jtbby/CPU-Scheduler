
// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
// Local Includes
#include "vm_cs.h"
#include "vm_support.h"
#include "vm_process.h"
#include "vm_printing.h"
#include "op_sched.h"

// Globals
pthread_mutex_t cs_cv_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cs_run_m = PTHREAD_MUTEX_INITIALIZER;
pthread_t pt_cs; // Main CS thread variable (controlled from atexit function)
static Op_process_s *on_cpu = NULL;
static Op_schedule_s *schedule = NULL;
static int cs_do_cs = 1; // Controls the lifetime CS Thread
static int cs_run = 0; // Controls the running of the CS Thread (initialized to STOP)
static char g_status_msg[MAX_STATUS] = {0};
static useconds_t sleep_usec_time = SLEEP_USEC;
static useconds_t between_usec_time = BETWEEN_USEC;

// Runs at VM startup to initialize context switching thread
void initialize_cs_system() {
  // Start the CS Thread Locked by...
  // 1) Acquiring the lock here on initialization
  // 2) Each iteration of the CS thread starts by acquiring the lock.
  // The Shell commands release/acquire the lock to control CS
  pthread_mutex_lock(&cs_cv_m);

  int ret = pthread_create(&pt_cs, NULL, &cs_thread, NULL);
  if(ret != 0) {
    abort_error("Could not create a Thread for the CS System.", __FILE__);
  }
  // Initialize the Scheduler System (this is designed as a part of CS)
  schedule = op_create();
  if(schedule == NULL) {
    abort_error("Failed to initialize the Scheduler System (op_create returned NULL).", __FILE__);
  }
}

// Called on an atexit to free all CS related memory.
void cs_cleanup() {
  print_status("... Beginning CS Shutdown");
  print_status("... Deallocating Scheduler");
  op_deallocate(schedule);
  print_status("... Shutting Down CS System and Dispatcher");
  cs_do_cs = 0; // Tell the thread to die.
  pthread_mutex_unlock(&cs_cv_m); // If the CS is not running, activate it so it can die.
  pthread_join(pt_cs, NULL);
  print_status("... Removing Process from CPU");
  free(on_cpu);
  on_cpu = NULL; // Nothing on CPU.
  print_status("... CS Shutdown Complete");
}

// Context Switching Thread
void *cs_thread(void *args) {
  int iteration = 1;
// 1) While not blocked... (lock cs_cv_m to block)
// .. a) Gets the next process to run from the Scheduler (select)
// .. .. Holds this in the on_cpu global
// .. b) Resumes the selected process
// .. c) Sleeps for sleep_usec_time microseconds
// .. d) Suspends the selected process
// .. e) Returns the process to the Scheduler (insert)
  while(cs_do_cs) {
    long delay = sleep_usec_time;
    pthread_mutex_lock(&cs_cv_m);  // mylock.acquire()  -- Turnstile Pattern
    pthread_mutex_unlock(&cs_cv_m);// mylock.release()
    // Check to see if the system is being shutdown while waiting on lock.
    if(cs_do_cs == 0) {
      continue; 
    }
    sprintf(g_status_msg, "Context Switch: Iteration %d", iteration++);
    print_debug(g_status_msg);

    // Call the Scheduler to get the next Process
    on_cpu = op_select_high(schedule);
    if(!on_cpu) {
      on_cpu = op_select_low(schedule);
      if(on_cpu) {
        delay *= 2;
      }
    }

    // Call the Scheduler to manage Promotions
    op_promote_processes(schedule);

    // Only Dispatch if something was selected
    if(on_cpu != NULL) {
      if(process_find(on_cpu->pid) == 0) {
        op_exited(schedule, on_cpu, 42);
        on_cpu = NULL;
      }
      else {
        // If this was pulled from the long-scheduler, run twice as long.
        sprintf(g_status_msg, "Schedule Select Returned PID %d", on_cpu->pid);
        print_debug(g_status_msg);
        kill(on_cpu->pid, SIGCONT);
        usleep(delay);
        // Process may have exited and already been cleaned up.  Check if still exists first.
        if(on_cpu) {
          kill(on_cpu->pid, SIGTSTP);
          op_add(schedule, on_cpu);
          on_cpu = NULL;
        }
      }
    }
    // Nothing selected, IDLE CPU
    else {
      print_debug("Schedule Select Returned Nothing");
      usleep(delay);
      // Unnecessary with the sleep... sched_yield(); // Tells Linux to switch threads
    }
    // Delay after the run quantum, but before we pick a new one (to help with debugging)
    usleep(between_usec_time);
  }
  pthread_exit(0);
}
/*
// Directs the Scheduler to suspend a process from execution
void cs_suspend(pid_t pid) {
  int last_state = -1;

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);
  stop_cs(); // Critical!  This ensures that all processes have been returned to Scheduler first
  print_debug("Suspending Process Now");
  op_suspend(schedule, pid);
  if(last_state == 1) {
    start_cs();
  }
}
*/
/*
// Directs the Scheduler to resume a process from execution
void cs_resume(pid_t pid) {
  int last_state = -1;

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);
  stop_cs(); // Critical!  This ensures that all processes have been returned to Scheduler first
  print_debug("Resuming Process Now");
  op_resume(schedule, pid);
  if(last_state == 1) {
    start_cs();
  }
}
*/
// Returns the process that was on the CPU back to the Scheduler
void cs_exiting_process(int exit_code) {
  if(on_cpu) {
    op_exited(schedule, on_cpu, exit_code);
    sprintf(g_status_msg, "Exiting PID %d, with exit code %d with op_exited\n", on_cpu->pid, exit_code);
    print_debug(g_status_msg);
    on_cpu = NULL;
  }
  else {
    print_warning("Tried to exit a non-existing process on the CPU");
  }
}

// Adds the newly created process to the schedule system
void cs_op_process(process_data_t *proc) {
  Op_process_s *proc_node = op_new_process(proc->cmd, proc->pid, proc->is_low, proc->is_critical);
  op_add(schedule, proc_node);
  print_op_debug(schedule);
}

// Tells the schedule to terminate the process with the given exit code
void cs_op_terminated(pid_t pid, int exit_code) {
  int last_state = -1;

  // There IS a race condition here, but it's a pretty minor one.  
  // If it WAS running when we shut it off, restart it when we finish.
  // It's possible they shut it off manually in the microseconds between these,
  // but it's not exactly the end of the world if they had.
  pthread_mutex_lock(&cs_run_m);
  last_state = cs_run;
  pthread_mutex_unlock(&cs_run_m);
  stop_cs(); // Critical! This ensures the state is consistent first.
  if(on_cpu && on_cpu->pid == pid) {
    // Exit from the CPU directly (terminated while being run)
    cs_exiting_process(exit_code);
  }
  else {
    // Exit from the Ready or Suspended Queues (terminated by command)
    op_terminated(schedule, pid, exit_code);
    sprintf(g_status_msg, "Terminating PID %d with exit code %d with op_terminated\n", pid, exit_code);
    print_debug(g_status_msg);
  }
  if(last_state == 1) {
    start_cs();
  }
} 

// Prints the full Schedule of all processes being tracked.
void print_schedule() {
  print_status("Printing the current Schedule Status...");
  sprintf(g_status_msg, "...[Ready - High Priority Queue - %d Processes]", op_get_count(schedule->ready_queue_high));
  print_status(g_status_msg);
  print_op_queue(schedule->ready_queue_high);
  sprintf(g_status_msg, "...[Ready - Low Priority Queue - %d Processes]", op_get_count(schedule->ready_queue_low));
  print_status(g_status_msg);
  print_op_queue(schedule->ready_queue_low);
  sprintf(g_status_msg, "...[Defunct Queue - %d Processes]", op_get_count(schedule->defunct_queue));
  print_status(g_status_msg);
  print_op_queue(schedule->defunct_queue);
}

// Prints a single Scheduler Queue
void print_op_queue(Op_queue_s *queue) {
  Op_process_s *walker = queue->head;
  while(walker != NULL) {
    print_process_node(walker);
    walker = walker->next;
  }
}

// Prints a schedule tracked process
void print_process_node(Op_process_s *node) {
  if((node->state >> 28)&1) {
    sprintf(g_status_msg, "     [PID :%d] %s%s %s (Exit Code: %d)", node->pid, ((node->state>>31)&1)?"[C]":"", ((node->state>>30)&1)?"[L]":"", node->cmd, ((node->state)&0x0FFFFFFF));
  }
  else {
    sprintf(g_status_msg, "     [PID :%d] %s%s %s", node->pid, ((node->state>>31)&1)?"[C]":"", ((node->state>>30)&1)?"[L]":"", node->cmd);
  }
  print_status(g_status_msg);
}

// Starts the CS Processing
void start_cs() {
  pthread_mutex_lock(&cs_run_m);
  if(cs_run == 0) {
    cs_run = 1;
    pthread_mutex_unlock(&cs_cv_m);
  }
  pthread_mutex_unlock(&cs_run_m);
}

// Stops the CS Processing
void stop_cs() {
  pthread_mutex_lock(&cs_run_m);
  if(cs_run == 1) {
    cs_run = 0;
    pthread_mutex_lock(&cs_cv_m);
  }
  pthread_mutex_unlock(&cs_run_m);
}

// Toggles the CS Processing
void toggle_cs() {
  pthread_mutex_lock(&cs_run_m);
  int is_running = cs_run;
  pthread_mutex_unlock(&cs_run_m);
  if(is_running) {
    print_status("Stopping the CS System");
    stop_cs();
  }
  else {
    sprintf(g_status_msg, "Starting CS System: %d usec Run, %d usec Between", sleep_usec_time, between_usec_time);
    print_status(g_status_msg);
    start_cs();
  }
}

// Returns the state AS OF THE TIME OF CALLING of the CS System
void print_cs_status() {
  pthread_mutex_lock(&cs_run_m);
  int state = cs_run;
  pthread_mutex_unlock(&cs_run_m);
  if(state == 1) {
    sprintf(g_status_msg, "CS System Running: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
    print_status(g_status_msg);
  }
  else {
    sprintf(g_status_msg, "CS System Stopped: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
    print_status(g_status_msg);
  }
  return;
}

// Set the time for each process to run for (Quantum)
void set_run_usec(useconds_t time) {
  sleep_usec_time = time;
  sprintf(g_status_msg, "Setting CS System: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
  print_status(g_status_msg);
}

// Set the time between processes running
void set_between_usec(useconds_t time) {
  between_usec_time = time;
  sprintf(g_status_msg, "Setting CS System: runtime %d usec, delaytime %d usec", sleep_usec_time, between_usec_time);
  print_status(g_status_msg);
}
