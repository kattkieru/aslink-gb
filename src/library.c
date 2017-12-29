/** Library module --
    Implementation of module providing all services for object file
    libraries in the generic SDCC linker.

    Original version by Thomas Tensi, 2008-01
    based on the module lklibr.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/


#include "library.h"

/*========================================*/

#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "list.h"
#include "multimap.h"
#include "parser.h"
#include "string.h"
#include "stringlist.h"
#include "symbol.h"

/*========================================*/

#define Library__magicNumber 0x33335678


typedef enum { Library__LoadStatus_notLoaded, Library__LoadStatus_marked,
	       Library__LoadStatus_loaded } Library__LoadStatus;
  /** the status of a library file: it may be not loaded (because no
      reference to it exists), marked for load (not yet loaded, but
      referenced) and loaded */


typedef struct {
  UINT32 magicNumber;
  Boolean isObjectFile;
  Library__LoadStatus loadStatus;
  String_Type path;
  SizeType offset;
  String_Type directoryPath;
  StringList_Type symbolNameList;
} Library__Record;
  /** record containing the information about some library: whether it
      is an plain object file (without further structure), is loaded,
      its full path name, its directory path and the list of symbol
      names contained in that library; when <offset> is not zero, this
      means that the information starts in file at given offset */


typedef Library__Record *Library__Type;


static List_Type Library__list;
  /** list of all libraries encountered so far */

static Multimap_Type Library__symbolIndex;
  /** mapping from symbol name to libraries containing that symbol */

static StringList_Type Library__pathList;
  /** list of all paths used for library search */


static String_Type Library__fileExtension;
  /** extension of library file */

static String_Type Library__objectFileExtension;
  /** extension of object file in library */

static String_Type Library__indexStartKeyword;
static String_Type Library__indexEndKeyword;
static String_Type Library__libStartKeyword;
static String_Type Library__libEndKeyword;
static String_Type Library__moduleStartKeyword;
static String_Type Library__moduleEndKeyword;
  /** list of keywords in inlined object files */


typedef enum { Library__ParseState_atFileSpecification,
	       Library__ParseState_inSdccLib,
	       Library__ParseState_afterIndexStart,
	       Library__ParseState_inIndex,
	       Library__ParseState_afterModuleStart,
	       Library__ParseState_inModule,
	       Library__ParseState_done, Library__ParseState_inError
} Library__ParseState;
  /** state of parsing single lines in a library file */

/*--------------------*/

static Object Library__make (void);
static Boolean Library__hasPath (in Object object, in Object key);

static TypeDescriptor_Record Library__tdRecord =
  { /* .objectSize = */ sizeof(Library__Record), /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL, /* .constructionProc = */ Library__make,
    /* .destructionProc = */ NULL, /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ Library__hasPath };

static TypeDescriptor_Type Library__typeDescriptor = &Library__tdRecord;
  /** variable used for describing the type properties when library
      records occur in generic types like lists */


/*========================================*/
/*           INTERNAL ROUTINES            */
/*========================================*/

static void Library__ensureSuffix (inout String_Type *st,
				   in String_Type suffix);
static void Library__handleFileLine (inout Library__Type *library,
				     in String_Type libraryFileLine,
				     inout Library__ParseState *state);
static void Library__processFileReference (inout Library__Type *library);
static void Library__removeBaseName (inout String_Type *path);

/*--------------------*/

static Library__Type Library__attemptConversion (in Object library)
  /** verifies that <library> is really a library; if not, the program
      stops with an error message */
{
  return attemptConversion("Library__Type", library, Library__magicNumber);
}

/*--------------------*/

