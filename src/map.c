/** Map module --
    Implementation of module providing all services for generic maps.

    This implementation uses an open hash table which should be okay
    for quick access and change of map elements.

    Original version by Thomas Tensi, 2008-02
*/

#include "map.h"

/*========================================*/

#include "globdefs.h"
#include "list.h"
#include "typedescriptor.h"

/*========================================*/

#define Map__magicNumber 0x18432567
#define Map__entryMagicNumber 0x43218765


#define Map__bucketCount 64
  /* number of hash buckets within a map */


typedef struct Map__Record {
  long magicNumber;
  TypeDescriptor_Type keyTypeDescriptor;
  List_Type bucket[Map__bucketCount];
} Map__Record;
  /** record type representing a hash table with <Map__bucketCount>
      buckets */


typedef struct {
  long magicNumber;
  Map_Type map;
  Object key;
  Object value;
} Map__EntryRecord;
  /** the entry within a map containing a key and an object as
      value */


typedef Map__EntryRecord *Map__Entry;

/*--------------------*/

/* type descriptors for lists */

static void Map__destroyEntry (inout Object *object);
static Object Map__makeEntry (void);
static Boolean Map__entryHasKey (in Object object, in Object key);

static TypeDescriptor_Record Map__entryRecordTDRecord =
  { /* .objectSize = */ sizeof(Map__EntryRecord), 
    /* .assignmentProc = */ NULL, /* .comparisonProc = */ NULL,
    /* .constructionProc = */ Map__makeEntry,
    /* .destructionProc = */ Map__destroyEntry,
    /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Map__entryHasKey };

static TypeDescriptor_Type Map__entryRecordTypeDescriptor =
  &Map__entryRecordTDRecord;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Map__Entry Map__attemptConversionToEntry (in Object entry)
{
  return attemptConversion("Map__Entry", entry, 
			   Map__entryMagicNumber);
}

/*--------------------*/

static Boolean Map__checkValidityPRE (in Object map, in char *procName)
  /** checks as a precondition of routine <procName> whether <map> is
      a valid map and returns the check result */
{
  return PRE(Map_isValid(map), procName, "invalid map");
}

/*--------------------*/
/*--------------------*/

static void Map__destroyEntry (inout Object *object)
  /* deallocates the entry of a map given by <object> */
{
  Boolean precondition = PRE(isValidObject(*object, Map__entryMagicNumber),
			     "Map__destroyEntry", "invalid map entry");

  if (precondition) {
    *object = NULL;
  }
}

/*--------------------*/

static Boolean Map__entryHasKey (in Object object, in Object key)
  /* checks whether the entry of a map given as <object> has
     <key> as its key */
{
  Map__Entry entry = Map__attemptConversionToEntry(object);
  return (TypeDescriptor_compareObjects(entry->map->keyTypeDescriptor,
					key, entry->key));
}

/*--------------------*/

static SizeType Map__hashValue (in Map_Type map, in Object key)
  /* calculates a hash value for <key> in map in the range of 0 to
     <Map__bucketCount>-1*/
{
  SizeType result;

  result = TypeDescriptor_objectHashCode(map->keyTypeDescriptor, key);
  return (result % Map__bucketCount);
}

/*--------------------*/

