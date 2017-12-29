/** IntegerMap module --
    Implementation of module providing all services for maps from
    objects to long integers.  It redefines only the <set> and
    <lookup> routine; the other Map routines may be used as is.

    Note that the integer value 0 is mapped by the set routine to some
    other value and remapped upon lookup.  This is necessary to
    distinguish it from the NULL pointer used for telling that a
    lookup has failed.

    Original version by Thomas Tensi, 2008-07
*/

#include "integermap.h"

/*========================================*/

#include "globdefs.h"
#include "map.h"

/*========================================*/

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* ACCESS             */
/*--------------------*/

long IntegerMap_lookup (in IntegerMap_Type map, in Object key)
{
  Object resultAsObject = Map_lookup(map, key);
  long result;

  if (resultAsObject == NULL) {
    result = IntegerMap_notFound;
  } else {
    result = (long) resultAsObject;

    /* remap the representation for 0 value */
    if (result == SizeType_max) {
      result = 0;
    }
  }

  return result;
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void IntegerMap_set (inout IntegerMap_Type *map, in Object key, in long value)
{
  char *procName = "IntegerMap_set";
  Boolean precondition = PRE(value != IntegerMap_notFound, procName,
			     "cannot use given value in integer map");

  if (precondition) {
    Object valueAsObject;

    /* map value 0 to SizeType_max to distinguish it from NULL failure
       indication of <Map_lookup> */
    if (value == 0) {
      value = SizeType_max;
    }

    valueAsObject = (Object) value;
    Map_set(map, key, valueAsObject);
  }
}