static Library__Type Library__addFile (in String_Type path,
				       in String_Type relativePath,
				       in SizeType offset,
				       in Boolean isObjectFile)
  /** checks whether library file with <relativePath> can be found in
      <path>; if yes, it is added to library list and <isFound> is set
      to true;  returns library when found or NULL */
{
  String_Type fileName = String_make();
  Library__Type library;
  Boolean isFound;

  String_copy(&fileName, path);
  Library__ensureSuffix(&fileName, File_directorySeparator);
  String_append(&fileName, relativePath);
  isFound = File_exists(fileName);

  if (!isFound) {
    library = NULL;
  } else {
    // TODO: this lookup does currently not work for embedded
    //       libraries...
    library = List_lookup(Library__list, path);

    if (library != NULL) {
      /* there is already an entry ==> skip */
    } else {
      Object *objectPtr = List_append(&Library__list);
      library = Library__attemptConversion(*objectPtr);
      library->isObjectFile = isObjectFile;
      String_copy(&library->directoryPath, fileName);
      Library__removeBaseName(&library->directoryPath);
      String_copy(&library->path, fileName);
      library->offset = offset;
    }
  }

  String_destroy(&fileName);
  return library;
}

/*--------------------*/

static void Library__buildIndex (void)
 /** build an in-memory cache of all the symbols contained in all
     libraries */
{
  List_Cursor libraryCursor;

  Library__symbolIndex = Multimap_make(String_typeDescriptor);

  /* iterate through all library files */
  for (libraryCursor = List_resetCursor(Library__list);
       libraryCursor != NULL;
       List_advanceCursor(&libraryCursor)) {

    Library__Type library =
      Library__attemptConversion(List_getElementAtCursor(libraryCursor));

    if (!library->isObjectFile) {
      String_Type filePath = String_make();
      File_Type libraryFile;
      Library__ParseState state = Library__ParseState_atFileSpecification;

      String_copy(&filePath, library->path);

      if (!File_open(&libraryFile, filePath, File_Mode_read)) {
	Error_raise(Error_Criticality_fatalError,
		    "cannot open library file %s", 
		    String_asCharPointer(filePath));
      } else {
	/* read lines from the library file specifying some object file
	   or some embedded index information */
	String_Type libraryFileLine = String_make();

	while (state != Library__ParseState_done) {
	  File_readLine(&libraryFile, &libraryFileLine);

	  if (String_length(libraryFileLine) == 0) {
	    /* end of file */
	    state = Library__ParseState_done;
	  } else {
	    String_removeTrailingCrLf(&libraryFileLine);
	    Library__handleFileLine(&library, libraryFileLine, &state);

	    if (state == Library__ParseState_inError) {
	      Error_raise(Error_Criticality_fatalError,
			  "bad line in library file %s: %s",
			  String_asCharPointer(filePath), 
			  String_asCharPointer(libraryFileLine));
	      state = Library__ParseState_done;
	    }
	  }
	}

	String_destroy(&libraryFileLine);
      }

      File_close(&libraryFile);
      String_destroy(&filePath);
    }
  }
}

/*--------------------*/

static void Library__ensureSuffix (inout String_Type *st,
				   in String_Type suffix)
  /** ensures that <st> has <suffix> */
{
  if (!String_hasSuffix(*st, suffix)) {
    String_append(st, suffix);
  }
}

/*--------------------*/

static Boolean Library__findSymbol (in Symbol_Type symbol)
  /** locates <symbol> in overall library list and when found, sets
      status of appropriate library file to "marked" (when not already
      loaded or marked) */
{
  String_Type symbolName = String_make();
  List_Type libraryList;
  Boolean isFound;

  Symbol_getName(symbol, &symbolName);
  libraryList = Multimap_lookup(Library__symbolIndex, symbolName);
  isFound = (libraryList != NULL);

  if (isFound) {
    Library__Type library;

    if (List_length(libraryList) > 1) {
      List_Cursor cursor;
      String_Type libraryNames = String_make();

      for (cursor = List_resetCursor(libraryList);  cursor != NULL;
	   List_advanceCursor(&cursor)) {
	library = List_getElementAtCursor(cursor);
	String_appendCharArray(&libraryNames, "\n  ");
	String_append(&libraryNames, library->directoryPath);
      }

      Error_raise(Error_Criticality_warning,
		  "definition of public symbol '%s' found more than once in%s",
		  String_asCharPointer(symbolName),
		  String_asCharPointer(libraryNames));
      String_destroy(&libraryNames);
    }

    /* take the first or only library in list */
    library = Library__attemptConversion(List_getElement(libraryList, 1));

    if (library->loadStatus == Library__LoadStatus_notLoaded) {
      library->loadStatus = Library__LoadStatus_marked;
    }
  }

  String_destroy(&symbolName);
  return isFound;
}

