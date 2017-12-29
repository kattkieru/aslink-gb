/** Error module --
    Implementation of module providing all services for dealing with
    errors. There are several kinds of criticality which define
    whether only a message is written or the program is stopped
    immediately.

    Error output may be redirected to any open StdIO file and normally
    goes to stderr.

    Original version by Thomas Tensi, 2006-08
*/

#include "error.h"

/*========================================*/

#include "globdefs.h"
#include <stdarg.h>
#  define StdArg_endArgList va_end
#  define StdArg_startArgList va_start
#  define StdArg_VarArgList va_list
#include <stdlib.h>
#  define StdLib_exit exit


/*========================================*/

File_Type Error__reportingTarget;
  /** file where the error messages go to */

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Error_initialize (void)
{
  Error_setReportingTarget(File_stderr);
}

/*--------------------*/

void Error_finalize (void)
{
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Error_setReportingTarget (in File_Type reportingFile)
{
  Error__reportingTarget = reportingFile;
}

/*--------------------*/

void Error_raise (in Error_Criticality criticality, in char *message, ...)
{
  char *leadInString = "???";
  StdArg_VarArgList argumentList;

  StdArg_startArgList(argumentList, message);

  switch (criticality) {
    case Error_Criticality_warning:
      leadInString = "ASLINK Warning";
      break;
    case Error_Criticality_error:
      leadInString = "ASLINK Error";
      break;
    case Error_Criticality_fatalError:
      leadInString = "ASLINK Fatal Error";
  }

  File_writeCharArray(&Error__reportingTarget, leadInString);
  File_writeCharArray(&Error__reportingTarget, ": ");

  /* print out remainder of message */
  File_writePrintfArguments(&Error__reportingTarget, message, argumentList);
  StdArg_endArgList(argumentList);
  File_writeChar(&Error__reportingTarget, '\n');

  if (criticality == Error_Criticality_fatalError) {
    StdLib_exit(1);
  }
}
