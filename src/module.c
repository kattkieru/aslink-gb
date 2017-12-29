/** Module module --
    Implementation of module providing all services for module
    definitions in the generic SDCC linker

    Original version by Thomas Tensi, 2006-08
    based on the module lkhead.c by Alan R. Baldwin
*/

#include "module.h"

#include "area.h"
#include "error.h"
#include "globdefs.h"
#include "list.h"
#include "string.h"
#include "symbol.h"

/*========================================*/

#define Module__magicNumber 0x132465

typedef struct Module__Record {
  long magicNumber;
  String_Type name;
  Module_SegmentIndex segmentCount;
  Module_SymbolIndex symbolCount;
  String_Type associatedFileName;
  Area_SegmentList segmentList;
  Symbol_List symbolList;
} Module__Record;
/** type describing the characteristics of a module
    represented by a single link file; it contains a
    reference to a link file descriptor which describes
    the file containing the header directive, the number
    of data/code area segments contained in this module,
    the number of symbols referenced/defined in this module,
    a list of segments and a list of symbols contained in this
    module */

static List_Type Module__list;
  /** list of all modules known to the linker */

static List_Type Module__indexByFileName;
  /** index list of all modules for access by file name */

static List_Type Module__indexByName;
  /** index list of all modules for access by module name */

static Module_Type Module__currentModule;
  /** the currently processed module */

/* ---- type descriptors ---- */ 
static Object Module__make (void);
static Boolean Module__hasFileNameKey (in Object object, in Object key);
static Boolean Module__hasNameKey (in Object object, in Object key);

static TypeDescriptor_Record Module__tdRecord = 
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ NULL,
    /* .destructionProc = */ NULL, /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Module__hasNameKey };

TypeDescriptor_Type Module_typeDescriptor = &Module__tdRecord;


static TypeDescriptor_Record Module__recordTDRecord = 
  { /* .objectSize = */ sizeof(Module__Record), /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ Module__make,
    /* .destructionProc = */ (TypeDescriptor_DestructionProc) Module_destroy,
    /* .hashCodeProc = */ NULL, /* .keyValidationProc = */ NULL };

static TypeDescriptor_Type Module__recordTypeDescriptor = 
  &Module__recordTDRecord;
  /** variable used for describing the type properties when module
      records occur in generic types like lists */

static TypeDescriptor_Record Module__typeByFileNameDRecord =
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ NULL,
    /* .destructionProc = */ NULL, /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Module__hasFileNameKey };

static TypeDescriptor_Type Module__typeByFileNameDescriptor =
  &Module__typeByFileNameDRecord;
  /** variable used for describing the type properties when module
      object occur in generic types like lists and should be searched
      by file name */


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Module_Type Module__attemptConversion (in Object module)
  /** verifies that <module> is really a pointer to a module; if not,
      the program stops with an error message */
{
  return attemptConversion("Module_Type", module, Module__magicNumber);
}

/*--------------------*/

static Boolean Module__isValid (in Object module)
{
  return isValidObject(module, Module__magicNumber);
}


/*--------------------*/

static Boolean Module__checkValidityPRE (in Object module, in char *procName)
  /** checks as a precondition of routine <procName> whether <module> is
      a valid module and returns the check result */
{
  return PRE(Module__isValid(module), procName, "invalid module");
}


/*--------------------*/
/*--------------------*/

static Object *Module__addEntry (inout Module_Type *module,
				 inout List_Type *list)
  /** adds a new entry to <list> within <module> */
{
  if (*module != NULL) {
    return List_append(list);
  } else {
    Error_raise(Error_Criticality_fatalError, "No header defined\n");
    return NULL;
  }
}

/*--------------------*/

static Boolean Module__hasFileNameKey (in Object object, in Object key)
  /** checks whether module <object> has <key> as identifying file
      name */
{
  Module_Type module = Module__attemptConversion(object);
  String_Type fileName = key;
  return String_isEqual(module->associatedFileName, fileName);
}

/*--------------------*/

static Boolean Module__hasNameKey (in Object object, in Object key)
  /** checks whether module <object> has <key> as identifying name */
{
  Module_Type module = Module__attemptConversion(object);
  String_Type moduleName = key;
  return String_isEqual(module->name, moduleName);
}

/*--------------------*/