/*--------------------*/

static void Library__handleFileLine (inout Library__Type *parentLibrary,
				     in String_Type libraryFileLine,
				     inout Library__ParseState *state)
  /** processes a single line <libraryFileLine> of file for
      <parentLibrary> and changes <state> accordingly */
{
  SizeType blankPosition;
  Boolean isOkay;
  static long indexSize;
  static Library__Type embeddedLibrary;

  switch (*state) {
    case Library__ParseState_atFileSpecification:
      if (String_isEqual(libraryFileLine, Library__libStartKeyword)) {
	*state = Library__ParseState_inSdccLib;
      } else {
	/* the line specifies the name of an object file name */
	Library__ensureSuffix(&libraryFileLine, Library__objectFileExtension);
	embeddedLibrary = Library__addFile((*parentLibrary)->directoryPath,
					   libraryFileLine, 
					   0, true);
	if (embeddedLibrary != NULL) {
	  Library__processFileReference(&embeddedLibrary);
	} else {
	  Error_raise(Error_Criticality_warning,
		      "object file %s in library %s not found",
		      String_asCharPointer(libraryFileLine),
		      String_asCharPointer((*parentLibrary)->path));
	}
      }

      break;
	
    case Library__ParseState_inSdccLib:
      if (String_isEqual(libraryFileLine, Library__indexStartKeyword)) {
	*state = Library__ParseState_afterIndexStart;
      } else {
	*state = Library__ParseState_inError;
      }
      
      break;

    case Library__ParseState_afterIndexStart:
      /* this line contains the size of the index */
      indexSize = 0;
      isOkay = String_convertToLong(libraryFileLine, 10, &indexSize);
      *state = (isOkay ? Library__ParseState_inIndex
		: Library__ParseState_inError);
      break;

    case Library__ParseState_inIndex:
      if (String_isEqual(libraryFileLine, Library__moduleStartKeyword)) {
	*state = Library__ParseState_afterModuleStart;
      } else if (String_isEqual(libraryFileLine, Library__indexEndKeyword)) {
	*state = Library__ParseState_done;
      } else {
	*state = Library__ParseState_inError;
      }
      
      break;

    case Library__ParseState_afterModuleStart:
      /* this line contains the module name and the file offset
	 relative to index */
      blankPosition = String_findCharacter(libraryFileLine, ' ');

      if (blankPosition == String_notFound) {
	*state = Library__ParseState_inError;
      } else {
	String_Type moduleName = String_make();
	long moduleOffset = 0;
	String_Type moduleOffsetString = String_make();

	String_getSubstring(&moduleName, libraryFileLine,
			    1, blankPosition - 1);
	String_getSubstring(&moduleOffsetString, libraryFileLine,
			    blankPosition + 1, 1000);
      	isOkay = String_convertToLong(moduleOffsetString, 10, &moduleOffset);

	if (isOkay) {
	  /* update entry for embedded library: it is the same as the
	     parent except for an nonzero offset */
	  embeddedLibrary = Library__addFile((*parentLibrary)->directoryPath,
					     String_emptyString, 
					     indexSize + moduleOffset,
					     true);
	  isOkay = (embeddedLibrary != NULL);
	}

	*state = (isOkay ? Library__ParseState_inModule
		  : Library__ParseState_inError);
	String_destroy(&moduleOffsetString);
	String_destroy(&moduleName);
      }
      
      break;

    case Library__ParseState_inModule:
      if (!String_isEqual(libraryFileLine, Library__moduleEndKeyword)) {
	/* add some object */
	// TODO: check whether complete library has to be loaded or
	//       only some segment
	String_Type symbolName = libraryFileLine;
	StringList_append(&embeddedLibrary->symbolNameList, symbolName);
	Multimap_add(&Library__symbolIndex, symbolName, embeddedLibrary);
      } else {
	*state = Library__ParseState_inIndex;
	embeddedLibrary = NULL;
      }

      break;
  }
}

