/** Area module --
    Implementation of module providing all services for area definitions
    in the generic SDCC linker

    Original version by Thomas Tensi, 2006-08
    based on the module lkarea.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "area.h"

#include "error.h"
#include "globdefs.h"
#include "list.h"
#include "module.h"
#include "set.h"
#include "string.h"
#include "symbol.h"
#include "target.h"

/*========================================*/

#define Area__segmentMagicNumber 0x11112222
#define Area__magicNumber        0x22221111

typedef List_Type Area_SegmentList;
  /** list of Area_SegmentRecord */


typedef struct Area__Record {
  long magicNumber;
  String_Type name;
  Area_AttributeSet attributes;
  Target_Address startAddress;
  Target_Address totalSize;
  Area_SegmentList segmentList;
} Area__Record;
  /** type representing each 'unique' data or code area definition
      found in the linker files; it consists of the name of the area,
      a set of attributes (REL/CON/OVR/ABS), the area base address and
      total size */


typedef struct Area__SegmentRecord {
  long magicNumber;
  Area_Type parentArea;
  Module_Type parentModule;
  Target_Address startAddress;
  Target_Address totalSize;
  Symbol_List symbolList;
} Area__SegmentRecord;
  /** type representing a segment of an area which is defined by every
      "A" directive in the linker files; there are back references to
      the respective code or data area and the header where this
      section belongs to; also a list of all symbols in this segment
      is stored */

static List_Type Area__list;
  /** list containing all area definitions */

static String_Type Area__absoluteAreaName;
  /** name of predefined absolute area */

static Area_Type Area__absoluteArea;
  /** the predefined absolute area */

static Area_Segment Area__currentSegment;
  /** the currently processed segment */

/*--------------------*/

static void Area__destroy (inout Object *object);
static Boolean Area__hasKey (in Object listElement, in Object key);
static Object Area__makeSegment (void);
static Boolean Area__segmentHasKey (in Object listElement, in Object key);


static TypeDescriptor_Record Area__tdRecord =
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, 
    /* .constructionProc = */ NULL, /* .destructionProc = */ NULL,
    /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Area__hasKey };

TypeDescriptor_Type Area_typeDescriptor = &Area__tdRecord;


static TypeDescriptor_Record Area__segmentTDRecord =
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, 
    /* .constructionProc = */ NULL, /* .destructionProc = */ NULL,
    /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Area__segmentHasKey };

TypeDescriptor_Type Area_segmentTypeDescriptor = &Area__segmentTDRecord;


static TypeDescriptor_Record Area__segmentRecordTDRecord =
  { /* .objectSize = */ sizeof(Area__SegmentRecord),
    /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ Area__makeSegment,
    /* .destructionProc = */ NULL, /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Area__segmentHasKey };

static TypeDescriptor_Type Area__segmentRecordTypeDescriptor = 
  &Area__segmentRecordTDRecord;
  /** variable used for describing the type properties when segment
      records occur in generic types like lists */


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Area_Type Area__attemptConversion (in Object area)
  /** verifies that <area> is really a pointer to an area descriptor;
      if not, the program stops with an error message */
{
  return attemptConversion("Area_Type", area, Area__magicNumber);
}

/*--------------------*/

static Area_Segment Area__attemptConversionToSegment (in Object segment)
  /** verifies that <segment> is really a pointer to a segment;
      if not, the program stops with an error message */
{
  return attemptConversion("Area_Segment", segment, Area__segmentMagicNumber);
}

/*--------------------*/

static Boolean Area__isValid (in Object area)
{
  return isValidObject(area, Area__magicNumber);
}

/*--------------------*/

static Boolean Area__isValidSegment (in Object segment)
{
  return isValidObject(segment, Area__segmentMagicNumber);
}

/*--------------------*/

static Boolean Area__checkSegmentValidityPRE (in Object segment,
					      in char *procName)
  /** checks as a precondition of routine <procName> whether <segment> is
      a valid segment and returns the check result */
{
  return PRE(Area__isValidSegment(segment), procName, "invalid area segment");
}

/*--------------------*/

static Boolean Area__checkValidityPRE (in Object area,
				       in char *procName)
  /** checks as a precondition of routine <procName> whether <area> is
      a valid area and returns the check result */
{
  return PRE(Area__isValid(area), procName, "invalid area object");
}

/*--------------------*/
/*--------------------*/

