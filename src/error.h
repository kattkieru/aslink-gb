/** Error module --
    This module provides all services for dealing with errors. Errors
    are classified into several kinds of criticality which define
    whether only an informational message is written or the program has
    to be stopped immediately because of a fatal situation.

    Error output normally goes to stderr, but may be redirected to any
    open output file.

    Original version by Thomas Tensi, 2006-08
*/

#ifndef __ERROR_H
#define __ERROR_H

/*========================================*/

#include "file.h"
#include "globdefs.h"

/*========================================*/

typedef enum {
  Error_Criticality_warning, Error_Criticality_error,
  Error_Criticality_fatalError
} Error_Criticality;

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Error_initialize (void);
  /** sets up internal data structures */

/*--------------------*/

void Error_finalize (void);
  /** cleans up internal data structures */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Error_setReportingTarget (in File_Type reportingFile);
  /** all subsequent error output is directed to <reportingFile> */

/*--------------------*/

void Error_raise (in Error_Criticality criticality, in char *message, ...);
  /** raises an error with <criticality> displaying <message>; the
      message string may be a printf-style template with parameters
      which are filled by the optional arguments */


#endif /* __ERROR_H */