/*--------------------*/

static Boolean Library__hasPath (in Object object, in Object key)
  /** checks whether <object> has <key> as path name identification */
{
  Library__Type library = Library__attemptConversion(object);
  String_Type pathName = key;
  return String_isEqual(library->path, pathName);
}

/*--------------------*/

static void Library__load (inout Library__Type *library)
{
  Library__Type currentLibrary = *library;

  if (currentLibrary->offset > 0) {
    File_writeCharArray(&File_stderr, "loading library: ");
    File_writeString(&File_stderr, currentLibrary->path);
    File_writeCharArray(&File_stderr, " embedded at ");
    File_writeHex(&File_stderr, currentLibrary->offset, 8);
    File_writeChar(&File_stderr, '\n');

    // TODO: embedded libraries: construct an appropriate file name
    // (with the '@' convention and have it parsed by Parser module
    Error_raise(Error_Criticality_fatalError,
		"embedded library not supported");
  }

  Parser_parseObjectFile(true, currentLibrary->path);
  currentLibrary->loadStatus = Library__LoadStatus_loaded;
}

/*--------------------*/

static Object Library__make (void)
  /** construct new library and return it as object */
{
  Library__Type library = NEW(Library__Record);

  library->magicNumber    = Library__magicNumber;
  library->loadStatus     = Library__LoadStatus_notLoaded;
  library->path           = String_make();
  library->symbolNameList = StringList_make();
  library->directoryPath  = String_make();

  return library;
}

/*--------------------*/

static void Library__processFileReference (inout Library__Type *library)
  /** add defined symbols from object file specified by <library> to
      that library symbol list */
{
  Library__Type currentLibrary = *library;
  List_Cursor cursor;
  StringList_Type symbolNameList = StringList_make();

  Parser_collectSymbolDefinitions(currentLibrary->path, &symbolNameList);
  List_concatenate(&currentLibrary->symbolNameList, symbolNameList);

  /* update the multimap of symbols */
  for (cursor = List_resetCursor(symbolNameList);
       cursor != NULL;
       List_advanceCursor(&cursor)) {
    String_Type symbolName = List_getElementAtCursor(cursor);
    Multimap_add(&Library__symbolIndex, symbolName, currentLibrary);
  }

  List_destroy(&symbolNameList);
}

/*--------------------*/

