#ifndef VM_SUPPORT_H
#define VM_SUPPORT_H

#include "op_sched.h"
#include "vm_printing.h"

#define MARK(str, ...) do {                   \
  printf("  %s[MARK]%s ", MAGENTA, RST);             \
  printf(str, ##__VA_ARGS__);    \
  printf("    %s{%s:%d in %s}%s\n", MAGENTA, __FILE__, __LINE__, __func__, RST); \
} while(0) 

void register_signal(int sig, void (*handler)(int));
void print_prompt();
void print_status(char *msg);
void print_debug(char *msg);
void print_warning(char *msg);
void abort_error(char *msg, char *src);
void print_trilby_banner();
void print_op_debug(Op_schedule_s *schedule);
void print_op_queue_debug(Op_queue_s *queue);
void print_process_node_debug(Op_process_s *node);

#endif
