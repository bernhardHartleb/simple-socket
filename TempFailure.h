#ifndef _TEMPFAILURE_H
#define _TEMPFAILURE_H

#include <unistd.h>
#include <errno.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) (expression)
#endif

#endif
