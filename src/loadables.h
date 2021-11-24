#ifndef __LOADABLES_H_
#define __LOADABLES_H_

//#include "bash/builtins.h"
//#include "bash/shell.h"
//#include "bash/builtins/bashgetopt.h"
#include "bash/builtins/common.h"

# if __GNUC__ >= 4
#  define PUBLIC __attribute__ ((visibility ("default")))
#  define LOCAL  __attribute__ ((visibility ("hidden")))
# else
#  define PUBLIC
#  define LOCAL
# endif

#endif
