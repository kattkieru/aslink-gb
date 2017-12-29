/** MapFile module --
    Implementation of module providing all services for putting out
    mapfiles.  Those map files give an overview about the object files
    read, the allocation of symbols and areas and the library files
    used.

    Original version by Thomas Tensi, 2008-03
    based on the module lklist.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "mapfile.h"

/*========================================*/

#include "globdefs.h"
#include "area.h"
#include "error.h"
#include "file.h"
#include "library.h"
#include "multimap.h"
#include "string.h"
#include "stringlist.h"
#include "stringtable.h"

#include <stdlib.h>
# define StdLib_qsort   qsort

/*========================================*/

#define MapFile__maxCount 10
  /** at most 10 different map files may be open simultaneously */

typedef struct {
  Boolean isInUse;
  File_Type file;
  String_Type suffix;
  MapFile_ProcDescriptor routines;
} MapFile__Descriptor;
  /** type to store the properties of map files with assigned output
      procedures */


typedef struct {
  Symbol_Type symbol;
  Target_Address address;
} MapFile__SymbolWithAddress;
  /** entry for mapping from symbol to associated address */


typedef void (*MapFile__DescriptorHandlerProc)(inout MapFile__Descriptor *d);
  /** routine type to perform some operation on an active map file
      descriptor */

static MapFile__Descriptor MapFile__list[MapFile__maxCount];
  /** list of descriptors for the registered map files */

static UINT8 MapFile__count;
  /** the number of actively registered map files */

static Boolean MapFile__isOpen;

static String_Type MapFile__tempString;
  /** string used to transfer parameter information into the handler
      routines */

static UINT8 MapFile__base = 16;
  /** default base used for number output */

static StringList_Type MapFile__linkFileList;
  /** list of link files for output in map files */


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static void MapFile__closeFile (in MapFile__Descriptor *descriptor)
{
  File_close(&descriptor->file);
  descriptor->isInUse = false;
}

/*--------------------*/

static int MapFile__compareSymbolsByAddress (in MapFile__SymbolWithAddress *a,
					     in MapFile__SymbolWithAddress *b)
  /** compares the addresses of two symbol entries and returns -1, 0
      or +1 for less, equal and greater */
{
  int result;

  if (a->address == b->address) {
    result = 0;
  } else if (a->address < b->address) {
    result = -1;
  } else {
    result = 1;
  }

  return result;
}

/*--------------------*/

static void MapFile__iterateOverActive (
                             in MapFile__DescriptorHandlerProc operation)
{
  UINT8 i;

  for (i = 0;  i < MapFile__maxCount;  i++) {
    MapFile__Descriptor *descriptor = &MapFile__list[i];

    if (descriptor->isInUse) {
      operation(descriptor);
    }
  }
}

/*--------------------*/

static void MapFile__openFile (in MapFile__Descriptor *descriptor)
{
  String_Type mapFileName = String_make();
  String_copy(&mapFileName, MapFile__tempString);
  String_append(&mapFileName, descriptor->suffix);
  File_open(&descriptor->file, mapFileName, File_Mode_write);
  String_destroy(&mapFileName);
}

/*--------------------*/

static void MapFile__writeHeaderLines (inout File_Type *file,
				       in char *linePrefix,
				       in char *columnSeparator,
				       in UINT8 columnCount,
				       in char *headingList[],
				       in UINT8 columnWidthList[])
  /** writes a line of <columnCount> left-justified column headings
      given in <headingList> to <file> where columns are separated by
      <columnSeparator>; the column widths are given in
      <columnWidthList>; this heading line is followed by an
      appropriate line of dashes; each line is introduced by
      <linePrefix> which is typically an empty string */
{
  String_Type currentLine = String_make();
  UINT8 lineNumber;
  String_Type st = String_make();

  File_writeChar(file, '\n');

  for (lineNumber = 1;  lineNumber <= 2;  lineNumber++) {
    UINT8 i;

    String_copyCharArray(&currentLine, linePrefix);

    for (i = 0;  i < columnCount;  i++) {
      char *separator = (i == columnCount - 1 ? "\n" : columnSeparator);
      char fillChar = (lineNumber != 1 ? '-'
		       : (i < columnCount - 1 ? ' ' : String_terminator));
      char *word = (lineNumber == 1 ? headingList[i] : "");

      String_copyCharArrayAligned(&st, columnWidthList[i], word, 
				  fillChar, true);
      String_appendCharArray(&st, separator);
      String_append(&currentLine, st);
    }

    File_writeString(file, currentLine);
  }

  String_destroy(&st);
  String_destroy(&currentLine);
}