static Object Map__makeEntry (void)
  /* creates new entry for a map and returns it as object */
{
  Map__Entry entry = NEW(Map__EntryRecord);

  entry->magicNumber = Map__entryMagicNumber;
  entry->key         = NULL;
  entry->value       = NULL;

  return entry;
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Map_initialize (void)
{
}

/*--------------------*/

void Map_finalize (void)
{
}

/*--------------------*/
/* TYPE CHECKING      */
/*--------------------*/

Boolean Map_isValid (in Object map)
{
  return isValidObject(map, Map__magicNumber);
}

/*--------------------*/

TypeDescriptor_Type Map_getKeyType (in Map_Type map)
{
  char *procName = "Map_getKeyType";
  Boolean precondition = Map__checkValidityPRE(map, procName);
  TypeDescriptor_Type result = NULL;

  if (precondition) {
    result = map->keyTypeDescriptor;
  }

  return result;
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Map_Type Map_make (in TypeDescriptor_Type keyTypeDescriptor)
{
  int i;
  Map_Type map = NEW(Map__Record);

  map->magicNumber       = Map__magicNumber;
  map->keyTypeDescriptor = keyTypeDescriptor;

  for (i = 0;  i < Map__bucketCount;  i++) {
    map->bucket[i] = List_make(Map__entryRecordTypeDescriptor);
  }

  return map;
}

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Map_destroy (inout Map_Type *map)
{
  char *procName = "Map_destroy";
  Map_Type currentMap = *map;
  Boolean precondition = Map__checkValidityPRE(currentMap, procName);

  if (precondition) {
    int i;

    for (i = 0;  i < Map__bucketCount;  i++) {
      List_destroy(&(currentMap->bucket[i]));
    }

    DESTROY(*map);
    *map = NULL;
  }
}

/*--------------------*/
/* ACCESS             */
/*--------------------*/

Object Map_lookup (in Map_Type map, in Object key)
{
  char *procName = "Map_lookup";
  Boolean precondition = Map__checkValidityPRE(map, procName);
  Object result = NULL;

  if (precondition) {
    SizeType hashValue = Map__hashValue(map, key);
    Object object = List_lookup(map->bucket[hashValue], key);

    if (object != NULL) {
      Map__Entry entry = Map__attemptConversionToEntry(object);
      result = entry->value;
    }
  }

  return result;
}

/*--------------------*/

void Map_getKeyList (in Map_Type map, inout List_Type *keyList)
{
  char *procName = "Map_getKeyList";
  Boolean precondition = Map__checkValidityPRE(map, procName);

  if (precondition) {
    int i;

    List_clear(keyList);

    for (i = 0;  i < Map__bucketCount;  i++) {
      List_Type bucket = map->bucket[i];
      List_Cursor cursor = List_resetCursor(bucket);

      while (cursor != NULL) {
	Map__Entry entry = List_getElementAtCursor(cursor);
	Object *newObjectPtr = List_append(keyList);
	TypeDescriptor_assignObject(map->keyTypeDescriptor, newObjectPtr,
				    entry->key);
	List_advanceCursor(&cursor);
      }
    }
  }
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Map_clear (inout Map_Type *map)
{
  char *procName = "Map_clear";
  Map_Type currentMap = *map;
  Boolean precondition = Map__checkValidityPRE(currentMap, procName);

  if (precondition) {
    int i;

    for (i = 0;  i < Map__bucketCount;  i++) {
      List_Type bucket = currentMap->bucket[i];
      List_clear(&bucket);
    }
  }
}

/*--------------------*/

void Map_set (inout Map_Type *map, in Object key, in Object value)
{
  char *procName = "Map_set";
  Map_Type currentMap = *map;
  Boolean precondition = Map__checkValidityPRE(currentMap, procName);

  if (precondition) {
    SizeType hashValue = Map__hashValue(currentMap, key);
    List_Type bucket = currentMap->bucket[hashValue];
    Object object = List_lookup(bucket, key);
    Boolean keyIsNew = (object == NULL);
    Map__Entry entry;
 
    if (keyIsNew) {
      Object *entryPtr = List_append(&bucket);
      object = *entryPtr;
    }

    entry = Map__attemptConversionToEntry(object);

    if (keyIsNew) {
      entry->map = currentMap;
      entry->key = TypeDescriptor_makeObject(currentMap->keyTypeDescriptor);
      TypeDescriptor_assignObject(currentMap->keyTypeDescriptor,
				  &(entry->key), key);
    }
    
    entry->value = value;
  }
}

/*--------------------*/

void Map_deleteKey (inout Map_Type *map, in Object key)
{
  char *procName = "Map_deleteKey";
  Map_Type currentMap = *map;
  Boolean precondition = Map__checkValidityPRE(currentMap, procName);

  if (precondition) {
    SizeType hashValue = Map__hashValue(currentMap, key);
    List_Type bucket = currentMap->bucket[hashValue];
    List_Cursor cursor = List_setCursorToElement(bucket, key);

    if (cursor != NULL) {
      Object object = List_getElementAtCursor(cursor);
      Map__Entry entry = Map__attemptConversionToEntry(object);
      TypeDescriptor_destroyObject(currentMap->keyTypeDescriptor,
				   &(entry->key));
      List_deleteElementAtCursor(cursor);
    }
  }
}
