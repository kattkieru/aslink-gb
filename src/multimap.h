/** Multimap module --
    This module provides all services for generic multimaps.  Those
    multimaps represent partial functions from keys to sets of values
    and have at most one set of values for some key.

    It is possible to add some value for a key, check the assigned
    values, remove some value or even delete the assignment for some
    key completely.

    Original version by Thomas Tensi, 2008-01
*/

#ifndef __MULTIMAP_H
#define __MULTIMAP_H

/*========================================*/

#include "globdefs.h"
#include "map.h"
#include "typedescriptor.h"

/*========================================*/

typedef Map_Type Multimap_Type;
  /** multimap type based on map type */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Multimap_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void Multimap_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* TYPE CHECKING      */
/*--------------------*/

Boolean Multimap_isValid (in Object map);
  /** checks whether <map> is a valid multimap */

/*--------------------*/

TypeDescriptor_Type Multimap_getKeyType (in Multimap_Type map);
  /** gets type descriptor for keys in <map> */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Multimap_Type Multimap_make (in TypeDescriptor_Type keyTypeDescriptor);
  /** constructs a single multimap with keys conforming to
      <keyTypeDescriptor> */


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Multimap_destroy (out Multimap_Type *map);
  /** destroys all elements in <map> and map itself */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

List_Type Multimap_lookup (in Multimap_Type map, in Object key);
  /** searches <map> for element with identification <key> and returns
      associated value list or NULL if none exists */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Multimap_clear (inout Multimap_Type *map);
  /** removes all elements in <map> */

/*--------------------*/

void Multimap_add (inout Multimap_Type *map, in Object key, in Object value);
  /** adds <value> to <key> in <map> */

/*--------------------*/

void Multimap_deleteKey (inout Multimap_Type *map, in Object key);
  /** removes <key> and all its entries from <map>; when <key> is not
      in <map>, nothing happens */

/*--------------------*/

void Multimap_deleteValue (inout Multimap_Type *m,
			   in Object key, in Object value);
  /* removes associated <value> for <key> in <map>; when <value> is
     not associated with <key>, nothing happens */

#endif /* __MULTIMAP_H */