static void Area__destroy (inout Area_Type *area)
{
  char *procName = "Area__destroy";
  Area_Type currentArea = *area;
  Boolean precondition = Area__checkValidityPRE(currentArea, procName);

  if (precondition) {
    String_destroy(&currentArea->name);
    List_destroy(&currentArea->segmentList);
    DESTROY(currentArea);
    *area = NULL;
  }
}

/*--------------------*/

static Boolean Area__hasKey (in Object object, in Object key)
  /** checks whether <object> has <key> as identification */
{
  Area_Type area = Area__attemptConversion(object);
  String_Type name = key;
  return String_isEqual(area->name, name);
}

/*--------------------*/

static Area_Type Area__init (in String_Type areaName,
			     in Area_AttributeSet attributeSet)
  /** creates and links in a new area with <areaName> and attributes
      <attributeSet> */
{
  Object *objectPtr = List_append(&Area__list);
  Area_Type area = NEW(Area__Record);
  *objectPtr = area;

  area->magicNumber  = Area__magicNumber;
  area->name         = String_make();
  area->segmentList  = List_make(Area_segmentTypeDescriptor);
  area->startAddress = 0;
  Set_clear(&area->attributes);

  String_copy(&area->name, areaName);
  area->attributes   = attributeSet;
  return area;
}

/*--------------------*/

static Object Area__makeSegment (void)
  /** private construction of segment used when a new entry is
      created in segment record list */
{
  Module_Type currentModule = Module_currentModule();
  Area_Segment segment = NULL;

  if (currentModule == NULL) {
    Error_raise(Error_Criticality_fatalError, "No module header defined");
  } else {
    segment = NEW(Area__SegmentRecord);

    segment->magicNumber  = Area__segmentMagicNumber;
    segment->parentArea   = NULL;
    segment->parentModule = currentModule;
    segment->startAddress = 0;
    segment->totalSize    = 0;
    segment->symbolList   = List_make(Symbol_typeDescriptor);
  }

  return segment;
}

/*--------------------*/

static void Area__linkSegments (inout Area_Type area)
 /** resolves the segment addresses for <area> and reports any paging
     boundary and length errors */
{
  /* TODO: check lkarea::lnksect for revised allocation strategy */

  Target_Address address = area->startAddress;
  Target_Address size = 0;
  Boolean hasOverlayedSegments = Set_isElement(area->attributes,
                                    Area_Attribute_hasOverlayedSegments);
  Boolean hasPagedSegments = Set_isElement(area->attributes,
                                           Area_Attribute_hasPagedSegments);
  List_Cursor segmentCursor;

  if (hasPagedSegments && ((address & 0xFF) != 0)) {
    Error_raise(Error_Criticality_warning, "Paged Area %s Boundary Error",
		area->name);
  }

  for (segmentCursor = List_resetCursor(area->segmentList);
       segmentCursor != NULL;
       List_advanceCursor(&segmentCursor)) {
    Object object = List_getElementAtCursor(segmentCursor);
    Area_Segment segment = Area__attemptConversionToSegment(object);

    segment->startAddress = address;

    if (!hasOverlayedSegments) {
      /* concatenated segments */
      address += segment->totalSize;
      size += segment->totalSize;
    } else if (segment->totalSize > size) {
      size = segment->totalSize;
    }
  }

  area->totalSize = size;

  if (hasPagedSegments && size > 256) {
    Error_raise(Error_Criticality_warning, "Paged Area %s Length Error",
		area->name);
  }
}

/*--------------------*/

