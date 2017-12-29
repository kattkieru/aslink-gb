/** Set module --
    This module provides all services for handling sets represented as
    long integers (bitsets).  Those sets can only carry integer
    elements in the range of 0 to 31 (or enumeration types).

    The standard routines are available to make a set empty (<clear>),
    to check whether it is empty (<isEmpty>), to find some element in
    the set (<firstElement>), to make a singleton set from one element
    (<make>), to find the complement set (<complement>), to test
    set membership (<isElement>) and to in-/exclude an element
    (<include>, <exclude>).

    Original version by Thomas Tensi, 2004-09
*/

#ifndef __SET_H
#define __SET_H

/*========================================*/

#include "globdefs.h"

/*========================================*/

typedef long Set_Type;
  /** a set may contain the elements 0..31 */

typedef char Set_Element;
  /** the base type of the set */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Set_initialize (void);
  /** sets up internal data structures */

/*--------------------*/

void Set_finalize (void);
  /** cleans up internal data structures */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Set_Type Set_make (in Set_Element element);
  /** makes a singleton set from <element> */

/*--------------------*/

void Set_clear (out Set_Type *set);
  /** makes <set> empty */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

Set_Element Set_firstElement (in Set_Type set);
  /** finds out first element in <set> when not empty */

/*--------------------*/

Boolean Set_isElement (in Set_Type set, in Set_Element element);
  /** tells whether <element> occurs in <set> or not */


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

Boolean Set_isEmpty (in Set_Type set);
  /** finds out whether <set> is empty */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

Set_Type Set_complement (in Set_Type set);
  /** returns the complement set of <set> */

/*--------------------*/

void Set_include (inout Set_Type *set, in Set_Element element);
  /** adds <element> to <set> */

/*--------------------*/

void Set_exclude (inout Set_Type *set, in Set_Element element);
  /** removes <element> from <set> */

#endif /* __SET_H */
