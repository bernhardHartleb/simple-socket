#ifndef NET_TempFailure_h__
#define NET_TempFailure_h__

#include <unistd.h>
#include <errno.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) (expression)
#endif

#endif // NET_TempFailure_h__
