/** StringTable module --
    Implementation of module providing services for handling two
    string tables in the SDCC linker.  Those string tables contain the
    global base address definitions and the global symbol definitions
    as strings.

    Original version by Thomas Tensi, 2007-09
*/

#include "stringtable.h"

/*========================================*/

#include "globdefs.h"
#include "list.h"
#include "string.h"
#include "stringlist.h"

/*========================================*/

StringTable_Type StringTable_baseAddressList;
StringTable_Type StringTable_globalDefList;

/*========================================*/

/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void StringTable_initialize (void)
{
  StringTable_baseAddressList = StringList_make();
  StringTable_globalDefList   = StringList_make();
}

/*--------------------*/

void StringTable_finalize (void)
{
  List_destroy(&StringTable_baseAddressList);
  List_destroy(&StringTable_globalDefList);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void StringTable_addCharArray (inout StringTable_Type *stringTable,
			       in char *st)
{
  String_Type str = String_makeFromCharArray(st);
  StringList_append(stringTable, str);
  String_destroy(&str);
}