static Boolean Area__segmentHasKey (in Object object, in Object key)
  /** checks whether <object> has <key> as identification */
{
  Area_Segment segment = Area__attemptConversionToSegment(object);
  String_Type otherSegmentName = key;
  String_Type segmentName = segment->parentArea->name;

  return String_isEqual(segmentName, otherSegmentName);
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Area_initialize (void)
{
  Area_AttributeSet attributes;

  /* construct the list of all known areas */
  Area__list = List_make(Area_typeDescriptor);

  /* there is a special area called "ABS" */
  Set_clear(&attributes);
  Set_include(&attributes, Area_Attribute_isAbsolute);
  Set_include(&attributes, Area_Attribute_hasOverlayedSegments);
  Area__absoluteAreaName = String_makeFromCharArray(".ABS.");
  Area__absoluteArea = Area__init(Area__absoluteAreaName, attributes);
}

/*--------------------*/

void Area_finalize (void)
{
  String_destroy(&Area__absoluteAreaName);
  List_destroy(&Area__list);
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Area_Type Area_make (in String_Type areaName, 
		     in Area_AttributeSet attributeSet)
{
  Area_Type area = List_lookup(Area__list, areaName);

  if (area == NULL) {
    /* make a new area */
    area = Area__init(areaName, attributeSet);
  } else {
    /* check whether this is really an area */
    area = Area__attemptConversion(area);
    /* existing area: check whether attributes match */
    if (attributeSet != area->attributes) {
      Error_raise(Error_Criticality_warning,
		  "Conflicting flags in area %s\n", areaName);
    }
  }

  return area;
}

/*--------------------*/

void Area_makeSegment (in String_Type areaName,
		       in Target_Address totalSize,
                       in Area_AttributeSet attributeSet)
{
  Area_Type area = Area_make(areaName, attributeSet);
  Area_Segment segment = Area__makeSegment();
  Object *objectPtr = List_append(&area->segmentList);
  *objectPtr = segment;

  segment->parentArea = area;
  segment->totalSize  = totalSize;
  
  Module_addSegment(&segment->parentModule, segment);
  Area__currentSegment = segment;
}

/*--------------------*/

void Area_makeAbsoluteSegment (void)
{
  Area_AttributeSet attributeSet = Area__absoluteArea->attributes;
  Area_makeSegment(Area__absoluteAreaName, 0, attributeSet);
}

/*--------------------*/

Area_AttributeSet Area_makeAttributeSet (in UINT8 attributeSetEncoding)
{
  Area_AttributeSet result;

  Set_clear(&result);

  if ((attributeSetEncoding & 004) != 0) {
    Set_include(&result, Area_Attribute_hasOverlayedSegments);
  }

  if ((attributeSetEncoding & 010) != 0) {
    Set_include(&result, Area_Attribute_isAbsolute);
  }

  if ((attributeSetEncoding & 020) != 0) {
    Set_include(&result, Area_Attribute_hasPagedSegments);
  }

  return result;
}


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Area_destroy (inout Area_Type *area)
{
  char *procName = "Area_destroy";
  Area_Type currentArea = *area;
  Boolean precondition = Area__checkValidityPRE(currentArea, procName);

  if (precondition) {
    /* remove it from area list */
    List_Cursor areaCursor = List_setCursorToElement(Area__list,
						     currentArea->name);
    List_deleteElementAtCursor(areaCursor);
  }
}


/*--------------------*/
/* STATUS REPORT      */
/*--------------------*/

Area_Segment Area_currentSegment (void)
{
  return Area__currentSegment;
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Area_getList (inout Area_List *areaList)
{
  List_copy(areaList, Area__list);
}

/*--------------------*/

void Area_getName (in Area_Type area, out String_Type *name)
{
  char *procName = "Area_getName";
  Boolean precondition = Area__checkValidityPRE(area, procName);

  if (precondition) {
    String_copy(name, area->name);
  }
}

/*--------------------*/

Area_AttributeSet Area_getAttributes (in Area_Type area)
{
  char *procName = "Area_getAttributes";
  Boolean precondition = Area__checkValidityPRE(area, procName);
  Area_AttributeSet result = 0;

  if (precondition) {
    result = area->attributes;
  }

  return result;
}

/*--------------------*/

Target_Address Area_getAddress (in Area_Type area)
{
  char *procName = "Area_getAddress";
  Boolean precondition = Area__checkValidityPRE(area, procName);
  Target_Address result = 0;

  if (precondition) {
    result = area->startAddress;
  }

  return result;
}

/*--------------------*/

void Area_getListOfSegments (in Area_Type area, 
			     out Area_SegmentList *segmentList)
{
  char *procName = "Area_getListOfSegments";
  Boolean precondition = Area__checkValidityPRE(area, procName);

  if (precondition) {
    List_copy(segmentList, area->segmentList);
  }
}

/*--------------------*/

UINT8 Area_getMemoryPage (in Area_Type area)
{
  /* TODO: is there a difference between bank and memory page? */

  /* TODO: check whether this should be handled by some platform
     specific routine */

  char *procName = "Area_getMemoryPage";
  Boolean precondition = Area__checkValidityPRE(area, procName);
  UINT8 memoryPage = 0x00;

  if (precondition) {
    Area_AttributeSet attributes = area->attributes;

    if (Set_isElement(attributes, Area_Attribute_isInCodeSpace)) {
      memoryPage = 0x0C;
    }

    if (Set_isElement(attributes, Area_Attribute_isInExternalDataSpace)) {
      memoryPage = 0x0D;
    }

    if (Set_isElement(attributes, Area_Attribute_isInBitSpace)) {
      memoryPage = 0x0B;
    }
  }

  return memoryPage;
}

/*--------------------*/

Target_Address Area_getSegmentAddress (in Area_Segment segment)
{
  char *procName = "Area_getSegmentAddress";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);
  Target_Address result = 0;

  if (precondition) {
    result = segment->startAddress;
  }

  return result;
}

/*--------------------*/

Area_Type Area_getSegmentArea (in Area_Segment segment)
{
  char *procName = "Area_getSegmentArea";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);
  Area_Type result = NULL;

  if (precondition) {
    result = segment->parentArea;
  }

  return result;
}

/*--------------------*/

Module_Type Area_getSegmentModule (in Area_Segment segment)
{
  char *procName = "Area_getSegmentModule";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);
  Module_Type result = NULL;

  if (precondition) {
    result = segment->parentModule;
  }

  return result;
}

