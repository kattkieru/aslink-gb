/** List module --
    Implementation of module providing all services for generic lists.

    Original version by Thomas Tensi, 2006-07
*/

#include "list.h"

/*========================================*/

#include <stddef.h>
#include "globdefs.h"
#include "error.h"

/*========================================*/

#define List__magicNumber 0x87654321

typedef struct List__Linkable {
  List_Type list;
  struct List__Linkable *next;
  Object data;
} List__Linkable;
  /** list element type for a single linked list */


typedef struct List__Record {
  UINT32 magicNumber;
  TypeDescriptor_Type typeDescriptor;
  List_Cursor first;
  List_Cursor last;
} List__Record;
  /** list header record type with a leading information about the
      element size, several pointers to routines (for construction,
      assignment etc.) and pointers to first and last element (this
      type information should only be used internally!) */


static TypeDescriptor_Record List__tdRecord = 
  { /* .objectSize = */ sizeof(List_Type),
    /* .assignmentProc = */ (TypeDescriptor_AssignmentProc) List_copy,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ NULL,
    /* .destructionProc = */ (TypeDescriptor_DestructionProc) List_destroy,
    /* .hashCodeProc = */ NULL, /* .keyValidationProc = */ NULL };

TypeDescriptor_Type List_typeDescriptor = &List__tdRecord;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static List_Type List__attemptConversion (in Object list)
{
  return attemptConversion("List_Type", list, List__magicNumber);
}

/*--------------------*/

