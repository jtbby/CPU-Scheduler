/* 
 * Name: Justin Thomas
 */

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
// Local Includes
#include "op_sched.h"
#include "vm_support.h"
#include "vm_process.h"

#define CRITICAL_FLAG   (1 << 31) 
#define LOW_FLAG        (1 << 30) 
#define READY_FLAG      (1 << 29)
#define DEFUNCT_FLAG    (1 << 28)
#define MAX_AGE 5 


/* Initializes the Op_schedule_s Struct and all of the Op_queue_s Structs
 * Follow the project documentation for this function.
 * Returns a pointer to the new Op_schedule_s or NULL on any error.
 */
Op_schedule_s *op_create() {
 
  Op_schedule_s *new = malloc(sizeof(Op_schedule_s));
  
  new->ready_queue_high = malloc(sizeof(Op_queue_s));
  new->ready_queue_low = malloc(sizeof(Op_queue_s));
  new->defunct_queue = malloc(sizeof(Op_queue_s));

  new->ready_queue_high->head = NULL;
  new->ready_queue_low->head = NULL;
  new->defunct_queue->head = NULL;

  new->ready_queue_high->count = 0;
  new->ready_queue_low->count = 0;
  new->defunct_queue->count = 0;

  if(new == NULL) {
    return NULL;
  }

  return new; // Replace This Line with Your Code!
}

/* Create a new Op_process_s with the given information.
 * - Malloc and copy the command string, don't just assign it!
 * Follow the project documentation for this function.
 * Returns the Op_process_s on success or a NULL on any error.
 */
Op_process_s *op_new_process(char *command, pid_t pid, int is_low, int is_critical) {

  int mask; /* mask variable that's free to use */
  Op_process_s *newProcess = malloc(sizeof(Op_process_s));

  newProcess->state &= ~(DEFUNCT_FLAG); /* set defunct to 0 by clearing it with AND operation + complement */
  newProcess->state |= (READY_FLAG); /* set ready to 1 by OR operation and shifting */

  if(is_low == 0) { /* if is_low is false (0) then change bit 30 to 0 and everything else to the same */
    newProcess->state &= ~(LOW_FLAG);
  } else {
    newProcess->state |= (LOW_FLAG);
  }

  if(is_critical == 0) { /* if is_critical is false (0) then change bit 31 to 0 and everything else to the same */
    newProcess->state &= ~(CRITICAL_FLAG);
  } else {
    newProcess->state |= (CRITICAL_FLAG);
  }

  mask = 0xf0000000; /* have a mask with 1111 for bit 28+29+30+31 */

  newProcess->state &= mask; /* use the mask to keep the current bits from 28-31 the same and make the rest all 0*/

  newProcess->age = 0;

  newProcess->pid = pid;

  newProcess->cmd = malloc(MAX_CMD * sizeof(char));

  strncpy(newProcess->cmd, command, MAX_CMD * ( sizeof(char)));

  newProcess->next = NULL;

  return newProcess; // Replace This Line with Your Code!
}

/* Adds a process into the appropriate singly linked list queue.
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int op_add(Op_schedule_s *schedule, Op_process_s *process) {
  
  if(schedule == NULL) {
    return -1;
  }

  Op_process_s *current; /* Node to use to iterate from head */

  process->state |= READY_FLAG; 
  process->state &= ~(DEFUNCT_FLAG);

  if((LOW_FLAG & process->state) == LOW_FLAG) { /* Insert a node at ready queue low*/
    
    if(schedule->ready_queue_low->head == NULL) {
      
      schedule->ready_queue_low->head = process;
      schedule->ready_queue_low->head->next = NULL;
      schedule->ready_queue_low->count++;

      return 0;

    } else {
      
      current = schedule->ready_queue_low->head;
      
      while(current->next != NULL) {
        current = current->next;
      }

      current->next = process;
      current->next->next = NULL;
      schedule->ready_queue_low->count++;

      return 0;
    }

  }

  if((LOW_FLAG & process->state) == 0) { /* Insert a node at ready queue high */

    if(schedule->ready_queue_high->head == NULL) {

      schedule->ready_queue_high->head = process;
      schedule->ready_queue_high->head->next = NULL;
      schedule->ready_queue_high->count++;
      return 0;

    } else {

      current = schedule->ready_queue_high->head;

      while (current->next != NULL) {
        current = current->next;
      }
      
      current->next = process;
      current->next->next = NULL;
      schedule->ready_queue_high->count++;

      return 0;

    }

  }

  return -1;

