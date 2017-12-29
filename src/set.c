/** Set module --
    Implementation of module providing services for handling sets
    represented as integers.

    Original version by Thomas Tensi, 2004-09
*/

#include "set.h"

/*========================================*/

#include "globdefs.h"

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Set_initialize (void)
{
}

/*--------------------*/

void Set_finalize (void)
{
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Set_Type Set_make (in Set_Element element)
{
  return ( 1 << (Set_Type)element );
}

/*--------------------*/

void Set_clear (out Set_Type *set)
{
  *set = (Set_Type) 0U;
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

Set_Element Set_firstElement (in Set_Type set)
{
   Set_Element i;
   Set_Type mask;
   Boolean found;

   mask = 1U;
   found = ((mask & set) != 0);

   for (i = 0;  i != 32 && !found;  i++) {
     mask += mask;
     found = ((mask & set) != 0);
   }

   return i;
}

/*--------------------*/

Boolean Set_isElement (in Set_Type set, in Set_Element element)
{
  return (Boolean)((Set_make(element) & set) != 0);
}


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

Boolean Set_isEmpty (in Set_Type set)
{
  return (set == 0);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

Set_Type Set_complement (in Set_Type set)
{
  return (Set_Type)(~set);
}

/*--------------------*/

void Set_include (inout Set_Type *set, in Set_Element element)
{
  *set |= ((Set_Type)1 << element);
}

/*--------------------*/

void Set_exclude (inout Set_Type *set, in Set_Element element)
{
  *set &= ~((Set_Type)1 << element);
}
