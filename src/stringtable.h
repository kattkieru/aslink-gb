/** StringTable module --
    This module provides all services for handling two string tables
    in the SDCC linker.  Those string tables contain the global base
    address definitions and the global symbol definitions as strings.

    Original version by Thomas Tensi, 2007-03
*/

#ifndef __STRINGTABLE_H
#define __STRINGTABLE_H

/*========================================*/

#include "globdefs.h"
#include "stringlist.h"

/*========================================*/

typedef StringList_Type StringTable_Type;
extern StringTable_Type StringTable_baseAddressList;
extern StringTable_Type StringTable_globalDefList;

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void StringTable_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void StringTable_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void StringTable_addCharArray (inout StringTable_Type *stringTable,
			       in char *st);
  /** adds character array <st> to string table <stringTable> */

#endif /* __STRINGTABLE_H */
