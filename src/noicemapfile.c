/** NoICEMapFile module --
    Implementation of module providing all services for putting out
    mapfiles in NoICE format.  The output routine defined here links
    into the generic MapFile module of the SDCC linker.

    Original version by Thomas Tensi, 2009-03
    based on module lknoice.c by John Hartman

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "noicemapfile.h"

/*========================================*/

#include "area.h"
#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "list.h"
#include "mapfile.h"
#include "set.h"
#include "string.h"
#include "symbol.h"
#include "target.h"

#include <ctype.h>
# define CType_isDigit isdigit

/*========================================*/

/* some constant strings for function symbol suffices */
static String_Type NoICEMapFile__globalFuncSuffix;
static String_Type NoICEMapFile__staticFuncSuffix;
static String_Type NoICEMapFile__endOfFuncSuffix;

static String_Type NoICEMapFile__specialCommentPrefix;
static String_Type NoICEMapFile__currentFile;
static String_Type NoICEMapFile__currentFunction;

/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static void NoICEMapFile__processSymbol (inout File_Type *mapFile,
					 in Symbol_Type symbol,
					 in UINT8 areaMemoryPage);

static void NoICEMapFile__writeDefForFile (inout File_Type *mapFile,
					   in String_Type fileName);

static void NoICEMapFile__writeDefForFunction (inout File_Type *mapFile,
					       in String_Type functionName,
					       in Boolean isStatic,
					       in Target_Address address,
					       in UINT8 areaMemoryPage);

static void NoICEMapFile__writeDefForLine (inout File_Type *mapFile,
					   in String_Type lineNumberString,
					   in Target_Address address,
					   in UINT8 areaMemoryPage);

static void NoICEMapFile__writeDefForSymbol (inout File_Type *mapFile,
					     in String_Type symbolName,
					     in Boolean isStatic,
					     in Target_Address address,
					     in UINT8 areaMemoryPage);

static void NoICEMapFile__writeFunctionEnd (inout File_Type *mapFile,
					    in Target_Address address,
					    in UINT8 areaMemoryPage);

/*--------------------*/
/*--------------------*/

static void NoICEMapFile__appendPagedAddress (inout String_Type *st,
					      in Target_Address address,
					      in UINT8 areaMemoryPage)
  /** appends the string representation of the paged address given by
      <address> and <areaMemoryPage> to string <st> separated by a
      leading blank */
{
  String_appendChar(st, ' ');
  String_appendInteger(st, areaMemoryPage, 16);
  String_appendCharArray(st, ":0x");
  String_appendInteger(st, address, 16);
}

/*--------------------*/

static void NoICEMapFile__processArea (inout File_Type *mapFile,
				       in Area_Type area)
  /** traverses <area> and writes all appropriate information to NoICE
      map file given by <file> */
{
  Symbol_List areaSymbolList = List_make(Symbol_typeDescriptor);
  List_Cursor symbolCursor;
  UINT8 areaMemoryPage = Area_getMemoryPage(area);

  MapFile_getSortedAreaSymbolList(area, &areaSymbolList);

  /* write all symbols to map file */
  for (symbolCursor = List_resetCursor(areaSymbolList);
       symbolCursor != NULL;
       List_advanceCursor(&symbolCursor)) {
    Symbol_Type symbol = List_getElementAtCursor(symbolCursor);
    NoICEMapFile__processSymbol(mapFile, symbol, areaMemoryPage);
  }
}

/*--------------------*/