/* Returns the number of items in a given Op_queue_s
 * Follow the project documentation for this function.
 * Returns the number of processes in the list or -1 on any errors.
 */
int op_get_count(Op_queue_s *queue) {

  if(queue == NULL) {
    return -1;
  }

  return queue->count; // Replace This Line with Your Code!
}

/* Selects the next process to run from the High Ready Queue.
 * Follow the project documentation for this function.
 * Returns the process selected or NULL if none available or on any errors.
 */
Op_process_s *op_select_high(Op_schedule_s *schedule) {
  

  Op_process_s *current;
  Op_process_s *prev;

  if(schedule == NULL) {  /* Error Check */
    return NULL;
  }
  
  if (schedule->ready_queue_high->head == NULL) { /* Check If No processes available */
    return NULL;
  }
  
  current = schedule->ready_queue_high->head;

  if((current->state & CRITICAL_FLAG) != 0) { /* Check Head First */
    schedule->ready_queue_high->head = current->next;
    
    current->age = 0;
    current->next = NULL;

    schedule->ready_queue_high->count--;

    return current;
    
  }

  prev =  current;
  current = current->next;

  while(current != NULL) { /* Iterate through list to find critical flag */
    
    if((current->state & CRITICAL_FLAG) != 0) {
      
      prev->next = current->next;
      
      current->age = 0;
      current->next = NULL;

      schedule->ready_queue_high->count--;

      return current;
    }

    prev =  current;
    current = current->next;

  }


  current = schedule->ready_queue_high->head; /* Remove first process that isn't critical */
  schedule->ready_queue_high->head = current->next;
  current->age = 0;
  current->next = NULL;
  

  return current; // Replace This Line with Your Code!
}

/* Schedule the next process to run from the Low Ready Queue.
 * Follow the project documentation for this function.
 * Returns the process selected or NULL if none available or on any errors.
 */
Op_process_s *op_select_low(Op_schedule_s *schedule) {
  
  Op_process_s *current;

  if(schedule == NULL) { /* Error Check */
    return NULL;
  }
  
  if (schedule->ready_queue_low->head == NULL) { /* Error Check */
    return NULL;
  }
  
  current = schedule->ready_queue_low->head;
  schedule->ready_queue_high->head = current->next;
  current->age = 0;
  current->next = NULL;

   

  return current;
}

/* Add age to all processes in the Ready - Low Priority Queue, then
 *  promote all processes that are >= MAX_AGE.
 * Follow the project documentation for this function.
 * Returns a 0 on success or -1 on any errors.
 */
