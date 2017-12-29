/** Map module --
    This module provides all services for generic maps.  Those maps
    represent partial functions from keys to values and have at most
    one value for some key.

    It is possible to add some value for a key, check the assigned
    values, remove some value or even delete the assignment for some
    key completely.

    Because map entries are inserted, assigned to and discarded, a
    type descriptor for keys must be provided when a map is created.
    There is no such descriptor for the values in the maps, because
    they are always considered as non-unique references and require no
    specific action.

    Original version by Thomas Tensi, 2008-01
*/

#ifndef __MAP_H
#define __MAP_H

/*========================================*/

#include "globdefs.h"
#include "list.h"
#include "typedescriptor.h"

/*========================================*/

typedef struct Map__Record *Map_Type;
  /** map type based on private underlying structure type */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Map_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void Map_finalize (void);
  /** cleans up internal data structures for this module */

/*--------------------*/
/* TYPE CHECKING      */
/*--------------------*/

Boolean Map_isValid (in Object map);
  /** checks whether <map> is a valid map */

/*--------------------*/

TypeDescriptor_Type Map_getKeyType (in Map_Type map);
  /** gets type descriptor for keys in <map> */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Map_Type Map_make (in TypeDescriptor_Type keyTypeDescriptor);
  /** constructs a single map with keys conforming to
      <keyTypeDescriptor> and object values */

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Map_destroy (out Map_Type *map);
  /** destroys all elements in <map> and map itself */

/*--------------------*/
/* ACCESS             */
/*--------------------*/

Object Map_lookup (in Map_Type map, in Object key);
  /** searches <map> for element with identification <key> and returns
      associated value or NULL if none exists */

/*--------------------*/

void Map_getKeyList (in Map_Type map, inout List_Type *keyList);
  /** gets list of keys of <map> and returns them in <keyList> */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Map_clear (inout Map_Type *map);
  /** removes all elements in <map> */

/*--------------------*/

void Map_set (inout Map_Type *map, in Object key, in Object value);
  /** sets <value> for <key> in <map> */

/*--------------------*/

void Map_deleteKey (inout Map_Type *map, in Object key);
  /** removes <key> from <map>; when <key> is not in <map>, nothing
      happens */


#endif /* __MAP_H */
