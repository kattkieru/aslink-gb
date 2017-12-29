/** Symbol module --
    Implementation of module providing all services for handling
    external symbols in the generic SDCC linker

    Original version by Thomas Tensi, 2006-08
    based on the module lksym.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "symbol.h"

/*========================================*/

#include "area.h"
#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "list.h"
#include "map.h"
#include "module.h"
#include "set.h"
#include "string.h"
#include "typedescriptor.h"

/*========================================*/

#define Symbol__magicNumber 0x4711

/*--------------------*/

typedef enum {
  Symbol__Attribute_isDefined, Symbol__Attribute_isReferenced,
  Symbol__Attribute_isSurrogate
} Symbol__Attribute;
  /** possible attributes of a symbol: it may be referenced and/or
      defined and - when banking is used - it may be a surrogate
      symbol for doing interbank calls*/

/*--------------------*/

typedef Set_Type Symbol__AttributeSet;
  /** a set of symbol attributes*/

/*--------------------*/

typedef struct Symbol__Record {
  long magicNumber;
  String_Type name;
  Area_Segment definingSegment;
  Symbol__AttributeSet attributes;
  Target_Address startAddress;
} Symbol__Record;
  /** expanded information of a symbol created for every unique symbol
      referenced or defined within the linker files; it consist of the
      symbol's name, the defining area segment, the attributes
      denoting whether it is referenced and/or defined and the
      relative address within the enclosing area */


static List_Type Symbol__list;
static Map_Type Symbol__indexByName;
static Boolean Symbol__platformIsCaseSensitive;
  /** tells whether platform uses case-sensitive names */

/*--------------------*/

static Boolean Symbol__hasKey (in Object listElement, in Object key);
static Object Symbol__make (void);

static TypeDescriptor_Record Symbol__tdRecord = 
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ NULL,
    /* .destructionProc = */ NULL, /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Symbol__hasKey };

TypeDescriptor_Type Symbol_typeDescriptor = &Symbol__tdRecord;

static TypeDescriptor_Record Symbol__recordTDRecord =
  { /* .objectSize = */ sizeof(Symbol__Record), /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, 
    /* .constructionProc = */ (TypeDescriptor_ConstructionProc) Symbol__make,
    /* .destructionProc = */ (TypeDescriptor_DestructionProc) Symbol_destroy,
    /* .hashCodeProc = */ NULL, /* .keyValidationProc = */ Symbol__hasKey };

static TypeDescriptor_Type Symbol__recordTypeDescriptor = 
  &Symbol__recordTDRecord;
  /** variable used for describing the type properties when symbol
      records occur in generic types like lists */


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Symbol_Type Symbol__attemptConversion (in Object symbol)
  /** verifies that <symbol> is really a pointer to a symbol;
      if not, the program stops with an error message  */
{
  return attemptConversion("Symbol_Type", symbol, Symbol__magicNumber);
}

/*--------------------*/

static Boolean Symbol__isValid (in Object symbol)
{
  return isValidObject(symbol, Symbol__magicNumber);
}


/*--------------------*/

static Boolean Symbol__checkValidityPRE (in Object symbol, in char *procName)
  /** checks as a precondition of routine <procName> whether <symbol> is
      a valid symbol and returns the check result */
{
  return PRE(Symbol__isValid(symbol), procName, "invalid symbol");
}


/*--------------------*/
/*--------------------*/

static Symbol_Type Symbol__lookup (in String_Type name, in Boolean isCreated)
  /** looks up a symbol with <name> in the symbol hash table and
      returns pointer to it */
{
  Symbol_Type symbol;

  if (!Symbol__platformIsCaseSensitive) {
    /* normalize name to upper case */
    String_Type upperCaseName = String_make();
    String_convertToUpperCase(name, &upperCaseName);
    String_copy(&name, upperCaseName);
    String_destroy(&upperCaseName);
  }

  symbol = Map_lookup(Symbol__indexByName, name);
  
  if (symbol == NULL && isCreated) {
    Object *objectPtr = List_append(&Symbol__list);
    symbol = Symbol__attemptConversion(*objectPtr);

    String_copy(&symbol->name, name);
    Map_set(&Symbol__indexByName, name, symbol);
  }
  
  return symbol;
}

