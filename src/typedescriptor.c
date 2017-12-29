/** TypeDescriptor module --
    Implementation of module providing all services for generic
    handling of objects within some container structures.

    Original version by Thomas Tensi, 2008-02
*/

#include "typedescriptor.h"

/*========================================*/

#include "globdefs.h"
#include "error.h"

#include <string.h>
# define STRING_memcmp memcmp
# define STRING_memcpy memcpy

#include <stdlib.h>
# define Stdlib_malloc malloc

/*========================================*/

static SizeType TypeDescriptor__directHashProc (in Object object);

static TypeDescriptor_Record TypeDescriptor_dtDescriptorRecord = 
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ NULL,
    /* .destructionProc = */ NULL,
    /* .hashCodeProc = */ TypeDescriptor__directHashProc,
    /* .keyValidationProc = */ NULL };


TypeDescriptor_Type TypeDescriptor_plainDataTypeDescriptor =
  &TypeDescriptor_dtDescriptorRecord;


TypeDescriptor_Type TypeDescriptor_default = NULL;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static SizeType TypeDescriptor__directHashProc (in Object object)
  /** hash procedure for plain data types taking the object value
      itself as the hash code */
{
  return (SizeType) object;
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void TypeDescriptor_initialize (void)
{
}

/*--------------------*/

void TypeDescriptor_finalize (void)
{
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void TypeDescriptor_assignObject (in TypeDescriptor_Type typeDescriptor,
				  inout Object *destination,
				  in Object source)
{
  TypeDescriptor_AssignmentProc assignmentProc = NULL;
  SizeType objectSize = 0;

  if (typeDescriptor != TypeDescriptor_default) {
    assignmentProc = typeDescriptor->assignmentProc;
    objectSize     = typeDescriptor->objectSize;
  }
  
  if (assignmentProc != NULL) {
    assignmentProc(destination, source);
  } else if (objectSize > 0) {
    STRING_memcpy(*destination, source, objectSize);
  } else {
    *destination = source;
  }
}

/*--------------------*/

Boolean TypeDescriptor_checkObjectForKey (
			 in TypeDescriptor_Type typeDescriptor,
			 in Object object, in Object key)
{
  TypeDescriptor_KeyValidationProc keyValidationProc = NULL;
  SizeType objectSize = 0;
  Boolean result;

  if (typeDescriptor != TypeDescriptor_default) {
    keyValidationProc = typeDescriptor->keyValidationProc;
    objectSize        = typeDescriptor->objectSize;
  }
  
  if (keyValidationProc != NULL) {
    result = keyValidationProc(object, key);
  } else if (objectSize > 0) {
    result = STRING_memcmp(object, key, objectSize);
  } else {
    result = (object == key);
  }

  return result;
}

/*--------------------*/

Boolean TypeDescriptor_compareObjects ( in TypeDescriptor_Type typeDescriptor,
					in Object objectA, in Object objectB)
{
  TypeDescriptor_ComparisonProc comparisonProc = NULL;
  SizeType objectSize = 0;
  Boolean result;

  if (typeDescriptor != TypeDescriptor_default) {
    comparisonProc = typeDescriptor->comparisonProc;
    objectSize     = typeDescriptor->objectSize;
  }
  
  if (comparisonProc != NULL) {
    result = comparisonProc(objectA, objectB);
  } else if (objectSize > 0) {
    result = STRING_memcmp(objectA, objectB, objectSize);
  } else {
    result = (objectA == objectB);
  }

  return result;
}

/*--------------------*/

void TypeDescriptor_destroyObject (in TypeDescriptor_Type typeDescriptor,
				   inout Object *object)
{
  TypeDescriptor_DestructionProc destructionProc = NULL;
  SizeType objectSize = 0;

  if (typeDescriptor != TypeDescriptor_default) {
    destructionProc = typeDescriptor->destructionProc;
    objectSize      = typeDescriptor->objectSize;
  }
  
  if (destructionProc != NULL) {
    destructionProc(object);
  } else if (objectSize > 0) {
    DESTROY(*object);
  } else {
    *object = NULL;
  }
}

/*--------------------*/

Object TypeDescriptor_makeObject (in TypeDescriptor_Type typeDescriptor)
{
  TypeDescriptor_ConstructionProc constructionProc = NULL;
  Object object;
  SizeType objectSize = 0;

  if (typeDescriptor != TypeDescriptor_default) {
    constructionProc = typeDescriptor->constructionProc;
    objectSize       = typeDescriptor->objectSize;
  }
  
  if (constructionProc != NULL) {
    object = constructionProc();
  } else if (objectSize > 0) {
    object = Stdlib_malloc(objectSize);
  } else {
    object = NULL;
  }

  return object;
}

/*--------------------*/

SizeType TypeDescriptor_objectHashCode (in TypeDescriptor_Type typeDescriptor,
					in Object object)
{
  TypeDescriptor_HashCodeProc hashCodeProc = NULL;
  SizeType objectSize = sizeof(Object);
  SizeType result = 0;

  if (typeDescriptor != TypeDescriptor_default) {
    hashCodeProc = typeDescriptor->hashCodeProc;
    objectSize   = typeDescriptor->objectSize;
  }
  
  if (hashCodeProc != NULL) {
    result = hashCodeProc(object);
  } else {
    SizeType i;
    char *array = object;

    for (i = 0;  i < objectSize;  i++) {
      result += (result + array[i]);
    }
  }

  return result;
}