/*--------------------*/

static void MapFile__writeMapFileData (in MapFile__Descriptor *descriptor)
  /** puts out symbol table to map file when output procedure is
      defined */
{
  MapFile_SymbolTableOutputProc symbolTableOutputProc =
    descriptor->routines.symbolTableOutputProc;

  if (symbolTableOutputProc != NULL) {
    symbolTableOutputProc(&descriptor->file);
  }
}

/*--------------------*/

static void MapFile__writeMessage (in MapFile__Descriptor *descriptor)
{
  File_writeCharArray(&descriptor->file, "\n?ASlink-Warning-");
  File_writeString(&descriptor->file, MapFile__tempString);
}

/*--------------------*/

static void MapFile__writeSpecialComment (in MapFile__Descriptor *descriptor)
  /** puts out special comment in <tempString> to map file when
      relevant */
{
  MapFile_CommentOutputProc commentOutputProc =
    descriptor->routines.commentOutputProc;

  if (commentOutputProc != NULL) {
    commentOutputProc(&descriptor->file, MapFile__tempString);
  }
}

/*--------------------*/

static void MapFile__writeStandardAreaInfo (inout File_Type *file,
					    in Area_Type area)
  /** writes canonical information for <area> to <file> */
{
  UINT8 areaMemoryPage;
  String_Type currentLine = String_make();
  UINT8 headerColumnCount = 5;
  UINT8 headerColumnWidthList[] = { 35, 6, 6, 14, 15 };
  char *headingList[] = { "Area", "Addr", "Size", "Decimal Bytes", 
		      "(Attributes)" };
  UINT8 symbolLineColumnCount = 2;
  UINT8 memPageColumnWidth = 3;
  UINT8 symbolLineColumnWidthList[] = { 8, 50 };
  char *symbolLineHeadingList[] = { "Value", "Global" };
  char *symbolLinePrefix = "  ";
  String_Type st = String_make();

  /* - - - - - - - - - -*/
  /* write header lines */
  /* - - - - - - - - - -*/

  MapFile__writeHeaderLines(file, "", " ", headerColumnCount, headingList,
			    headerColumnWidthList);

  /* - - - - - - - - - - - - - - - - - -*/
  /* write area header information line */
  /* - - - - - - - - - - - - - - - - - -*/
  {
    Target_Address areaAddress;
    Area_AttributeSet areaAttributes;
    Boolean areaIsPaged;
    String_Type areaName = String_make();
    Target_Address areaSize;
    char *temp;

    Area_getName(area, &areaName);
    areaMemoryPage = Area_getMemoryPage(area);
    areaAddress    = Area_getAddress(area);
    areaSize       = Area_getSize(area);
    areaAttributes = Area_getAttributes(area);
    areaIsPaged    = 
      Set_isElement(areaAttributes, Area_Attribute_hasPagedSegments);

    String_clear(&currentLine);
    String_copyAligned(&st, headerColumnWidthList[0], areaName, ' ', true);
    String_appendChar(&st, ' ');
    String_append(&currentLine, st);

    String_copyIntegerAligned(&st, headerColumnWidthList[1], areaAddress, 
			      MapFile__base, ' ', false);
    String_appendChar(&st, ' ');
    String_append(&currentLine, st);

    String_copyIntegerAligned(&st, headerColumnWidthList[2], areaSize,
			      MapFile__base, ' ', false);
    String_appendChar(&st, ' ');
    String_append(&currentLine, st);

    String_copyIntegerAligned(&st, 6, areaSize, 10, ' ', false);
    String_prependCharArray(&st, "= ");
    String_appendCharArray(&st, " bytes");
    String_copyAligned(&st, headerColumnWidthList[3], st, ' ', true);
    String_appendChar(&st, ' ');
    String_append(&currentLine, st);

    temp = (Set_isElement(areaAttributes, Area_Attribute_isAbsolute)
	    ? "(ABS" : "(REL");
    String_copyCharArray(&st, temp);
    temp = (Set_isElement(areaAttributes, Area_Attribute_hasOverlayedSegments)
	    ? ",OVR" : ",CON");
    String_appendCharArray(&st, temp);
    temp = (areaIsPaged ? ",PAG" : "");
    String_appendCharArray(&st, temp);
    String_appendChar(&st, ')');

    if (areaIsPaged) {
      Boolean addressIsBad = ((areaAddress & 0xFF) != 0);
      Boolean sizeIsBad = (areaSize > 256);

      if (addressIsBad || sizeIsBad) {
	String_appendCharArray(&st, "  ");

	if (addressIsBad) {
	  String_appendCharArray(&st, " Boundary");
	}

	if (addressIsBad && sizeIsBad) {
	  String_appendCharArray(&st, " /");
	}

	if (sizeIsBad) {
	  String_appendCharArray(&st, " Length");
	}

	String_appendCharArray(&st, " Error"); 
      }
    }

    String_append(&currentLine, st);
    String_appendChar(&currentLine, '\n');
    File_writeString(file, currentLine);
    String_destroy(&areaName);
  }

  /* - - - - - - - - - - - - */
  /* output list of symbols */
  /* - - - - - - - - - - - - */
  {
    Symbol_List areaSymbolList = List_make(Symbol_typeDescriptor);
    List_Cursor symbolCursor;

    MapFile_getSortedAreaSymbolList (area, &areaSymbolList);
    File_writeChar(file, '\n');
    MapFile__writeHeaderLines(file, symbolLinePrefix, " ",
			      symbolLineColumnCount, symbolLineHeadingList,
			      symbolLineColumnWidthList);

    for (symbolCursor = List_resetCursor(areaSymbolList);
	 symbolCursor != NULL;
	 List_advanceCursor(&symbolCursor)) {
      UINT8 columnWidth;
      Symbol_Type symbol = List_getElementAtCursor(symbolCursor);
      Target_Address address = Symbol_absoluteAddress(symbol);
      String_Type symbolName = String_make();

      Symbol_getName(symbol, &symbolName);

      String_copyCharArray(&currentLine, symbolLinePrefix);

      if (areaMemoryPage == 0) {
	String_copyCharArrayAligned(&st, memPageColumnWidth, "", ' ', false);
      } else {
	String_copyIntegerAligned(&st, memPageColumnWidth - 1,
				  areaMemoryPage, MapFile__base, '0', false);
	String_appendChar(&st, ':');
      }

      String_append(&currentLine, st);
      columnWidth = symbolLineColumnWidthList[0] - memPageColumnWidth;
      String_copyIntegerAligned(&st, columnWidth,
				address, MapFile__base, ' ', false);
      String_append(&currentLine, st);
      String_appendChar(&currentLine, ' ');

      String_copyAligned(&st, symbolLineColumnWidthList[1], symbolName,
			 String_terminator, true);
      String_append(&currentLine, st);
      String_appendChar(&currentLine, '\n');

      File_writeString(file, currentLine);
      String_destroy(&symbolName);
    }
  }
}

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void MapFile_initialize (void)
{
  UINT8 i;

  MapFile__count = 0;
  MapFile__isOpen = false;
  MapFile__tempString = String_make();
  MapFile__linkFileList = StringList_make();

  for (i = 0;  i < MapFile__maxCount;  i++) {
    MapFile__Descriptor *descriptor = &MapFile__list[i];
    descriptor->isInUse = false;
    descriptor->suffix = String_make();
    descriptor->routines.commentOutputProc = NULL;
    descriptor->routines.symbolTableOutputProc = NULL;
  }
}

