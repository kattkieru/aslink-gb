/** StringList module --
    Implementation of module providing services for handling for lists
    of strings.  It only provides a specific constructor and a
    customized append routine.

    Original version by Thomas Tensi, 2007-09
*/

#include "stringlist.h"

/*========================================*/

#include "globdefs.h"
#include "file.h"
#include "list.h"
#include "string.h"

#include <stdio.h>
# define StdIO_printf printf

/*========================================*/

/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void StringList_initialize (void)
{
}

/*--------------------*/

void StringList_finalize (void)
{
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

StringList_Type StringList_make (void)
{
  return List_make(String_typeDescriptor);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void StringList_append (inout StringList_Type *list, in String_Type st)
{
  Object *objectPtr = List_append(list);
  String_Type *ptr = (String_Type *) objectPtr;
  //String_make(ptr);
  String_copy(ptr, st);
}


/*--------------------*/
/* OUTPUT             */
/*--------------------*/

void StringList_write (in StringList_Type list, inout File_Type *file,
		       in String_Type separator)
{
  List_Cursor stringCursor;

  for (stringCursor = List_resetCursor(list);
       stringCursor != NULL;
       List_advanceCursor(&stringCursor)) {
    String_Type st = List_getElementAtCursor(stringCursor);
    File_writeString(file, st);
    File_writeString(file, separator);
  }
}