int op_promote_processes(Op_schedule_s *schedule) {
  Op_process_s *current;
  Op_process_s *iterator; /* Use a second iterating node to preserve the current pointer */
  Op_process_s *prev;

  if (schedule == NULL) { /* Error Check */
    return -1;
  }

  if (schedule->ready_queue_low->head == NULL) { /* Empty queue check */
    return 0;
  }

   current = schedule->ready_queue_low->head;

    while (current != NULL) {
      current->age++;
      current = current->next;
    }


 while(schedule->ready_queue_low->head != NULL) {/* Loop to continuosly check the low priority head until it cannot be removed*/
    
    current = schedule->ready_queue_low->head; 

    if (current->age < MAX_AGE) {
      break;
    }

    if (current->age >= MAX_AGE) {
      schedule->ready_queue_low->head = current->next;

      current->age = 0;
      current->next = NULL;
      schedule->ready_queue_low->count--;

      if(schedule->ready_queue_high->head == NULL) {/* Check if High queue is NULL*/
        schedule->ready_queue_high->head = current;
        schedule->ready_queue_high->count++;
        continue;   
      }

      iterator = schedule->ready_queue_high->head;

      while(iterator->next != NULL) {
        iterator = iterator->next;
      }

      iterator->next = current;
      schedule->ready_queue_high->count++;
    }
  }
  
  if(schedule->ready_queue_low->head == NULL) {
    return 0;
  }

  if(schedule->ready_queue_low->head->next == NULL) {
    return 0;
  }
  
  do{/* Loop through low priority queue and insert*/
    
    current = schedule->ready_queue_low->head->next;
    prev = schedule->ready_queue_low->head;

    if (current->age >= MAX_AGE) {
      prev->next = current->next;
      current->age = 0;
      current->next = NULL;
      schedule->ready_queue_low->count--;

      if(schedule->ready_queue_high->head == NULL) {
        schedule->ready_queue_high->head = current;
        schedule->ready_queue_high->count++;
        continue;   
      }
      
      iterator = schedule->ready_queue_high->head;

      while(iterator->next != NULL) {
        iterator = iterator->next;
      }

      iterator->next = current;
    }

  }while (current != NULL);

  return 0;
}

/* This is called when a process exits normally.
 * Put the given node into the Defunct Queue and set the Exit Code 
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */ 
int op_exited(Op_schedule_s *schedule, Op_process_s *process, int exit_code) {

  Op_process_s *current;

  if(schedule == NULL) {
    return -1;
  }

  if(process == NULL) {
    return -1;
  }

  process->state = process->state | DEFUNCT_FLAG;
  process->state = process->state & ~(READY_FLAG);

  process->state = process->state | exit_code;

  if(schedule->defunct_queue->head == NULL) {
    schedule->defunct_queue->head = process;
    schedule->defunct_queue->count++;
    return 0;
  }

  current = schedule->defunct_queue->head;

  while(current->next != NULL) {
    current = current->next;
  }

  current->next = process;
  current->next->next = NULL;
  schedule->defunct_queue->count++;

  return 0;

}