static void NoICEMapFile__processSymbol (inout File_Type *mapFile,
					 in Symbol_Type symbol,
					 in UINT8 areaMemoryPage)
  /** parses name of <symbol> and writes appropriate information to
      NoICE map file given by <file>; <areaMemoryPage> gives the
      memory page of the symbol area */
{
  char *procName = "NoICEMapFile__processSymbol";

  Target_Address address = Symbol_absoluteAddress(symbol);
  SizeType dotPosition;
  Boolean hasError = false;
  String_Type symbolName = String_make();
  SizeType stringLength;

  Symbol_getName(symbol, &symbolName);

  stringLength = String_length(symbolName);
  dotPosition = String_findCharacter(symbolName, '.');

  if (dotPosition == stringLength) {
    hasError = true;
  } else if (dotPosition == String_notFound) {
    /* just a symbol without any dot separators */
    NoICEMapFile__writeDefForSymbol(mapFile, symbolName, false, 
				    address, areaMemoryPage);
  } else {
    /* there are at least two meaningful parts */

    char firstSuffixCharacter;
    String_Type fileToken = String_make();
    String_Type suffix = String_make();
    SizeType suffixLength;

    /* split at first dot */
    String_getSubstring(&fileToken, symbolName, 1, dotPosition - 1);
    String_getSubstring(&suffix, symbolName, dotPosition + 1, SizeType_max);

    /* check for a dot in <suffix> */
    suffixLength = String_length(suffix);
    dotPosition = String_findCharacter(suffix, '.');

    if (dotPosition == suffixLength) {
      hasError = true;
    } else {
      NoICEMapFile__writeDefForFile(mapFile, fileToken);

      if (dotPosition == String_notFound) {
	firstSuffixCharacter = String_getCharacter(suffix, 1);

	/* <symbolName> is either "file.symbol" or "file.line#" */
	if (CType_isDigit(firstSuffixCharacter)) {
	  NoICEMapFile__writeDefForLine(mapFile, suffix,
					address, areaMemoryPage);
	} else {
	  NoICEMapFile__writeFunctionEnd(mapFile, 0, 0);
	  NoICEMapFile__writeDefForSymbol(mapFile, suffix, true,
					  address, areaMemoryPage);
	}
      } else {
	/* <symbolName> is either "file.function.symbol" or
	   "file.function..SPECIAL" */
	
	String_Type functionToken = String_make();

	/* split at first dot in <suffix> */
	String_getSubstring(&functionToken, suffix, 1, dotPosition - 1);
        String_getSubstring(&suffix, suffix, dotPosition + 1, SizeType_max);
	firstSuffixCharacter = String_getCharacter(suffix, 1);

	if (firstSuffixCharacter == '.') {
	  if (String_isEqual(suffix, NoICEMapFile__globalFuncSuffix)) {
	    NoICEMapFile__writeDefForFunction(mapFile, functionToken, false,
					      address, areaMemoryPage);
	  } else if (String_isEqual(suffix, NoICEMapFile__staticFuncSuffix)) {
	    NoICEMapFile__writeDefForFunction(mapFile, functionToken, true,
					      address, areaMemoryPage);
	  } else if (String_isEqual(suffix, NoICEMapFile__endOfFuncSuffix)) {
	    NoICEMapFile__writeFunctionEnd(mapFile, address, areaMemoryPage);
	  }
	} else {
	  NoICEMapFile__writeDefForFunction(mapFile, functionToken, false,
					    0, 0);

	  /* look for optional level integer */
	  suffixLength = String_length(suffix);
	  dotPosition = String_findCharacter(suffix, '.');
	  
	  if (dotPosition != String_notFound && dotPosition != suffixLength) {
	    long level;
	    String_Type levelString = String_make();
	    String_getSubstring(&levelString, suffix, dotPosition + 1,
				SizeType_max);

	    if (String_convertToLong(levelString, 10, &level) && level > 0) {
	      String_appendChar(&fileToken, '_');
	      String_appendInteger(&fileToken, level, 10);
	    }

	    String_destroy(&levelString);
	  }

	  NoICEMapFile__writeDefForSymbol(mapFile, fileToken, true, 
					  address, areaMemoryPage);
	}

	String_destroy(&functionToken);
      }
    }

    String_destroy(&suffix);
    String_destroy(&fileToken);
  }

  if (hasError) {
    Error_raise(Error_Criticality_warning, 
		"bad symbol in %s: %s", procName, 
		String_asCharPointer(symbolName));
  }

  String_destroy(&symbolName);
}

/*--------------------*/

static void NoICEMapFile__writeDefForFile (inout File_Type *mapFile,
					   in String_Type fileName)
  /** writes definition text for a file definition onto <file> for
      file with name <fileName> */
{
  if (!String_isEqual(fileName, NoICEMapFile__currentFile)) {
    String_Type st = String_makeFromCharArray("FILE ");
    String_append(&st, fileName);
    String_append(&st, String_newline);
    File_writeString(mapFile, st);
    String_destroy(&st);
  }
}

/*--------------------*/

static void NoICEMapFile__writeDefForFunction (inout File_Type *mapFile,
					       in String_Type functionName,
					       in Boolean isStatic,
					       in Target_Address address,
					       in UINT8 areaMemoryPage)
  /** writes definition text for a function definition onto <file> for
      function with name <functionName> with properties <isStatic>,
      <address> and <areaMemoryPage> */
{
  if (!String_isEqual(functionName, NoICEMapFile__currentFunction)) {
    char *funcCommand = (isStatic ? "SFUNC " : "FUNC ");
    String_Type st = String_make();

    String_copy(&NoICEMapFile__currentFunction, functionName);

    if (address != 0) {
      char *defCommand  = (isStatic ? "DEFS " : "DEF ");

      String_appendCharArray(&st, defCommand);
      String_append(&st, functionName);
      NoICEMapFile__appendPagedAddress(&st, address, areaMemoryPage);
      String_append(&st, String_newline);
    }
     
    String_appendCharArray(&st, funcCommand);
    String_append(&st, functionName);

    if (address != 0) {
      NoICEMapFile__appendPagedAddress(&st, address, areaMemoryPage);
    }

    String_append(&st, String_newline);
    File_writeString(mapFile, st);
    String_destroy(&st);
  }
}

