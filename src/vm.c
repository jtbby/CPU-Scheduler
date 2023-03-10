
// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
// Project Includes
#include "vm_support.h"
#include "vm_shell.h"
#include "vm_process.h"
#include "vm_printing.h"
#include "vm_cs.h"

/* Project Globals */
int debug_mode = DEFAULT_DEBUG; // Debug Mode is OFF (0) to begin.

/* Handlers */
// Segmentation Fault Detected in Trilby-VM
void hnd_sigsegv(int sig) {
  abort_error("Segmentation Fault Detected!\n", __FILE__);
}

// Ctrl-C was entered by the User (Toggle the CS System)
void hnd_sigint(int sig) {
  toggle_cs();
}

/* Functions */
// Cleanup will ensure all threads are finished and, most importantly, 
//  all our created processes are well and truly killed.
// Registered with atexit()
void vm_cleanup() {
  print_status("Cleaning up VM environment.");
  cs_cleanup();
  deallocate_process_system();
}

// Set up the main VM environment, then drop to a user shell.
int main() {
  register_signal(SIGSEGV, hnd_sigsegv);
  register_signal(SIGINT, hnd_sigint);

  print_trilby_banner();
  // Registers a function to be called on exit.  
  // ** SIGSEGV is also handled to pass to exit.
  atexit(vm_cleanup);

  // Begin Running the Context Switch Threading System
  initialize_cs_system();
  // Set up main VM Environment to handle and track Jobs
  initialize_process_system(); 

  // Enter the user shell
  shell();

  // All exits from this program will call an atexit
  return EXIT_SUCCESS;
}
