/** NoICEMapFile module --
    This module provides all services for putting out mapfiles in
    NoICE format.  The output routine defined here links into the
    generic MapFile module of the SDCC linker.

    Original version by Thomas Tensi, 2009-03
    based on module lknoice.c by John Hartman
*/

#ifndef __NOICEMAPFILE_H
#define __NOICEMAPFILE_H

/*========================================*/

#include "file.h"
#include "globdefs.h"

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void NoICEMapFile_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void NoICEMapFile_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void NoICEMapFile_addSpecialComment (inout File_Type *file,
				     in String_Type comment);
  /** adds <comment> conditionally to NOICE <file> when it contains a
      relevant information */

/*--------------------*/

void NoICEMapFile_generate (inout File_Type *file);
  /** writes a map file in NoICE format to <file> */


#endif /* __NOICEMAPFILE_H */
