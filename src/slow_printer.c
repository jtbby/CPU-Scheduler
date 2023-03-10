
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#define SLEEP_USEC 500000 // 1000 = 1ms, 1000000 = 1sec
#define ITERATIONS 10 // Number of times to print

int main(int argc, char *argv[]){
  int counter = 1;
  char *end_ptr = NULL;

  if (argc==1) {
    counter = ITERATIONS;
  }
  else {
    errno = 0;
    counter = strtol(argv[1], &end_ptr, 10);
    if(errno || end_ptr == NULL || *end_ptr != '\0') {
      printf("Error: Could not Convert %s to a number.\n", argv[1]);
      exit(EXIT_FAILURE);
    }
  }

  int i = 0;
  for(i = 0; i <= counter; i++) {
    printf("[PID: %d] slow_printer %d...\n", getpid(), i);
    usleep(SLEEP_USEC);
  }
  
  return counter;
}
