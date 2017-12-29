/** StringList module --
    This module provides services for lists of strings.

    It only provides a specific constructor, a customized append
    routine and a write routine.  For all other operations the
    standard list routines must be used.

    Original version by Thomas Tensi, 2007-04
*/

#ifndef __STRINGLIST_H
#define __STRINGLIST_H

/*--------------------*/

#include "file.h"
#include "list.h"
#include "string.h"

/*--------------------*/

typedef List_Type StringList_Type;

/*========================================*/

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

StringList_Type StringList_make (void);
  /** constructs a single list with string elements */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void StringList_append (inout StringList_Type *list, in String_Type st);
  /** appends string <st> to end of <list> */


/*--------------------*/
/* OUTPUT             */
/*--------------------*/

void StringList_write (in StringList_Type list, inout File_Type *file,
		       in String_Type separator);
  /** writes <list> to <file>, where all entries are terminated by
      <separator> */


#endif /* __STRINGLIST_H */