/* This is called when the OS terminates a process early.
 * Remove the process with matching pid from Ready High or Ready Low and add the Exit Code to it.
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int op_terminated(Op_schedule_s *schedule, pid_t pid, int exit_code) {

  Op_process_s *current;
  Op_process_s *iterator;
  Op_process_s *prev;
  
  if(schedule == NULL) {
    return -1;
  }

  if(schedule->ready_queue_high->head && schedule->ready_queue_low->head == NULL) {
    return -1;
  }

  current = schedule->ready_queue_high->head;

  if(current != NULL) {
    
    if(current->pid == pid) {/* Check Head of the Ready Queue High */
      
      schedule->ready_queue_high->head = current->next;
      
      current->next = NULL;
      
      current->state = current->state | DEFUNCT_FLAG;
      current->state = current->state & ~(READY_FLAG);
      schedule->ready_queue_high->count--;

      current->state = current->state | exit_code;
      
      if(schedule->defunct_queue->head == NULL) {
        
        schedule->defunct_queue->head = current;
        schedule->defunct_queue->count++;
        return 0;
      
      }
      iterator = schedule->defunct_queue->head;
      
      while (iterator->next != NULL) {
        iterator = iterator->next;
      }

      iterator->next = current;
      schedule->defunct_queue->count++;
      return 0;      
      
    }

    do{/* Iterate to find the process in the ready queue high */
      prev = current;
      current = current->next;

      if(current->pid == pid) {
        
        prev->next = current->next;

        current->next = NULL;
        current->state = current->state | DEFUNCT_FLAG;
        current->state = current->state & ~(READY_FLAG);

        schedule->ready_queue_high->count--;

        current->state = current->state | exit_code;

        if (schedule->defunct_queue->head == NULL) {
          schedule->defunct_queue->head = current;
          schedule->defunct_queue->count++;
          return 0;
        }

        iterator = schedule->defunct_queue->head;

        while (iterator->next != NULL) {
          iterator = iterator->next;
        }

        iterator->next = current;
        schedule->defunct_queue->count++;
        return 0;
     
      }

    }while(current->next != NULL);
    
  }

  current = schedule->ready_queue_low->head;

  if(current != NULL) {
    
    if(current->pid == pid) {/* Check Head of the Ready Queue Low */
      
      schedule->ready_queue_low->head = current->next;
      
      current->next = NULL;
      
      current->state = current->state | DEFUNCT_FLAG;
      current->state = current->state & ~(READY_FLAG);
      schedule->ready_queue_low->count--;

      current->state = current->state | exit_code;
      
      if(schedule->defunct_queue->head == NULL) {
        
        schedule->defunct_queue->head = current;
        schedule->defunct_queue->count++;
        return 0;
      
      }
      iterator = schedule->defunct_queue->head;
      
      while (iterator->next != NULL) {
        iterator = iterator->next;
      }

      iterator->next = current;
      schedule->defunct_queue->count++;
      return 0;      
      
    }

    do{/* Iterate to find the process in the ready queue low */
      prev = current;
      current = current->next;

      if(current->pid == pid) {
        
        prev->next = current->next;

        current->next = NULL;
        current->state = current->state | DEFUNCT_FLAG;
        current->state = current->state & ~(READY_FLAG);

        schedule->ready_queue_low->count--;

        current->state = current->state | exit_code;

        if (schedule->defunct_queue->head == NULL) {
          schedule->defunct_queue->head = current;
          schedule->defunct_queue->count++;
          return 0;
        }

        iterator = schedule->defunct_queue->head;

        while (iterator->next != NULL) {
          iterator = iterator->next;
        }

        iterator->next = current;
        schedule->defunct_queue->count++;
        return 0;

      }
    }while(current->next != NULL);
    
  }

  return -1;
}

/* Frees all allocated memory in the Op_schedule_s, all of the Queues, and all of their Nodes.
 * Follow the project documentation for this function.
 */
void op_deallocate(Op_schedule_s *schedule) {

  Op_process_s *current;

  if(schedule->ready_queue_high->head != NULL) {/* Free ready queue high */

    while (schedule->ready_queue_high->head != NULL) {
      current = schedule->ready_queue_high->head;
      schedule->ready_queue_high->head = schedule->ready_queue_high->head->next;
      free(current->cmd);
      free(current);
    }

  }

  if(schedule->ready_queue_low->head != NULL) {/* Free ready queue low */
    
    while (schedule->ready_queue_low->head != NULL) {
      current = schedule->ready_queue_low->head;
      schedule->ready_queue_low->head = schedule->ready_queue_low->head->next;
      free(current->cmd);
      free(current);
    }

  }

  if(schedule->defunct_queue->head != NULL) {/* Free defunct queue */
   
    while (schedule->defunct_queue->head != NULL) {
      current = schedule->defunct_queue->head;
      schedule->defunct_queue->head = schedule->defunct_queue->head->next;
      free(current->cmd);
      free(current);
      }
    }



  

  /*Free queues + Schedule */
  if(schedule->ready_queue_high != NULL) {
    free(schedule->ready_queue_high);
  }

  if(schedule->ready_queue_low != NULL) {
    free(schedule->ready_queue_low);
  }

  if(schedule->defunct_queue != NULL) {
    free(schedule->defunct_queue);
  }

  if(schedule != NULL) {
    free(schedule);
  }
}
