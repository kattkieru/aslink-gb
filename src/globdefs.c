/** GlobDefs module --
    This module provides the implementation for elementary types and
    routines.

    Original version by Thomas Tensi, 2008-01
*/

#include "globdefs.h"

/*========================================*/

#include "error.h"

#include <stdio.h>
#  define StdIO_stderr stderr
#  define StdIO_fprintf fprintf
#include <stdlib.h>
#  define StdLib_exit exit

/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static void checkCondition (in Boolean condition, in char *format,
			    in char *procName, in char *message)

{
  if (condition) {
  } else {
    Error_raise(Error_Criticality_fatalError, format, procName, message);
  }
}


/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

void ASSERTION (in Boolean condition, in char *procName, in char *message)
{
  checkCondition(condition, "assertion violation in %s: %s",
		 procName, message);
}

/*--------------------*/

Object attemptConversion (in char *opaqueTypeName, in Object object,
			  in long magicNumber)
{
  if (isValidObject(object, magicNumber)) {
    return object;
  } else {
    StdIO_fprintf(StdIO_stderr, "%s %s %s",
                "bad ", opaqueTypeName, " object" );
    StdLib_exit(1);
  }
}

/*--------------------*/

Boolean isValidObject (in Object object, in long magicNumber)
{
  long *x = (long *) object;
  return (x != NULL && *x == magicNumber);
}

/*--------------------*/

Boolean PRE (in Boolean condition, in char *procName, in char *message)
{
  checkCondition(condition, "precondition violation in %s: %s",
		 procName, message);
  return condition;
}