/*--------------------*/

static void NoICEMapFile__writeDefForLine (inout File_Type *mapFile,
					   in String_Type lineNumberString,
					   in Target_Address address,
					   in UINT8 areaMemoryPage)
  /** writes definition text for a line definition onto <file> for
      line with number <lineNumberString> with properties <address>
      and <areaMemoryPage> */
{
  String_Type st = String_makeFromCharArray("LINE ");
  long lineNumber;

  String_convertToLong(lineNumberString, 10, &lineNumber);
  String_appendInteger(&st, lineNumber, 10);
  NoICEMapFile__appendPagedAddress(&st, address, areaMemoryPage);
  String_append(&st, String_newline);
  File_writeString(mapFile, st);
  String_destroy(&st);
}

/*--------------------*/

static void NoICEMapFile__writeDefForSymbol (inout File_Type *mapFile,
					     in String_Type symbolName,
					     in Boolean isStatic,
					     in Target_Address address,
					     in UINT8 areaMemoryPage)
  /** writes definition text for a symbol definition onto <file> for
      symbol with name <symbolName> with properties <isStatic>,
      <address> and <areaMemoryPage> */
{
  String_Type st = String_makeFromCharArray((isStatic ? "DEFS " : "DEF "));
  String_append(&st, symbolName);
  NoICEMapFile__appendPagedAddress(&st, address, areaMemoryPage);
  String_append(&st, String_newline);
  File_writeString(mapFile, st);
  String_destroy(&st);
}
  
/*--------------------*/

static void NoICEMapFile__writeFunctionEnd (inout File_Type *mapFile,
					    in Target_Address address,
					    in UINT8 areaMemoryPage)
  /** writes definition text for the end of a function definition onto
      <file> with properties <address> and <areaMemoryPage> */
{
  if (String_length(NoICEMapFile__currentFunction) > 0) {
    String_Type st = String_makeFromCharArray("ENDF");
    String_clear(&NoICEMapFile__currentFunction);

    if (address != 0) {
      NoICEMapFile__appendPagedAddress(&st, address, areaMemoryPage);
    }
    
    String_append(&st, String_newline);
    File_writeString(mapFile, st);

    String_destroy(&st);
  }
}

/*========================================*/
/*            EXPORTED FEATURES           */
/*========================================*/

void NoICEMapFile_initialize (void)
{
  NoICEMapFile__currentFile      = String_make();
  NoICEMapFile__currentFunction  = String_make();

  NoICEMapFile__specialCommentPrefix = String_makeFromCharArray(";!");

  NoICEMapFile__globalFuncSuffix = String_makeFromCharArray(".FN");
  NoICEMapFile__staticFuncSuffix = String_makeFromCharArray(".SFN");
  NoICEMapFile__endOfFuncSuffix  = String_makeFromCharArray(".EFN");
}

/*--------------------*/

void NoICEMapFile_finalize (void)
{
  String_destroy(&NoICEMapFile__currentFile);
  String_destroy(&NoICEMapFile__currentFunction);
  String_destroy(&NoICEMapFile__specialCommentPrefix);
  String_destroy(&NoICEMapFile__globalFuncSuffix);
  String_destroy(&NoICEMapFile__staticFuncSuffix);
  String_destroy(&NoICEMapFile__endOfFuncSuffix);
}

/*--------------------*/

void NoICEMapFile_addSpecialComment (inout File_Type *mapFile,
				     in String_Type comment)
{
  if (String_hasPrefix(comment, NoICEMapFile__specialCommentPrefix)) {
    String_Type st = String_make();
    SizeType commentPrefixLength =
      String_length(NoICEMapFile__specialCommentPrefix);
    String_getSubstring(&st, comment, commentPrefixLength, SizeType_max);
    File_writeString(mapFile, st);
    String_destroy(&st);
  }
}

/*--------------------*/

void NoICEMapFile_generate (inout File_Type *mapFile)
{
  List_Cursor areaCursor;
  Area_List areaList = List_make(Area_typeDescriptor);

  /*{
    if (jfp) fprintf( jfp, "LOAD %s.IHX\n", linkp->f_idp );
    }*/

  Area_getList(&areaList);

  for (areaCursor = List_resetCursor(areaList);  areaCursor != NULL;
       List_advanceCursor(&areaCursor)) {
    Area_Type area = List_getElementAtCursor(areaCursor);
    NoICEMapFile__processArea(mapFile, area);
  }

  List_destroy(&areaList);
}


