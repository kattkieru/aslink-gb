/** IntegerMap module --
    This module provides all services for maps from objects to long
    integers.  It redefines only the <set> and <lookup> routine; the
    other <Map> routines may be used as is.

    This wrapper module is necessary because the map module uses
    pointers as map values and returns <NULL> as a failure indicator.
    When an integer 0 occurs as a map value, it hence cannot be
    distinguished from a <NULL> pointer by the <Map> module and cannot
    be used.  <IntegerMap> does some internal bookkeeping and hides
    this complication from its clients which can safely use 0 as a
    value in an integer map.  Instead of <NULL> it returns <notFound>
    when a lookup fails.

    Original version by Thomas Tensi, 2008-07
*/

#ifndef __INTEGERMAP_H
#define __INTEGERMAP_H

/*========================================*/

#include "globdefs.h"
#include "map.h"

/*========================================*/

typedef Map_Type IntegerMap_Type;
  /** redefined integer map type based on generic map type */


#define IntegerMap_notFound SizeType_max
  /** value returned when lookup fails */

/*========================================*/

/*--------------------*/
/* ACCESS             */
/*--------------------*/

long IntegerMap_lookup (in IntegerMap_Type map, in Object key);
  /** searches <map> for element with identification <key> and returns
      associated value or <notFound> if none exists */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void IntegerMap_set (inout IntegerMap_Type *map, in Object key, in long value);
  /** sets <value> for <key> in <map> */

#endif /* __INTEGERMAP_H */