/*--------------------*/

void Area_getSegmentName (in Area_Segment segment, out String_Type *name)
{
  char *procName = "Area_getSegmentName";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);

  if (precondition) {
    String_copy(name, segment->parentArea->name);
  }
}

/*--------------------*/

void Area_getSegmentSymbols (in Area_Segment segment, 
			     out Symbol_List *symbolList)
{
  char *procName = "Area_getSegmentSymbols";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);

  if (precondition) {
    List_copy(symbolList, segment->symbolList);
  }
}

/*--------------------*/

Target_Address Area_getSize (in Area_Type area)
{
  char *procName = "Area_getSize";
  Boolean precondition = Area__checkValidityPRE(area, procName);
  Target_Address result = 0;

  if (precondition) {
    result = area->totalSize;
  }

  return result;
}


/*--------------------*/
/* SELECTION          */
/*--------------------*/

void Area_setCurrent (in Area_Segment segment)
{
  char *procName = "Area_setCurrent";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);

  if (precondition) {
    Area__currentSegment = segment;
  }
}

/*--------------------*/

void Area_lookup (out Area_Type *area, in String_Type areaName)
{
  *area = List_lookup(Area__list, areaName);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Area_addSymbolToSegment (inout Area_Segment *segment,
			      in Symbol_Type symbol)
{
  char *procName = "Area_addSymbolToSegment";
  Area_Segment currentSegment = *segment;
  Boolean precondition = Area__checkSegmentValidityPRE(currentSegment,
						       procName);

  if (precondition) {
    Object *objectPtr = List_append(&currentSegment->symbolList);
    *objectPtr = symbol;
  }
}

/*--------------------*/

void Area_clearListOfSegments (inout Area_Type *area)
{
  char *procName = "Area_clearListOfSegments";
  Area_Type currentArea = *area;
  Boolean precondition = Area__checkValidityPRE(currentArea, procName);

  if (precondition) {
    List_clear(&currentArea->segmentList);
  }
}

/*--------------------*/

void Area_link (void)
{
  /* TODO: check lkarea::lnkarea for revised allocation strategy */

  Target_Address relativeBaseAddress = 0;
  List_Cursor areaCursor;

  /* make an absolute segment for the special label definitions */
  Area_makeAbsoluteSegment();

  for (areaCursor = List_resetCursor(Area__list);
       areaCursor != NULL;
       List_advanceCursor(&areaCursor)) {
    Area_Type currentArea = 
      Area__attemptConversion(List_getElementAtCursor(areaCursor));

    if (Set_isElement(currentArea->attributes, Area_Attribute_isAbsolute)) {
	/* area has absolute segments */
      Area__linkSegments(currentArea);
    } else {
      /* area has relocatable segments */
      if (currentArea->startAddress == 0) {
        currentArea->startAddress = relativeBaseAddress;
      }

      Area__linkSegments(currentArea);
      relativeBaseAddress = currentArea->startAddress + currentArea->totalSize;
    }

    /* create special symbols for the start address and the length of the
       area */
    if (!String_isEqual(currentArea->name, Area__absoluteAreaName)) {
      String_Type areaName = currentArea->name;
      SizeType nameLength = String_length(areaName) + 2;
      Symbol_Type symbol;
      String_Type specialSymbolName= String_allocate(nameLength);

      String_copyCharArray(&specialSymbolName, "s_");
      String_append(&specialSymbolName, areaName);
      symbol = Symbol_make(specialSymbolName, true, currentArea->startAddress);

      String_copyCharArray(&specialSymbolName, "l_");
      String_append(&specialSymbolName, areaName);
      symbol = Symbol_make(specialSymbolName, true, currentArea->totalSize);
      String_destroy(&specialSymbolName);
    }
  }
}

/*--------------------*/

void Area_replaceSegmentSymbol (inout Area_Segment *segment, 
				in Symbol_Type oldSymbol, 
				in Symbol_Type newSymbol)
{
  char *procName = "Area_replaceSegmentSymbol";
  Area_Segment currentSegment = *segment;
  Boolean precondition = Area__checkSegmentValidityPRE(currentSegment,
						       procName);

  if (precondition) {
    List_Cursor symbolCursor;

    for (symbolCursor = List_resetCursor(currentSegment->symbolList);
	 symbolCursor != NULL;
	 List_advanceCursor(&symbolCursor)) {
      Symbol_Type symbol = List_getElementAtCursor(symbolCursor);

      if (symbol == oldSymbol) {
	List_putElementToCursor(symbolCursor, newSymbol);
	break;
      }
    }
  }
}

/*--------------------*/

void Area_setBaseAddresses (in String_Type segmentName,
			    in Target_Address baseAddress)
{
  List_Cursor areaCursor;

  for (areaCursor = List_resetCursor(Area__list);
       areaCursor != NULL;
       List_advanceCursor(&areaCursor)) {
    Area_Type currentArea = 
      Area__attemptConversion(List_getElementAtCursor(areaCursor));

    if (String_isEqual(currentArea->name, segmentName)) {
      currentArea->startAddress = baseAddress;
    }
  }
}

/*--------------------*/

void Area_setSegmentArea (inout Area_Segment *segment, in Area_Type area)
{
  char *procName = "Area_addSymbolToSegment";
  Area_Segment currentSegment = *segment;
  Boolean precondition = Area__checkSegmentValidityPRE(currentSegment, 
						       procName);

  if (precondition) {
    Object *objectPtr = List_append(&area->segmentList);
    *objectPtr = currentSegment;
    currentSegment->parentArea = area;
  }
}


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void Area_toString (in Area_Type area, out String_Type *representation)
{
  char *procName = "Area_toString";
  Boolean precondition = Area__checkValidityPRE(area, procName);

  if (precondition) {
    SizeType segmentCount;

    String_appendCharArray(representation, "AREA ");
    String_append(representation, area->name);

    String_appendCharArray(representation, " (start_address = ");
    String_appendInteger(representation, area->startAddress, 16);

    String_appendCharArray(representation, ", total_size = ");
    String_appendInteger(representation, area->totalSize, 16);

    String_appendCharArray(representation, ", attributes = ");
    String_appendInteger(representation, area->attributes, 16);

    segmentCount = List_length(area->segmentList);
    String_appendCharArray(representation, ", segment_count = ");
    String_appendInteger(representation, (UINT32) segmentCount, 10);

    String_appendCharArray(representation, ")");
  }
}

/*--------------------*/

void Area_segmentToString (in Area_Segment segment,
			   out String_Type *representation)
{
  char *procName = "Area_segmentToString";
  Boolean precondition = Area__checkSegmentValidityPRE(segment, procName);

  if (precondition) {
    SizeType symbolCount;

    String_appendCharArray(representation, "SEGMENT ");
    String_append(representation, segment->parentArea->name);

    String_appendCharArray(representation, " (start_address = ");
    String_appendInteger(representation, segment->startAddress, 16);

    String_appendCharArray(representation, ", total_size = ");
    String_appendInteger(representation, segment->totalSize, 16);

    symbolCount = List_length(segment->symbolList);
    String_appendCharArray(representation, ", symbol_count = ");
    String_appendInteger(representation, (UINT32) symbolCount, 10);

    String_appendCharArray(representation, ")");
  }
}
