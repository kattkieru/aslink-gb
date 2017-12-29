/** Multimap module --
    Implementation of module providing all services for generic lists.

     This implementation uses an open hash table which should be okay
     for quick access and change of map elements.

    Original version by Thomas Tensi, 2008-02
*/

#include "multimap.h"

/*========================================*/

#include "globdefs.h"
#include "error.h"
#include "list.h"
#include "typedescriptor.h"

/*========================================*/


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Multimap_initialize (void)
{
}

/*--------------------*/

void Multimap_finalize (void)
{
}


/*--------------------*/
/* TYPE CHECKING      */
/*--------------------*/

Boolean Multimap_isValid (in Object map)
{
  return Map_isValid(map);
}

/*--------------------*/

TypeDescriptor_Type Multimap_getKeyType (in Multimap_Type map)
{
  return Map_getKeyType(map);
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Multimap_Type Multimap_make (in TypeDescriptor_Type keyTypeDescriptor)
{
  return Map_make(keyTypeDescriptor);
}


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Multimap_destroy (inout Multimap_Type *map)
{
  Multimap_Type currentMap = *map;
  List_Type keyList = List_make(Map_getKeyType(currentMap));
  List_Cursor keyListCursor;

  Map_getKeyList(currentMap, &keyList);
  keyListCursor = List_resetCursor(keyList);

  while (keyListCursor != NULL) {
    Object key = List_getElementAtCursor(keyListCursor);
    List_Type value = Map_lookup(currentMap, key);
    List_destroy(&value);
    List_advanceCursor(&keyListCursor);
  }
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

List_Type Multimap_lookup (in Multimap_Type map, in Object key)
{
  return Map_lookup(map, key);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Multimap_clear (inout Multimap_Type *map)
{
  Map_clear(map);
}

/*--------------------*/

void Multimap_add (inout Multimap_Type *map,
		   in Object key, in Object value)
{
  Multimap_Type currentMap = *map;
  List_Type valueList = Map_lookup(currentMap, key);
  Boolean keyIsNew = (valueList == NULL);
  Object valueObject;

  if (keyIsNew) {
    valueList = List_make(TypeDescriptor_default);
    Map_set(&currentMap, key, (Object) valueList);
  }

  valueObject = List_lookup(valueList, value);

  if (valueObject == NULL) {
    Object *newValueObject = List_append(&valueList);
    *newValueObject = value;
  }
}

/*--------------------*/

void Multimap_deleteKey (inout Multimap_Type *map, in Object key)
{
  Map_deleteKey(map, key);
}

/*--------------------*/

void Multimap_deleteValue (inout Multimap_Type *map,
			   in Object key, in Object value)
{

  Multimap_Type currentMap = *map;
  List_Type valueList = Map_lookup(currentMap, key);

  if (valueList != NULL) {
    List_Cursor cursor = List_setCursorToElement(valueList, value);

    if (cursor != NULL) {
      List_deleteElementAtCursor(cursor);
    }
  }
}