/*--------------------*/

static Boolean Symbol__hasKey (in Object object, in Object key)
  /** checks whether <object> has <key> as identification */
{
  Symbol_Type symbol = Symbol__attemptConversion(object);
  String_Type name = key;
  return String_isEqual(symbol->name, name);
}

/*--------------------*/

static Object Symbol__make (void)
  /** private construction of symbol used when a new entry is
      created in symbol record list */
{
  Symbol_Type symbol = NEW(Symbol__Record);

  symbol->magicNumber     = Symbol__magicNumber;
  symbol->name            = String_make();
  symbol->definingSegment = NULL;
  symbol->startAddress    = 0;
  Set_clear(&symbol->attributes);

  return symbol;
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Symbol_initialize (in Boolean platformIsCaseSensitive)
{
  /* set up list and name index for symbols */
  Symbol__list = List_make(Symbol__recordTypeDescriptor);
  Symbol__indexByName = Map_make(String_typeDescriptor);
  Symbol__platformIsCaseSensitive = platformIsCaseSensitive;
}

/*--------------------*/

void Symbol_finalize (void)
{
  List_destroy(&Symbol__list);
  Map_destroy(&Symbol__indexByName);
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Symbol_Type Symbol_make (in String_Type symbolName, in Boolean isDefinition,
			 in Target_Address startAddress)
{
  Symbol_Type symbol = Symbol__lookup(symbolName, true);
  Symbol__Attribute newValue = (isDefinition ? Symbol__Attribute_isDefined
   				             : Symbol__Attribute_isReferenced);
  Module_Type currentModule = Module_currentModule();

  if (isDefinition) {
    /* symbol is defined here */
    if (Symbol_isDefined(symbol)) {
      Error_raise(Error_Criticality_warning,
		  "Multiple definition of symbol %s", symbolName);
    }

    symbol->startAddress    = startAddress;
    symbol->definingSegment = Area_currentSegment();
    Area_addSymbolToSegment(&symbol->definingSegment, symbol);
  } else {
    /* symbol is referenced */
    if (startAddress != 0) {
      Error_raise(Error_Criticality_warning, 
		  "Non-zero address field in symbol reference %s",
		  symbolName);
    }
  }

  Set_include(&(symbol->attributes), newValue);
  Module_addSymbol(&currentModule, symbol);

  return symbol;
}

/*--------------------*/

Symbol_Type Symbol_makeBySplit (in Symbol_Type oldSymbol, 
				in String_Type symbolName)
{
  char *procName = "Symbol_makeBySplit";
  Boolean precondition = (PRE(Symbol__isValid(oldSymbol), procName,
			      "invalid symbol to be split")
			  && PRE(Symbol_isDefined(oldSymbol), procName,
				 "symbol to be split not defined")
			  && PRE(!Symbol_isSurrogate(oldSymbol), procName,
				 "symbol to be split may not be a surrogate"));
  Symbol_Type newSymbol = NULL;

  if (precondition) {
    String_Type oldSymbolName = String_make();
    Area_Segment oldSymbolSegment = oldSymbol->definingSegment;
    Module_Type oldSymbolModule = Area_getSegmentModule(oldSymbolSegment);

    String_copy(&oldSymbolName, oldSymbol->name);
    newSymbol = Symbol__lookup(symbolName, true);

    /* direct all references going to <oldSymbol> to target
       <newSymbol> (simply by swapping the names of the symbols and
       changing the name index entries */
    {
      Symbol_Type tempSymbol;

      /* swap the symbols */
      tempSymbol = newSymbol;
      newSymbol  = oldSymbol;
      oldSymbol  = tempSymbol;
    
      /* adapt the names */
      String_copy(&oldSymbol->name, oldSymbolName);
      String_copy(&newSymbol->name, symbolName);

      /* restore the old symbol data */
      oldSymbol->definingSegment = oldSymbolSegment;
      oldSymbol->startAddress    = newSymbol->startAddress;
      oldSymbol->attributes      = newSymbol->attributes;

      /* correct the references in the index */
      Map_set(&Symbol__indexByName, symbolName, newSymbol);
      Map_set(&Symbol__indexByName, oldSymbolName, oldSymbol);
    }

    /* the old symbol keeps its attributes, but is not referenced any
       longer */
    Set_exclude(&oldSymbol->attributes, Symbol__Attribute_isReferenced);

    /* the new symbol is a surrogate and referenced, but not yet
       defined */
    newSymbol->startAddress = 0;
    newSymbol->attributes = Set_make(Symbol__Attribute_isReferenced);
    Set_include(&newSymbol->attributes, Symbol__Attribute_isSurrogate);

    /* make the defining module reference the old symbol (after the
       swap it points to <newSymbol> ...) */
    Module_replaceSymbol(&oldSymbolModule, newSymbol, oldSymbol);
    Area_replaceSegmentSymbol(&oldSymbolSegment, newSymbol, oldSymbol);
  }

  return newSymbol;
}


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Symbol_destroy (inout Symbol_Type *symbol)
{
  char *procName = "Symbol_destroy";
  Symbol_Type currentSymbol = *symbol;
  Boolean precondition = Symbol__checkValidityPRE(currentSymbol, procName);

  if (precondition) {
    currentSymbol->magicNumber = 0;
    String_destroy(&currentSymbol->name);
  }

  *symbol = NULL;
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Symbol_getName (in Symbol_Type symbol, out String_Type *name)
{
  char *procName = "Symbol_getName";
  Boolean precondition = Symbol__checkValidityPRE(symbol, procName);

  if (precondition) {
    String_copy(name, symbol->name);
  }
}

/*--------------------*/

Area_Segment Symbol_getSegment (in Symbol_Type symbol)
{
  char *procName = "Symbol_getSegment";
  Boolean precondition = Symbol__checkValidityPRE(symbol, procName);
  Area_Segment result = NULL;

  if (precondition) {
    result = symbol->definingSegment;
  }

  return result;
}

/*--------------------*/

Boolean Symbol_isDefined (in Symbol_Type symbol)
{
  char *procName = "Symbol_isDefined";
  Boolean precondition = Symbol__checkValidityPRE(symbol, procName);
  Boolean result = false;

  if (precondition) {
    result = Set_isElement(symbol->attributes, Symbol__Attribute_isDefined);
  }

  return result;
}

/*--------------------*/

Boolean Symbol_isSurrogate (in Symbol_Type symbol)
{
  char *procName = "Symbol_isSurrogate";
  Boolean precondition = Symbol__checkValidityPRE(symbol, procName);
  Boolean result = false;

  if (precondition) {
    result = Set_isElement(symbol->attributes, Symbol__Attribute_isSurrogate);
  }

  return result;
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Symbol_setAddressForName (in String_Type symbolName,
			       in Target_Address address)
{
  Symbol_Type symbol = Symbol_lookup(symbolName);
  char *symName = String_asCharPointer(symbolName);

  if (symbol == NULL) {
    Error_raise(Error_Criticality_warning,
		"Predefined symbol %s has never been referenced",
		symName);
  } else {
    if (Symbol_isDefined(symbol)) {
      Error_raise(Error_Criticality_warning,
		  "Predefined symbol %s has already been defined elsewhere",
		  symName);
      symbol->definingSegment = NULL;
    }

    symbol->startAddress = address;
    Set_include(&symbol->attributes, Symbol__Attribute_isDefined);
  }
}


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

Symbol_Type Symbol_lookup (in String_Type symbolName)
{
  return Symbol__lookup(symbolName, false);
}

/*--------------------*/

Target_Address Symbol_absoluteAddress (in Symbol_Type symbol)
{
  char *procName = "Symbol_absoluteAddress";
  Boolean precondition = Symbol__checkValidityPRE(symbol, procName);
  Target_Address result = 0;

  if (precondition) {
    result = symbol->startAddress;

    if (symbol->definingSegment != NULL) {
      result += Area_getSegmentAddress(symbol->definingSegment);
    }
  }

  return result;
}


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

void Symbol_getUndefinedSymbolList (inout List_Type *undefinedSymbolList)
{
  List_Cursor symbolCursor;

  List_clear(undefinedSymbolList);

  /* traverse the symbol table */
  for (symbolCursor = List_resetCursor(Symbol__list);
       symbolCursor != NULL;
       List_advanceCursor(&symbolCursor)) {
    Object object = List_getElementAtCursor(symbolCursor);
    Symbol_Type symbol = Symbol__attemptConversion(object);

    if (symbol->definingSegment == NULL) {
      /* this cannot happen unless it is undefined??? */
    }

    if (!Symbol_isDefined(symbol)) {
      /* referenced, but not defined ==> add it to result list */
      Object *objectPtr = List_append(undefinedSymbolList);
      *objectPtr = symbol;
    }
  }
}

/*--------------------*/

void Symbol_checkForUndefinedSymbols (inout File_Type *file)
{
  List_Type moduleList;
  List_Type moduleSymbolList;
  List_Cursor symbolCursor;
  Symbol_List undefinedSymbolList;

  moduleList = List_make(Module_typeDescriptor);
  moduleSymbolList = List_make(Symbol_typeDescriptor);
  undefinedSymbolList = List_make(Symbol_typeDescriptor);
  Symbol_getUndefinedSymbolList(&undefinedSymbolList);

  /* traverse the list of undefined symbols */
  for (symbolCursor = List_resetCursor(undefinedSymbolList);
       symbolCursor != NULL;
       List_advanceCursor(&symbolCursor)) {
    Object object = List_getElementAtCursor(symbolCursor);
    Symbol_Type symbol = Symbol__attemptConversion(object);
    List_Cursor moduleCursor;

    Module_getModuleList(&moduleList);

    /* traverse all modules for that symbol */
    for (moduleCursor = List_resetCursor(moduleList);
	 moduleCursor != NULL;
	 List_advanceCursor(&moduleCursor)) {
      Module_Type module = List_getElementAtCursor(moduleCursor);

      Module_getSymbolList(module, &moduleSymbolList);

      if (List_lookup(moduleSymbolList, symbol->name) != NULL) {
	/* module references symbol */
	String_Type moduleName = String_make();
	Module_getName(module, &moduleName);
	File_writeCharArray(file, "Undefined Global ");
	File_writeString(file, symbol->name);
	File_writeCharArray(file, " referenced by module ");
	File_writeString(file, moduleName);
	File_writeChar(file, '\n');
	String_destroy(&moduleName);
      }
    }
  }

  List_destroy(&undefinedSymbolList);
  List_destroy(&moduleSymbolList);
  List_destroy(&moduleList);
}


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void Symbol_toString (in Symbol_Type symbol, out String_Type *representation)
{
  char *procName = "Symbol_toString";
  Boolean precondition = Symbol__checkValidityPRE(symbol, procName);

  if (precondition) {
    String_appendCharArray(representation, "SYMBOL ");
    String_append(representation, symbol->name);

    String_appendCharArray(representation, " (start_address = ");
    String_appendInteger(representation, symbol->startAddress, 16);

    String_appendCharArray(representation, ", attributes = {");

    if (Symbol_isDefined(symbol)) {
      String_appendCharArray(representation, "DEF ");
    }

    if (Set_isElement(symbol->attributes, Symbol__Attribute_isReferenced)) {
      String_appendCharArray(representation, "REF ");
    }

    if (Symbol_isSurrogate(symbol)) {
      String_appendCharArray(representation, "SURR ");
    }

    String_appendCharArray(representation, "})");
  }
}