/*--------------------*/

void MapFile_finalize (void)
{
  UINT8 i;

  for (i = 0;  i < MapFile__maxCount;  i++) {
    MapFile__Descriptor *descriptor = &MapFile__list[i];
    String_destroy(&descriptor->suffix);
  }

  List_destroy(&MapFile__linkFileList);
  String_destroy(&MapFile__tempString);
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

Boolean MapFile_isOpen (void)
{
  return (MapFile__isOpen);
}

/*--------------------*/

void MapFile_getSortedAreaSymbolList (in Area_Type area, 
				      out Symbol_List *areaSymbolList)
{
  UINT16 i;
  char *procName = "MapFile_getSortedAreaSymbolList";
  MapFile__SymbolWithAddress *symbolArray;
  UINT16 symbolCount;
  Symbol_List tempSymbolList = List_make(Symbol_typeDescriptor);

  /* concatenate all symbols from the segments into
     <tempSymbolList> */
  {
    Symbol_List segmentSymbolList = List_make(Symbol_typeDescriptor);
    List_Cursor segmentCursor;
    Area_SegmentList segmentList = List_make(Area_segmentTypeDescriptor);

    Area_getListOfSegments(area, &segmentList);

    for (segmentCursor = List_resetCursor(segmentList);
	 segmentCursor != NULL;
	 List_advanceCursor(&segmentCursor)) {
      Area_Segment segment = List_getElementAtCursor(segmentCursor);
      Area_getSegmentSymbols(segment, &segmentSymbolList);
      List_concatenate(&tempSymbolList, segmentSymbolList);
    }

    List_destroy(&segmentList);
    List_destroy(&segmentSymbolList);
  }

  /* store all symbols from <tempSymbolList> in temporary array and
     sort them in ascending order of addresses */
  {
    List_Cursor symbolCursor;

    symbolCount = List_length(tempSymbolList);
    symbolArray = NEWARRAY(MapFile__SymbolWithAddress, symbolCount);
    ASSERTION(symbolArray != NULL, procName, "out of memory");

    i = 0;

    for (symbolCursor = List_resetCursor(tempSymbolList);
	 symbolCursor != NULL;
	 List_advanceCursor(&symbolCursor)) {
      Symbol_Type symbol = List_getElementAtCursor(symbolCursor);

      symbolArray[i].symbol = symbol;
      symbolArray[i].address = Symbol_absoluteAddress(symbol);
      i++;
    }
  
    StdLib_qsort(symbolArray, symbolCount, sizeof(MapFile__SymbolWithAddress),
		 MapFile__compareSymbolsByAddress);
  }

  /* transfer sorted symbols into <areaSymbolList> */
  {
    List_clear(areaSymbolList);

    for (i = 0;  i < symbolCount;  i++) {
      Symbol_Type symbol = symbolArray[i].symbol;
      Object *objectPtr = List_append(areaSymbolList);
      *objectPtr = symbol;
    }

    DESTROY(symbolArray);
  }
    
  List_destroy(&tempSymbolList);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void MapFile_openAll (in String_Type fileNamePrefix)
{
  MapFile__isOpen = true;
  String_copy(&MapFile__tempString, fileNamePrefix);
  MapFile__iterateOverActive(MapFile__openFile);
}

/*--------------------*/

void MapFile_closeAll (void)
{
  MapFile__iterateOverActive(MapFile__closeFile);
  MapFile__isOpen = false;
  MapFile__count = 0;
}

/*--------------------*/

void MapFile_registerForOutput (in String_Type fileNameSuffix,
				in MapFile_ProcDescriptor routines)
{
  if (MapFile__count >= MapFile__maxCount) {
    Error_raise(Error_Criticality_fatalError,
		"too many map files open simultaneously");
  } else {
    MapFile__Descriptor *descriptor = &MapFile__list[MapFile__count];
    UINT8 i;
    Boolean isFound = false;

    /* check whether an entry with that suffix already exists */
    for (i = 0;  i < MapFile__count;  i++) {
      MapFile__Descriptor *entry = &MapFile__list[i];
      
      if (entry->isInUse) {
	if (String_isEqual(entry->suffix, fileNameSuffix)) {
	  isFound = true;
	  break;
	}
      }
    }

    if (isFound) {
      Error_raise(Error_Criticality_warning,
		  "ignored duplicate map file request for ",
		  String_asCharPointer(fileNameSuffix));
    } else {
      String_copy(&descriptor->suffix, fileNameSuffix);
      descriptor->routines = routines;
      descriptor->isInUse = true;
      MapFile__count++;
    }
  }
}

/*--------------------*/

void MapFile_setOptions (in UINT8 base, in StringList_Type linkFileList)
{
  MapFile__base = base;
  List_copy(&MapFile__linkFileList, linkFileList);
}

/*--------------------*/

void MapFile_writeErrorMessage (in String_Type message)
{
  String_copy(&MapFile__tempString, message);
  MapFile__iterateOverActive(MapFile__writeMessage);
}

/*--------------------*/

void MapFile_writeSpecialComment (in String_Type comment)
{
  String_copy(&MapFile__tempString, comment);
  MapFile__iterateOverActive(MapFile__writeSpecialComment);
}

/*--------------------*/

void MapFile_writeLinkingData (void)
{
  MapFile__iterateOverActive(MapFile__writeMapFileData);
}

/*--------------------*/

void MapFile_generateStandardFile (inout File_Type *file)
{
  String_Type st = String_make();

  if (MapFile__base == 16) {
    File_writeCharArray(file, "Hexadecimal\n\n");
  } else if (MapFile__base == 8) {
    File_writeCharArray(file, "Octal\n\n");
  } else if (MapFile__base == 10) {
    File_writeCharArray(file, "Decimal\n\n");
  }

  /*.......................*/
  /* output map area lists */
  /*.......................*/
  {
    List_Cursor areaCursor;
    Area_List areaList = List_make(Area_typeDescriptor);

    Area_getList(&areaList);

    for (areaCursor = List_resetCursor(areaList);  areaCursor != NULL;
	 List_advanceCursor(&areaCursor)) {
      Area_Type area = List_getElementAtCursor(areaCursor);
      MapFile__writeStandardAreaInfo(file, area);
    }

    List_destroy(&areaList);
  }
    
  /*.....................*/
  /* output linked files */
  /*.....................*/
  {
    UINT8 columnCount = 2;
    UINT8 columnWidthList[] = {32, 55 };
    char *headingList[] = { "Files Linked", "[ module(s) ]" };
    String_Type fileName = String_make();
    List_Cursor fileNameCursor;
    Multimap_Type fileNameToModuleMap = Multimap_make(String_typeDescriptor);
    List_Cursor moduleCursor;
    List_Type moduleList = List_make(Module_typeDescriptor);
    UINT8 moduleNamesPerLine = 3;
    UINT8 moduleNameColumnWidth = 16;

    /* construct a multimap from link file name to modules in that file */
    Module_getModuleList(&moduleList);

    for (moduleCursor = List_resetCursor(moduleList);
	 moduleCursor != NULL;
	 List_advanceCursor(&moduleCursor)) {
      Module_Type module = List_getElementAtCursor(moduleCursor);
      Module_getFileName(module, &fileName);
      Multimap_add(&fileNameToModuleMap, fileName, module);
    }

    /* output files with the associated modules */
    File_writeChar(file, '\n');
    MapFile__writeHeaderLines(file, "", "", columnCount, headingList,
			      columnWidthList);

    for (fileNameCursor = List_resetCursor(MapFile__linkFileList);
	 fileNameCursor != NULL;
	 List_advanceCursor(&fileNameCursor)) {
      String_Type linkFileName = List_getElementAtCursor(fileNameCursor);
      List_Type fileModuleList = Multimap_lookup(fileNameToModuleMap,
						 linkFileName);
      Boolean hasModules = (List_length(fileModuleList) > 0);
      UINT8 moduleIndex = 1;
      String_Type moduleName = String_make();
      String_copyAligned(&st, columnWidthList[0], linkFileName, ' ', true);
      File_writeString(file, st);

      if (hasModules) {
	File_writeCharArray(file, "[ ");
      }

      for (moduleCursor = List_resetCursor(fileModuleList);
	   moduleCursor != NULL;
	   List_advanceCursor(&moduleCursor)) {
	Module_Type module = List_getElementAtCursor(moduleCursor);
	Module_getName(module, &moduleName);

	if (moduleIndex > 1) {
	  File_writeCharArray(file, ",");

	  if (moduleIndex % moduleNamesPerLine != 1) {
	    File_writeChar(file, ' ');
	  } else {
	    File_writeChar(file, '\n');
	    String_copyCharArrayAligned(&st, columnWidthList[0] + 2, "",
					' ', true);
	    File_writeString(file, st);
	  }
	}

	String_copyAligned(&moduleName, moduleNameColumnWidth, moduleName,
			   String_terminator, true);
	File_writeString(file, moduleName);
      }

      if (hasModules) {
	File_writeCharArray(file, " ]");
      }

      File_writeChar(file, '\n');
      String_destroy(&moduleName);
    }

    List_destroy(&moduleList);
    Multimap_destroy(&fileNameToModuleMap);
    String_destroy(&fileName);
  }

  /*.........................*/
  /* output linked libraries */
  /*.........................*/
  {
    UINT8 columnCount = 2;
    UINT8 columnWidthList[] = {32, 55 };
    char *headingList[] = { "Libraries Linked", "[ object file     ]" };
    List_Cursor fileNameCursor;
    StringList_Type libraryFileNameList = StringList_make();
    String_Type libraryPath = String_make();
    String_Type libraryBaseName = String_make();

    Library_getFileNameList(&libraryFileNameList);
    File_writeChar(file, '\n');
    MapFile__writeHeaderLines(file, "", "", columnCount, headingList,
			      columnWidthList);

    for (fileNameCursor = List_resetCursor(libraryFileNameList);
	 fileNameCursor != NULL;
	 List_advanceCursor(&fileNameCursor)) {
      String_Type libraryFileName = List_getElementAtCursor(fileNameCursor);
      SizeType separatorPosition =
	String_findFromEnd(libraryFileName, File_directorySeparator);

      if (separatorPosition == String_notFound) {
	String_clear(&libraryPath);
	String_copy(&libraryBaseName, libraryFileName);
      } else {
	String_getSubstring(&libraryPath, libraryFileName, 1,
			    separatorPosition);
	String_getSubstring(&libraryBaseName, libraryFileName,
			    separatorPosition + 1, 1000);
      }

      {
	/* when path is too long, cut it on left side and replace it by
	   dots */
	UINT8 columnWidth = columnWidthList[0];

	if (String_length(libraryPath) <= columnWidth) {
	  String_copyAligned(&st, columnWidth, libraryPath, ' ', true);
	} else {
	  String_copyAligned(&st, columnWidth - 3, libraryPath, ' ', false);
	  String_prependCharArray(&st, "...");
	}
      }

      File_writeString(file, st);
      File_writeCharArray(file, "  ");
      String_copyAligned(&st, columnWidthList[1], libraryBaseName,
			 String_terminator, true);
      File_writeString(file, st);
      File_writeChar(file, '\n');
    }

    String_destroy(&libraryPath);
    String_destroy(&libraryBaseName);
    List_destroy(&libraryFileNameList);
  }

  /*.................................*/
  /* output base address definitions */
  /*.................................*/
  if (List_length(StringTable_baseAddressList) > 0) {
    File_writeCharArray(file, "\nUser Base Address Definitions\n\n");
    StringList_write(StringTable_baseAddressList, file, String_newline);
  }
	
  /*...........................*/
  /* output global definitions */
  /*...........................*/
  if (List_length(StringTable_globalDefList) > 0) {
    File_writeCharArray(file, "\nUser Global Definitions\n\n");
    StringList_write(StringTable_globalDefList, file, String_newline);
  }

  File_writeCharArray(file, "\n\f");

  /*..........................*/
  /* output undefined symbols */
  /*..........................*/
  Symbol_checkForUndefinedSymbols(file);

  String_destroy(&st);
}