static Object Module__make (void)
  /** private construction of module used when a new entry is
      created in module record list */
{
  Module_Type module = NEW(Module__Record);

  module->magicNumber        = Module__magicNumber;
  module->name               = String_make();
  module->associatedFileName = String_make();
  module->segmentList        = List_make(Area_segmentTypeDescriptor);
  module->symbolList         = List_make(Symbol_typeDescriptor);

  return module;
}

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Module_initialize (void)
{
  Module__list            = List_make(Module__recordTypeDescriptor);
  Module__indexByFileName = List_make(Module__typeByFileNameDescriptor);
  Module__indexByName     = List_make(Module_typeDescriptor);
}

/*--------------------*/

void Module_finalize (void)
{
  List_destroy(&Module__list);
  List_destroy(&Module__indexByFileName);
  List_destroy(&Module__indexByName);
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

void Module_make (in String_Type associatedFileName,
		  in Module_SegmentIndex segmentCount,
		  in Module_SymbolIndex symbolCount)

{
  Object *objectPtr = List_append(&Module__list);
  Module_Type module = Module__attemptConversion(*objectPtr);

  String_copy(&module->associatedFileName, associatedFileName);
  module->segmentCount = segmentCount;
  module->symbolCount  = symbolCount;

  Module__currentModule = module;

  /* update index lists */
  objectPtr = List_append(&Module__indexByName);
  *objectPtr = module;
  objectPtr = List_append(&Module__indexByFileName);
  *objectPtr = module;
}


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Module_destroy (inout Module_Type *module)
{
  char *procName = "Module_destroy";
  Module_Type currentModule = *module;
  Boolean precondition = Module__checkValidityPRE(currentModule, procName);

  if (precondition) {
    String_destroy(&currentModule->name);
    String_destroy(&currentModule->associatedFileName);
    List_destroy(&currentModule->segmentList);
    List_destroy(&currentModule->symbolList);
    *module = NULL;
  }
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Module_getFileName (in Module_Type module, out String_Type *fileName)
{
  char *procName = "Module_getFileName";
  Boolean precondition = Module__checkValidityPRE(module, procName);

  if (precondition) {
    String_copy(fileName, module->associatedFileName);
  }
}

/*--------------------*/

void Module_getName (in Module_Type module, out String_Type *name)
{
  char *procName = "Module_getName";
  Boolean precondition = Module__checkValidityPRE(module, procName);

  if (precondition) {
    String_copy(name, module->name);
  }
}

/*--------------------*/

Area_Segment Module_getSegment (in Module_Type module,
				in Module_SegmentIndex segmentIndex)
{
  char *procName = "Module_getSegment";
  Boolean precondition = Module__checkValidityPRE(module, procName);
  Area_Segment result = NULL;

  if (precondition) {
    result = List_getElement(module->segmentList, segmentIndex);
  }

  return result;
}

/*--------------------*/

Area_Segment Module_getSegmentByName (in Module_Type module,
				      in String_Type segmentName)
{
  char *procName = "Module_getSegmentByName";
  Boolean precondition = Module__checkValidityPRE(module, procName);
  Area_Segment result = NULL;

  if (precondition) {
    result = List_lookup(module->segmentList, segmentName);
  }

  return result;
}

/*--------------------*/

Symbol_Type Module_getSymbol (in Module_Type module,
			      in Module_SymbolIndex symbolIndex)
{
  char *procName = "Module_getSymbol";
  Boolean precondition = Module__checkValidityPRE(module, procName);
  Symbol_Type result = NULL;

  if (precondition) {
    result = List_getElement(module->symbolList, symbolIndex);
  }

  return result;
}

/*--------------------*/

Symbol_Type Module_getSymbolByName (in Module_Type module,
				    in String_Type symbolName)
{
  char *procName = "Module_getSymbolByName";
  Boolean precondition = Module__checkValidityPRE(module, procName);
  Symbol_Type result = NULL;

  if (precondition) {
    result = List_lookup(module->symbolList, symbolName);
  }

  return result;
}


/*--------------------*/
/* STATUS REPORT      */
/*--------------------*/

Module_Type Module_currentModule (void)
{
  return Module__currentModule;
}


/*--------------------*/
/* ITERATION          */
/*--------------------*/

void Module_getModuleList (inout List_Type *moduleList)
{
  List_copy(moduleList, Module__indexByName);
}

/*--------------------*/

void Module_getSegmentList (in Module_Type module,
			    inout Area_SegmentList *segmentList)
{
  char *procName = "Module_getSegmentList";
  Boolean precondition = Module__checkValidityPRE(module, procName);

  if (precondition) {
    List_copy(segmentList, module->segmentList);
  }
}

/*--------------------*/

void Module_getSymbolList (in Module_Type module,
			   inout Symbol_List *symbolList)
{
  char *procName = "Module_getSymbolList";
  Boolean precondition = Module__checkValidityPRE(module, procName);

  if (precondition) {
    List_copy(symbolList, module->symbolList);
  }
}


/*--------------------*/
/* SELECTION          */
/*--------------------*/

void Module_setCurrentByName (in String_Type name,
			      out Boolean *isFound)
{
  Object m = List_lookup(Module__indexByName, name);
  Module_Type module = Module__attemptConversion(m);
  Module__currentModule = module;
  *isFound = (module != NULL);
}


/*--------------------*/

void Module_setCurrentByFileName (in String_Type fileName,
				  out Boolean *isFound)
{
  Object m = List_lookup(Module__indexByFileName, fileName);
  Module_Type module = Module__attemptConversion(m);
  Module__currentModule = module;
  *isFound = (module != NULL);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Module_setName (in String_Type name)
{
  if (Module__currentModule != NULL) {
    String_copy(&Module__currentModule->name, name);
  } else {
    Error_raise(Error_Criticality_fatalError, "No header defined\n");
  }
}

/*--------------------*/

void Module_addSegment (inout Module_Type *module, in Area_Segment segment)
{
  char *procName = "Module_addSegment";
  Module_Type currentModule = *module;
  Boolean precondition = Module__checkValidityPRE(currentModule, procName);

  if (precondition) {
    Area_Segment *segmentPointer = 
      (Area_Segment *) Module__addEntry(&currentModule, 
					&currentModule->segmentList);

    if (segmentPointer != NULL) {
      *segmentPointer = segment;
    }
  }
}

/*--------------------*/

void Module_addSymbol (inout Module_Type *module, in Symbol_Type symbol)
{
  char *procName = "Module_addSymbol";
  Module_Type currentModule = *module;
  Boolean precondition = Module__checkValidityPRE(currentModule, procName);

  if (precondition) {
    Symbol_Type *symbolPointer =
      (Symbol_Type *) Module__addEntry(&currentModule,
				       &currentModule->symbolList);

    if (symbolPointer != NULL) {
      *symbolPointer = symbol;
    }
  }
}

/*--------------------*/

void Module_replaceSymbol (inout Module_Type *module, 
			   in Symbol_Type oldSymbol, in Symbol_Type newSymbol)
{
  char *procName = "Module_replaceSymbol";
  Module_Type currentModule = *module;
  Boolean precondition = Module__checkValidityPRE(currentModule, procName);

  if (precondition) {
    List_Cursor symbolCursor;

    /* replace reference in symbol list */
    for (symbolCursor = List_resetCursor(currentModule->symbolList);
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
/* CONVERSION         */
/*--------------------*/

void Module_toString (in Module_Type module, out String_Type *representation)
{
  char *procName = "Module_toString";
  Boolean precondition = Module__checkValidityPRE(module, procName);

  if (precondition) {
    List_Cursor segmentCursor;
    List_Cursor symbolCursor;

    /* header line */
    String_appendCharArray(representation, "MODULE ");
    String_append(representation, module->name);

    /* segment count line */
    String_appendCharArray(representation, " (segment_count = ");
    String_appendInteger(representation, module->segmentCount, 10);

    /* symbol count line */
    String_appendCharArray(representation, ", symbol_count = ");
    String_appendInteger(representation, module->symbolCount, 10);
    String_appendCharArray(representation, "\n  ");

    /* segment lines */
    for (segmentCursor = List_resetCursor(module->segmentList);
	 segmentCursor != NULL;
	 List_advanceCursor(&segmentCursor)) {
      Area_Segment segment = List_getElementAtCursor(segmentCursor);
      Area_segmentToString(segment, representation);
      String_appendChar(representation, ' ');
    }

    String_appendCharArray(representation, "\n  ");

    /* symbol lines */
    for (symbolCursor = List_resetCursor(module->symbolList);
	 symbolCursor != NULL;
	 List_advanceCursor(&symbolCursor)) {
      Symbol_Type symbol = List_getElementAtCursor(symbolCursor);
      Symbol_toString(symbol, representation);
      String_appendChar(representation, ' ');
    }
  }
}
