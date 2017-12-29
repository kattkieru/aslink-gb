/** ListingUpdater module --
    This module provides all services for augmenting listing files.

    Based on the list of link files, the central routine <update>
    scans the appropriate directories for associated assembler
    listings and inserts the relocated code at appropriate places.
    The code is queried from the Target module and is only available
    at the end of the second linking pass.

    Note that the module knows very much about the structure of a
    assembler listing file and is fragile whenever that structure
    changes in the future.

    Original version by Thomas Tensi, 2008-06
*/

#ifndef __LISTINGUPDATER_H
#define __LISTINGUPDATER_H

/*========================================*/

#include "globdefs.h"
#include "stringlist.h"

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void ListingUpdater_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void ListingUpdater_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* TRANSFORMATION     */ 
/*--------------------*/

void ListingUpdater_update (in UINT8 base, in StringList_Type linkFileList);
  /** update listings with radix <base> and list of linked files
      <linkFileList> */

#endif /* __LISTINGUPDATER_H */
