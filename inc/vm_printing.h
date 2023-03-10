
#ifndef VM_PRINTING_H
#define VM_PRINTING_H

#include "vm_settings.h"

// Modify USE_COLORS in vm_settings.h to control colors
#if USE_COLORS > 0
 #define RST     "\033[0m"
 #define BOLD    "\033[1m"

 #define RED     "\033[1;31m"
 #define GREEN   "\033[1;32m"
 #define YELLOW  "\033[1;33m"
 #define BLUE    "\033[1;34m"
 #define MAGENTA "\033[1;35m"
 #define CYAN    "\033[1;36m"
 #define WHITE   "\033[1;37m"
#else 
 #define RST     ""
 #define BOLD    ""

 #define RED     ""
 #define GREEN   ""
 #define YELLOW  ""
 #define BLUE    ""
 #define MAGENTA ""
 #define CYAN    ""
 #define WHITE   ""
#endif // USE_COLORS

#endif
