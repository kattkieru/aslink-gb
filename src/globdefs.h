/** GlobDefs module --
    This module provides elementary types and routines for the generic
    ASXXX linker.

    The types defined are several integer types, boolean and a generic
    object type.  Additionally the module provides routines for some
    rudimentary form of assertion checking.

    Because this module is considered to be globally known, its
    identifiers are universal and are not prefixed by the module name
    (contrary to the identifier prefix convention).

    Original version by Thomas Tensi, 2006-07
*/

#ifndef __GLOBDEFS_H
#define __GLOBDEFS_H

/*========================================*/

#include <stddef.h>
#include <stdlib.h>

/*========================================*/

/*--------------*/
/* scalar types */
/*--------------*/

typedef unsigned char UINT8;
typedef signed char INT8;
typedef unsigned int UINT16;
typedef signed int INT16;
typedef unsigned long UINT32;
typedef signed long INT32;

typedef size_t SizeType;
#define SizeType_max 2147483647

typedef int Boolean;
#define false 0
#define true 1

typedef void *Object;
  /** placeholder type (representing all kinds of object types) */


/*----------------------------*/
/* tags for formal parameters */
/*----------------------------*/

#define in
#define inout
#define out
  /** formal parameter modes (purely for documentation) */

/*----------------------------*/
/* utility routines           */
/*----------------------------*/

#define NEW(elementType) malloc(sizeof(elementType))
  /** allocation routine for an object of some element type */

/*--------------------*/

#define NEWARRAY(elementType, count) calloc((count), sizeof(elementType))
  /** allocation routine for an array of elements */

/*--------------------*/

#define DESTROY(pointer)  free(pointer)
  /** deallocation routine for a pointer */

/*--------------------*/

Boolean PRE (in Boolean condition, in char *procName, in char *message);
  /** checks precondition <condition>; if false, <message> referring
      to <procName> is put out and program is terminated */

/*--------------------*/

void ASSERTION (in Boolean condition, in char *procName, in char *message);
  /** checks internal assertion <condition>; if false, <message>
      referring to <procName> is put out and program is terminated */

/*--------------------*/

Object attemptConversion (in char *opaqueTypeName, in Object object,
			  in long magicNumber);
  /** generic routine for verifying that <object> is a pointer to an
      internal type by checking the long integer pointed to; if it is
      not identical to <magicNumber>, the program stops with an error
      message  */

/*--------------------*/

Boolean isValidObject (in Object object, in long magicNumber);
  /** generic routine for verifying that <object> is a pointer to an
      internal type by checking the long integer pointed to; if it is
      not identical to <magicNumber>, the routine returns false */

#endif /* __GLOBDEFS_H */