static Boolean List__checkValidityPRE (in Object list, in char *procName)
  /** checks as a precondition of routine <procName> whether <list> is
      a valid list and returns the check result */
{
  return PRE(List_isValid(list), procName, "invalid list");
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void List_initialize (void)
{
}

/*--------------------*/

void List_finalize (void)
{
}

/*--------------------*/
/* TYPE CHECKING      */
/*--------------------*/

Boolean List_isValid (in Object list)
{
  return isValidObject(list, List__magicNumber);
}

/*--------------------*/

TypeDescriptor_Type List_getElementType (in List_Type list)
{
  char *procName = "List_getElementType";
  Boolean precondition = List__checkValidityPRE(list, procName);
  TypeDescriptor_Type result = NULL;

  if (precondition) {
    result = list->typeDescriptor;
  }

  return result;
}

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

List_Type List_make (in TypeDescriptor_Type typeDescriptor)
{
  List_Type list = NEW(List__Record);
  list->magicNumber       = List__magicNumber;
  list->typeDescriptor    = typeDescriptor;
  list->first             = NULL;
  list->last              = NULL;
  return list;
}

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void List_destroy (inout List_Type *list)
{
  List_clear(list);
  DESTROY(*list);
  *list = NULL;
}

/*--------------------*/
/* ACCESS             */
/*--------------------*/

Object List_lookup (in List_Type list, in Object key)
{
  char *procName = "List_lookup";
  Boolean precondition = List__checkValidityPRE(list, procName);
  Object result = NULL;

  if (precondition) {
    List_Cursor cursor = list->first;
    Boolean isFound = false;

    do {
      if (cursor != NULL) {
	Object currentObject = cursor->data;
	isFound = TypeDescriptor_checkObjectForKey(list->typeDescriptor,
						   currentObject, key);
	if (isFound) {
	  result = currentObject;
	} else {
	  cursor = cursor->next;
	}
      }
    } while (cursor != NULL && !isFound);
  }

  return result;
}

/*--------------------*/

Object List_getElement (in List_Type list, in SizeType i)
{
  char *procName = "List_getElement";
  Boolean precondition = List__checkValidityPRE(list, procName);
  Object result = NULL;

  if (precondition) {
    if (i > 0) {
      List_Cursor cursor = list->first;
      SizeType count = 1;

      while (cursor != NULL && count < i) {
	cursor = cursor->next;
	count++;
      }

      if (cursor != NULL) {
	result = cursor->data;
      }
    }
  }

  return result;
}

/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

SizeType List_length (in List_Type list)
{
  char *procName = "List_length";
  Boolean precondition = List__checkValidityPRE(list, procName);
  SizeType length = 0;

  if (precondition) {
    List_Cursor cursor = list->first;

    while (cursor != NULL) {
      cursor = cursor->next;
      length++;
    }
  }

  return length;
}

/*--------------------*/
/* CHANGE             */
/*--------------------*/

Object *List_append (inout List_Type *list)
{
  char *procName = "List_append";
  List_Type currentList = *list;
  Boolean precondition = List__checkValidityPRE(currentList, procName);
  Object *result = NULL;

  if (precondition) {
    TypeDescriptor_Type typeDescriptor = currentList->typeDescriptor;
    List_Cursor cursor = NEW(List__Linkable);

    cursor->list = currentList;
    cursor->next = NULL;
    cursor->data = TypeDescriptor_makeObject(typeDescriptor);

    if (currentList->last == NULL) {
      /* list is empty */
      currentList->first = cursor;
    } else {
      currentList->last->next = cursor;
    }
    
    currentList->last = cursor;
    result = &cursor->data;
  }

  return result;
}

/*--------------------*/

void List_clear (inout List_Type *list)
{
  char *procName = "List_clear";
  List_Type currentList = *list;
  Boolean precondition = List__checkValidityPRE(currentList, procName);

  if (precondition) {
    List_Cursor cursor = currentList->first;
    TypeDescriptor_Type typeDescriptor = currentList->typeDescriptor;

    while (cursor != NULL) {
      List_Cursor nextElement = cursor->next;
      TypeDescriptor_destroyObject(typeDescriptor, &cursor->data);
      DESTROY(cursor);
      cursor = nextElement;
    }

    currentList->first = NULL;
    currentList->last  = NULL;
  }
}

/*--------------------*/

void List_copy (inout List_Type *destination, in List_Type source)
{
  char *procName = "List_copy";
  TypeDescriptor_Type typeDescriptor = (*destination)->typeDescriptor;
  Boolean precondition = (PRE(List_isValid(*destination), procName,
			     "invalid destination")
			  && PRE(List_isValid(source), procName,
			     "invalid source")
			  && PRE(typeDescriptor == source->typeDescriptor,
				 procName, "incompatible list types"));

  if (precondition) {
    List_clear(destination);
    List_concatenate(destination, source);
  }
}

/*--------------------*/

void List_concatenate (inout List_Type *list, in List_Type otherList)
{
  char *procName = "List_concatenate";
  Boolean precondition = (PRE(List_isValid(*list), procName,
			     "invalid destination")
			  && PRE(List_isValid(otherList), procName,
			     "invalid source")
			  && PRE((*list)->typeDescriptor
				 == otherList->typeDescriptor, procName,
				 "incompatible list types"));

  if (precondition) {
    List_Cursor cursor = otherList->first;

    while (cursor != NULL) {
      Object currentObject = List_getElementAtCursor(cursor);
      List_Cursor newElementCursor;
      Object *objectPtr = List_append(list);

      ASSERTION(objectPtr != NULL, procName, "out of memory");
      newElementCursor = (*list)->last;
      List_putElementToCursor(newElementCursor, currentObject);
      cursor = cursor->next;
    }
  }
}


/*--------------------*/
/* ITERATION          */
/*--------------------*/

List_Cursor List_resetCursor (in List_Type list)
{
  char *procName = "List_resetCursor";
  Boolean precondition = List__checkValidityPRE(list, procName);
  List_Cursor cursor = NULL;

  if (precondition) {
    cursor = list->first;
  }

  return cursor;
}

/*--------------------*/

List_Cursor List_setCursorToElement (in List_Type list, in Object key)
{
  char *procName = "List_setCursorToElement";
  Boolean precondition = List__checkValidityPRE(list, procName);
  List_Cursor result = NULL;

  if (precondition) {
    List_Cursor cursor = list->first;
    TypeDescriptor_Type typeDescriptor = list->typeDescriptor;
    Boolean isFound = false;

    do {
      if (cursor != NULL) {
	Object currentObject = cursor->data;
	isFound = TypeDescriptor_checkObjectForKey(typeDescriptor,
						   currentObject, key);
	if (isFound) {
	  result = cursor;
	} else {
	  cursor = cursor->next;
	}
      }
    } while (cursor != NULL && !isFound);
  }

  return result;
}

/*--------------------*/

Object List_getElementAtCursor (in List_Cursor cursor)
{
  char *procName = "List_getElementAtCursor";
  Boolean precondition = PRE(cursor != NULL, procName, "cursor off");
  Object result = NULL;

  if (precondition) {
    result = cursor->data;
  }

  return result;
}

/*--------------------*/

void List_putElementToCursor (in List_Cursor cursor, in Object newValue)
{
  char *procName = "List_setCursorToElement";
  Boolean precondition = PRE(cursor != NULL, procName, "cursor off");

  if (precondition) {
    List_Type list = List__attemptConversion(cursor->list);
    TypeDescriptor_assignObject(list->typeDescriptor,
				&cursor->data, newValue);
  }
}

/*--------------------*/

void List_deleteElementAtCursor (in List_Cursor cursor)
{
  char *procName = "List_deleteElementAtCursor";
  Boolean precondition = PRE(cursor != NULL, procName, "cursor off");

  if (precondition) {
    List_Type list = List__attemptConversion(cursor->list);
    List_Cursor previousElement;

    /* get rid of information in current element */
    TypeDescriptor_destroyObject(list->typeDescriptor, &cursor->data);

    /* unlink current element from list */
    if (list->first == cursor) {
      previousElement = NULL;
      list->first = cursor->next;
    } else {
      previousElement = list->first;

      while (previousElement->next != cursor) {
	previousElement = previousElement->next;
      }

      previousElement->next = cursor->next;
    }

    if (list->last == cursor) {
      list->last = previousElement;
    }

    DESTROY(cursor);
  }
}

/*--------------------*/

void List_advanceCursor (inout List_Cursor *cursor)
{
  char *procName = "List_advanceCursor";
  Boolean precondition = PRE(cursor != NULL, procName, "cursor off");

  if (precondition) {
    *cursor = (*cursor)->next;
  }
}

