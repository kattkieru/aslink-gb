/** List module --
    This module provides all services for generic lists.  A generic
    list can take elements of fixed size where the size is specified
    upon list creation.  Additionally elements can be searched in the
    list and appended to the list.

    For iteration a cursor can be defined on a list and is used to
    linearly traverse it and inspect, change or delete the element in
    the list where it points to.

    A type descriptor for the embedded elements is given upon
    construction of a list.  This is necessary e.g. for additional
    cleanup or allocation operations when elements are deleted or
    created.  For scalar or purely reference-based types the default
    type descriptor is okay, because those types do not need any
    additional processing in those cases.

    Original version by Thomas Tensi, 2006-08
*/

#ifndef __LIST_H
#define __LIST_H

/*========================================*/

#include "globdefs.h"
#include "typedescriptor.h"

/*========================================*/

extern TypeDescriptor_Type List_typeDescriptor;
  /** variable used for describing the type properties when list
      objects occur in generic types (like other lists) */

/*--------------------*/

typedef struct List__Record *List_Type;
  /** generic list */

typedef struct List__Linkable *List_Cursor;
  /** cursor to some element within a list */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void List_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void List_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* TYPE CHECKING      */
/*--------------------*/

Boolean List_isValid (in Object list);
  /** checks whether <list> is a valid list */

/*--------------------*/

TypeDescriptor_Type List_getElementType (in List_Type list);
  /** returns the type descriptor for the elements in <list> */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

List_Type List_make (in TypeDescriptor_Type typeDescriptor);
  /** constructs a single list with elements of the type specified in
      <typeDescriptor>; this parameter specifies element size and how
      single elements behave on construction, destruction and
      assignment and how they are compared */


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void List_destroy (inout List_Type *list);
  /** destroys all elements in <list> and list itself */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

Object List_lookup (in List_Type list, in Object key);
  /** searches <list> for element with identification <key> and
      returns it or NULL if no such element exists */

/*--------------------*/

Object List_getElement (in List_Type list, in SizeType i);
  /** returns <i>-th element of <list> */


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

SizeType List_length (in List_Type list);
  /** returns the length of <list> */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

Object *List_append (inout List_Type *list);
  /** appends newly allocated element to end of <list> and returns
      a pointer this new list element */

/*--------------------*/

void List_clear (inout List_Type *list);
  /** removes all elements in <list> */

/*--------------------*/

void List_copy (inout List_Type *destination, in List_Type source);
  /** copies contents from <source> to <destination> */

/*--------------------*/

void List_concatenate (inout List_Type *list, in List_Type otherList);
  /** concatenates contents of <otherList> to <list> */


/*--------------------*/
/* ITERATION          */
/*--------------------*/

List_Cursor List_resetCursor (in List_Type list);
  /** returns cursor on head of <list>; if list is empty, result is
      NULL */

/*--------------------*/

List_Cursor List_setCursorToElement (in List_Type list, in Object key);
  /* sets cursor on entry in <list> which has <key>; when no such
     element exists, NULL is returned */

/*--------------------*/

Object List_getElementAtCursor (in List_Cursor cursor);
  /** gets element pointed at by <cursor> */

/*--------------------*/

void List_putElementToCursor (in List_Cursor cursor, in Object newValue);
  /** assigns <newValue> to element pointed at by <cursor> */

/*--------------------*/

void List_deleteElementAtCursor (in List_Cursor cursor);
  /* deletes element in list at <cursor>; when cursor is past list
     end, the behaviour is undefined */

/*--------------------*/

void List_advanceCursor (inout List_Cursor *cursor);
  /** advances <cursor> by one element within associated list; if list
      is exhausted, <cursor> is set to NULL */

#endif /* __LIST_H */
