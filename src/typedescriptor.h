/** TypeDescriptor module --
    This module encapsulates services for generic handling of objects
    within some container structures.  Containers within the linker
    always use pointers to elements.  The elementary operations for
    those element types must be defined and are referenced by a type
    descriptor.

    Those elementary operations are: construction of some object,
    destruction of an object, assignment of one object to another,
    comparison of two objects for equality, calculating some hash code
    for an object and checking whether some object has a specific key.
    When such a routine is not defined for some type, a reasonable
    default (like e.g. a pointer comparison) is used.

    Original version by Thomas Tensi, 2008-01
*/

#ifndef __TYPEDESCRIPTOR_H
#define __TYPEDESCRIPTOR_H

/*========================================*/

#include "globdefs.h"

/*========================================*/

typedef void (*TypeDescriptor_AssignmentProc)(in Object *destination,
					      in Object source);
  /** generic routine type describing the assignment of an object to
      another */


typedef Boolean (*TypeDescriptor_ComparisonProc)(in Object objectA,
						 in Object objectB);
  /** generic routine type describing the equality check of two
      objects */


typedef Object (*TypeDescriptor_ConstructionProc)(void);
  /** generic routine type describing the creation of an object */


typedef void (*TypeDescriptor_DestructionProc)(inout Object *object);
  /** generic routine type describing the destrucion of an object */


typedef SizeType (*TypeDescriptor_HashCodeProc)(in Object object);
  /** generic routine type describing the hash code calculation for an
      object */


typedef Boolean (*TypeDescriptor_KeyValidationProc)(in Object object,
						    in Object key);
  /** generic routine type describing the check of some object against
      some key */


typedef struct {
  SizeType objectSize;
  TypeDescriptor_AssignmentProc assignmentProc;
  TypeDescriptor_ComparisonProc comparisonProc;
  TypeDescriptor_ConstructionProc constructionProc;
  TypeDescriptor_DestructionProc destructionProc;
  TypeDescriptor_HashCodeProc hashCodeProc;
  TypeDescriptor_KeyValidationProc keyValidationProc;
} TypeDescriptor_Record;
  /** type defining the central characteristics of some type: its
      size, routines for assignment, construction, destruction,
      comparison and key validation; when a routine is NULL, the
      corresponding bitwise operations are used as a default (e.g. a
      memmove for assignment) */


typedef TypeDescriptor_Record *TypeDescriptor_Type;

/*--------------------*/

extern TypeDescriptor_Type TypeDescriptor_default;
  /** default type descriptor using standard bitwise operations; this
      descriptor can e.g. be used for containers of plain pointers
      which have no constructors, destructors or specific assignment
      operations  */


extern TypeDescriptor_Type TypeDescriptor_plainDataTypeDescriptor;
  /** type descriptor for plain data types (like integers) using
      standard bitwise operations and a hash code which interpretes
      the value of the data type */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void TypeDescriptor_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void TypeDescriptor_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void TypeDescriptor_assignObject (in TypeDescriptor_Type typeDescriptor,
				  inout Object *destination,
				  in Object source);
  /** assigns <source> to <destination> with the assignment routine
      defined in <typeDescriptor> */

/*--------------------*/

Boolean TypeDescriptor_checkObjectForKey (
				 in TypeDescriptor_Type typeDescriptor,
				 in Object object, in Object key);
  /** checks whether <object> has <key> with the key checking routine
      defined in <typeDescriptor> */

/*--------------------*/

Boolean TypeDescriptor_compareObjects (
				 in TypeDescriptor_Type typeDescriptor,
				 in Object objectA, in Object objectB);
  /** checks whether <objectA> and <objectB> are equal with the
      equality checking routine defined in <typeDescriptor> */

/*--------------------*/

void TypeDescriptor_destroyObject (in TypeDescriptor_Type typeDescriptor,
				   inout Object *object);
  /** destroys existing <object> with the destruction routine defined
      in <typeDescriptor> and nullifies pointer */

/*--------------------*/

Object TypeDescriptor_makeObject (in TypeDescriptor_Type typeDescriptor);
  /** makes new object with the creation routine defined in
      <typeDescriptor> */

/*--------------------*/

SizeType TypeDescriptor_objectHashCode (in TypeDescriptor_Type typeDescriptor,
					in Object object);
  /** returns hash code for <object> with the hash code calculation
      routine defined in <typeDescriptor> */


#endif /* __TYPEDESCRIPTOR_H */