static void Library__removeBaseName (inout String_Type *path)
  /** removes the base name from a path specification in <path> */
{
  SizeType separatorPosition =
    String_findFromEnd(*path, File_directorySeparator);

  if (separatorPosition == String_notFound) {
    String_clear(path);
  } else {
    String_getSubstring(path, *path, 1, separatorPosition);
  }
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Library_initialize (void)
{
  Library__list = List_make(Library__typeDescriptor);
  Library__pathList = StringList_make();

  Library__fileExtension       = String_makeFromCharArray(".lib");
  Library__objectFileExtension = String_makeFromCharArray(".o");
  Library__indexStartKeyword   = String_makeFromCharArray("<INDEX>");
  Library__indexEndKeyword     = String_makeFromCharArray("</INDEX>");
  Library__libStartKeyword     = String_makeFromCharArray("<SDCCLIB>");
  Library__libEndKeyword       = String_makeFromCharArray("</SDCCLIB>");
  Library__moduleStartKeyword  = String_makeFromCharArray("<MODULE>");
  Library__moduleEndKeyword    = String_makeFromCharArray("</MODULE>");
}

/*--------------------*/

void Library_finalize (void)
{
  List_destroy(&Library__list);
  List_destroy(&Library__pathList);
  String_destroy(&Library__fileExtension);
  String_destroy(&Library__objectFileExtension);
  String_destroy(&Library__indexStartKeyword);
  String_destroy(&Library__indexEndKeyword);
  String_destroy(&Library__libStartKeyword);
  String_destroy(&Library__libEndKeyword);
  String_destroy(&Library__moduleStartKeyword);
  String_destroy(&Library__moduleEndKeyword);
}


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Library_getFileNameList (out StringList_Type *libraryFileNameList)
{
  List_Cursor libraryCursor;

  List_clear(libraryFileNameList);

  for (libraryCursor = List_resetCursor(Library__list);
       libraryCursor != NULL;
       List_advanceCursor(&libraryCursor)) {
    Library__Type library = List_getElementAtCursor(libraryCursor);
    StringList_append(libraryFileNameList, library->path);
  }
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Library_addCodeSequences (void)
{
  List_Cursor libraryCursor;
  StringList_Type fileNameList = StringList_make();

  for (libraryCursor = List_resetCursor(Library__list);
       libraryCursor != NULL;
       List_advanceCursor(&libraryCursor)) {
    Library__Type library = List_getElementAtCursor(libraryCursor);

    if (library->loadStatus == Library__LoadStatus_loaded) {
      StringList_append(&fileNameList, library->path);
    }
  }

  /* this is pass 2 of the linkage */
  Parser_parseObjectFiles(false, fileNameList);
  List_destroy(&fileNameList);
}
/*--------------------*/

void Library_addDirectory (in String_Type path)
{
  StringList_append(&Library__pathList, path);
}

/*--------------------*/

void Library_addFilePathName (in String_Type path, out Boolean *isFound)
{
  SizeType libraryPathCount = List_length(Library__pathList);
  SizeType i;
  Library__Type library;

  if (!String_hasSuffix(path, Library__fileExtension)) {
    String_append(&path, Library__fileExtension);
  }

  /* first try directly without any prefixing library path */
  library = Library__addFile(String_emptyString, path, 0, false);
  *isFound = (library != NULL);

  for (i = 1;  i <= libraryPathCount;  i++) {
    String_Type directoryPath = String_make();
    String_Type directory = List_getElement(Library__pathList, i);

    String_copy(&directoryPath, directory);
    library = Library__addFile(directoryPath, path, 0, false);
    *isFound = *isFound || (library != NULL);
    String_destroy(&directoryPath);
  }
}


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

void Library_resolveUndefinedSymbols (void)
{
  Boolean someSymbolWasResolved = true;
  Boolean allSymbolsAreResolved = false;
  Symbol_List undefinedSymbolList = List_make(Symbol_typeDescriptor);

  Library__buildIndex();

  while (someSymbolWasResolved) {
    List_Cursor libraryCursor;

    someSymbolWasResolved = false;

    Symbol_getUndefinedSymbolList(&undefinedSymbolList);

    if (List_length(undefinedSymbolList) == 0) {
      allSymbolsAreResolved = true;
    } else {
      List_Cursor symbolCursor;

      for (symbolCursor = List_resetCursor(undefinedSymbolList);
	   symbolCursor != NULL;
	   List_advanceCursor(&symbolCursor)) {
	Symbol_Type symbol = List_getElementAtCursor(symbolCursor);
	Boolean isFound = Library__findSymbol(symbol);
	someSymbolWasResolved = someSymbolWasResolved || isFound;
      }
    }

    if (someSymbolWasResolved) {
      /* add all libraries marked for load */
      for (libraryCursor = List_resetCursor(Library__list);
	   libraryCursor != NULL;
	   List_advanceCursor(&libraryCursor)) {
	Library__Type library = List_getElementAtCursor(libraryCursor);
     
	if (library->loadStatus == Library__LoadStatus_marked) {
	  Library__load(&library);
	}
      }
    }
  }
}
