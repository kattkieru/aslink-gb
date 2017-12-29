/** Linker main module --
    This module coordinates all services for the generic SDCC linker.

    It contains the functions which
     - input the linker options, parameters, and specifications from
       the command line
     - performs a two pass link
     - produces the appropriate linked data output and/or
       link map file and/or relocated listing files.

   It globally initializes and finalizes all modules used.


   Original version by Thomas Tensi, 2008-05

   Based on the module lkmain.c by 
     Alan R. Baldwin, 721 Berkeley St., Kent, Ohio 44240
   with extensions by
     Pascal Felber

   NOTE: as a naming convention all file scope names have the module
   name as a prefix with a single underscore for externally visible
   names and two underscores for internal names
*/

/*====================*/
/* standard includes  */
/*====================*/

#include <ctype.h>
# define CType_isalpha isalpha
# define CType_isdigit isdigit
# define CType_toupper toupper
#include <stdio.h>
# define StdIO_fprintf fprintf
# define StdIO_stderr  stderr
#include <string.h>
# define STRING_findCharacter strchr

/*============================*/
/* program specific includes  */
/*============================*/

#include "area.h"
#include "banking.h"
#include "codeoutput.h"
#include "codesequence.h"
#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "library.h"
#include "list.h"
#include "listingupdater.h"
#include "map.h"
#include "mapfile.h"
#include "module.h"
#include "multimap.h"
#include "noicemapfile.h"
#include "parser.h"
#include "scanner.h"
#include "set.h"
#include "string.h"
#include "stringlist.h"
#include "stringtable.h"
#include "target.h"

/*====================*/

char *Main__usageHelpText[] = {
  "Startup:",
  "  -c                           Command line input",
  "  -f   file[LNK]               File input",
  "  -p   Prompt and echo of file[LNK] to stdout (default)",
  "  -n   No echo of file[LNK] to stdout",
  "Usage: [-Options] file [file ...]",
  "Librarys:",
  "  -k	Library path specification, one per -k",
  "  -l	Library file specification, one per -l",
  "Relocation:",
  "  -b   area base address = expression",
  "  -g   global symbol = expression",
  "Map format:",
  "  -m   Map output generated as file[MAP]",
  "  -x   Hexadecimal (default)",
  "  -d   Decimal",
  "  -q   Octal",
  "Banking:",
  "  -hfile  file specification containing assignments of modules to banks",
  "Output:",
  "  -i   Intel Hex as file[IHX]",
  "  -s   Motorola S19 as file[S19]",
  "  -j   Produce NoICE debug as file[NOI]",
  "List:",
  "  -u	Update listing file(s) with link data as file(s)[.RST]",
  "End:",
  "  -e   or null line terminates input",
  "",
  NULL
};


#define Main__optionCharacters (Main__extendedOptions Main__singleCharOptions)
  /** platform independent option characters (upper-case) */

#define Main__singleCharOptions "MXDQISUE"
  /** platform independent option characters which do not consume the
      rest of the argument */

#define Main__extendedOptions "KLHBG"
  /** platform independent option characters which consume the rest of
      the argument */

/*--------------------*/

static struct {
  Boolean linkFilesAreEchoed;
    /** tells that link command files are echoed to stdout */
  StringList_Type linkFileList;
  String_Type mainFileNamePrefix;  /** name of object file with main
				       program (with extension removed) */
  UINT8 radix;                     /** current radix used for numbers */
  Boolean ihxFileIsUsed;
  Boolean sRecordFileIsUsed;
  Boolean listingsAreAugmented;
} Main__options;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static void Main__addOptionsToList (in char *fileName, 
				    inout StringList_Type *stringList,
				    in Boolean linesAreEchoed)
  /** reads option lines from file with <fileName> and appends them to
      <stringList>; when <linesAreEchoed> all file lines are output to
      standard error */
{
  Boolean isOkay;
  File_Type linkFile;
  String_Type st = String_makeFromCharArray(fileName);

  isOkay = File_open(&linkFile, st, File_Mode_read);

  if (!isOkay) {
    Error_raise(Error_Criticality_fatalError,
		"could not open link option file %s", fileName);
  }

  for (;;) {
    SizeType stringLength; 

    File_readLine(&linkFile, &st);
    stringLength = String_length(st);

    if (stringLength == 0) {
      break;
    }

    if (linesAreEchoed) {
      File_writeString(&File_stderr, st);
    }

    /* strip off line terminating newline */
    String_deleteCharacters(&st, stringLength, 1);
    StringList_append(stringList, st);
  }

  File_close(&linkFile);
  String_destroy(&st);
}

/*--------------------*/

static void Main__collectOptions (in int argc, in char *argv[],
				  inout StringList_Type *argumentList)
  /** scans command line given by <argc> and <argv> for link file
      inclusions and combines all options from command line and link
      files into <argumentList> */
{
  int i;
  Boolean previousOptionWasFileFlag = false;

  List_clear(argumentList);

  /* collect all options relating to an option file */
  for (i = 1;  i < argc;  ++i) {
    char *thisArgument = argv[i];
    char firstCh = thisArgument[0];
    String_Type argument = String_makeFromCharArray(thisArgument);

    if (previousOptionWasFileFlag) {
      /* an -f option must be directly followed by a filename */
      Main__addOptionsToList(thisArgument, argumentList,
			     Main__options.linkFilesAreEchoed);
      previousOptionWasFileFlag = false;
    } else if (firstCh != '-') {
      /* this is a filename */
      StringList_append(argumentList, argument);
    } else {
      SizeType j = 1;
      Boolean isDone = false;

      do {
	char ch = thisArgument[j];

	if (ch == String_terminator || !CType_isalpha(ch)) {
	  isDone = true;
	} else {
	  ch = (char) CType_toupper(ch);

	  switch (ch) {
	    case 'C':
	      Main__addOptionsToList("stdin", argumentList, 
				     Main__options.linkFilesAreEchoed);
	      break;

	    case 'F':
	      previousOptionWasFileFlag = true;
	      break;

	    case 'N':
	    case 'P':
	      Main__options.linkFilesAreEchoed = (ch == 'P');
	      break;
	    default:
	      if (j == 1) {
		StringList_append(argumentList, argument);
	      }
	      isDone = true;
	  }
	}

	j++;

      } while (!isDone);
    }
    String_destroy(&argument);
  }
}

/*--------------------*/

static void Main__giveUsageInfo (void)
  /** outputs the linker name and version and a list of valid options
      to the stderr device */
{
  char **currentLine;
  String_Type st = String_make();
  String_Type targetUsageInfo = String_make();

  String_appendCharArray(&st, "\nASxxxx Linker \n\n");

  for (currentLine = Main__usageHelpText;  *currentLine != NULL;
       currentLine++) {
    String_appendCharArray(&st, *currentLine);
    String_appendCharArray(&st, "\n");
  }

  Target_info.giveUsageInfo(&targetUsageInfo);
  String_append(&st, targetUsageInfo);
  File_writeString(&File_stderr, st);

  String_destroy(&targetUsageInfo);
  String_destroy(&st);
}

/*--------------------*/

static Boolean Main__isLinkFileIntroCharacter (in char ch)
  /** tells whether <ch> may introduce a link file name */
{
  return (CType_isalpha(ch) || CType_isdigit(ch) || ch == '_');
}

/*--------------------*/

static void Main__processGlobalSymbolDefinitions (void)
  /** sets the addresses of several symbols to values from
      <StringTable.globalDefList> */
{
  Parser_setMappingFromList(StringTable_globalDefList,
		    (Parser_KeyValueMappingProc) Symbol_setAddressForName);
}


/*--------------------*/

static void Main__processOptions (in StringList_Type argumentList,
				  inout Boolean optionIsHandledList[])
  /** evaluates all command line or file linker directives and updates
      the appropriate variables */
{
  SizeType i;
  SizeType length = List_length(argumentList);
  Boolean isOkay = true;  /* tells that no problematic option has been
			     found */
  Parser_Options parserOptions = {10, unknown};

  for (i = 1;  i <= length;  i++) {
    Boolean *optionIsHandled = &optionIsHandledList[i];

    if (!*optionIsHandled) {
      String_Type argument = List_getElement(argumentList, i);
      char *arg = String_asCharPointer(argument);
      char firstChar = arg[0];

      if (Main__isLinkFileIntroCharacter(firstChar)) {
	/* process link file name */
	if (String_length(Main__options.mainFileNamePrefix) > 0) {
	  StringList_append(&Main__options.linkFileList, argument);
	} else {
	  /* this is the first object file name which defines the
	     name of the executable */
	  SizeType dotPosition;
	  dotPosition = String_findCharacterFromEnd(argument, '.');

	  if (dotPosition == String_notFound) {
	    dotPosition = String_length(argument) + 1;
	  }

	  String_getSubstring(&Main__options.mainFileNamePrefix, argument,
			      1, dotPosition - 1);
	}
	*optionIsHandled = true;
      } else if (firstChar == '-') {
	char *argPtr = &arg[1];
	char ch = *argPtr++;

	if (ch == String_terminator) {
	  Error_raise(Error_Criticality_warning,
		      "plain '-' option on command line ignored");
	  isOkay = false;
	} else if (CType_isalpha(ch)) {
	  ch = (char) CType_toupper(ch);
	    
	  if (STRING_findCharacter(Main__optionCharacters, ch) != NULL) {
	    *optionIsHandled = true;

	    if (STRING_findCharacter(Main__singleCharOptions, ch) == NULL) {
	      /* handle character string option */
	      String_Type st = String_makeFromCharArray(argPtr);
	      Boolean libraryIsFound;

	      if (ch == 'B') {
		Parser_setMappingFromString(st,
		    (Parser_KeyValueMappingProc) Area_setBaseAddresses);
	      } else if (ch == 'G') {
		Parser_setMappingFromString(st,
		    (Parser_KeyValueMappingProc) Symbol_setAddressForName);
	      } else if (ch == 'H') {
		Banking_readConfigurationFile(st);
	      } else if (ch == 'K') {
		Library_addDirectory(st);
	      } else if (ch == 'L') { 
		Library_addFilePathName(st, &libraryIsFound);

		if (!libraryIsFound) {
		  Error_raise(Error_Criticality_warning,
			      "couldn't find library '%s'",
			      String_asCharPointer(st));
		}
	      } else {
		Error_raise(Error_Criticality_error,
			    "unknown character string option %s", arg);
	      }

	      String_destroy(&st);
	    } else {
	      /* handle single char option */
	      if (ch == 'D') {
		Main__options.radix = 10;
	      } else if (ch == 'E') {
		/* ignore all remaining options and set them handled */
		while (i <= length) {
		  optionIsHandledList[i++] = true;
		}
	      } else if (ch == 'I') {
		/* select intel format as output */
		Main__options.ihxFileIsUsed = true;
	      } else if (ch == 'J') {
		/* activate NOICE debug map file */
		MapFile_ProcDescriptor routines =
		  { NoICEMapFile_addSpecialComment, NoICEMapFile_generate };
		String_Type suffix = String_makeFromCharArray(".noi");
		MapFile_registerForOutput(suffix, routines);
		String_destroy(&suffix);
	      } else if (ch == 'M') {
		/* produce standard map file */
		MapFile_ProcDescriptor routines =
		  { NULL, MapFile_generateStandardFile };
		String_Type suffix = String_makeFromCharArray(".map");
		MapFile_registerForOutput(suffix, routines);
		String_destroy(&suffix);
	      } else if (ch == 'Q') {
		Main__options.radix = 8;
	      } else if (ch == 'S') {
		/* select motorola format as output */
		Main__options.sRecordFileIsUsed = true;
	      } else if (ch == 'U') {
		Main__options.listingsAreAugmented = true;
	      } else if (ch == 'X') {
		Main__options.radix = 16;
	      } else {
		Error_raise(Error_Criticality_error,
			    "unknown single character option %s", arg);
	      }
	    }
	  }
	}
      }
    }
  }

  Target_info.handleCommandLineOptions(Main__options.mainFileNamePrefix,
				       argumentList, optionIsHandledList);

  /* find out whether there are any unhandled options */
  for (i = 1;  i <= length;  i++) {
    if (!optionIsHandledList[i]) {
      String_Type argument = List_getElement(argumentList, i);
      Error_raise(Error_Criticality_warning, "unknown commandline option: %s",
		  String_asCharPointer(argument));
      isOkay = false;
    }
  }

  if (!isOkay) {
    Main__giveUsageInfo();
  }

  /* process all open command line options with delayed processing */
  parserOptions.defaultBase = Main__options.radix;
  Parser_setDefaultOptions(parserOptions);
  MapFile_setOptions(Main__options.radix, Main__options.linkFileList);

  if (Main__options.ihxFileIsUsed) {
    String_Type ihxFileName = String_make();
    String_copy(&ihxFileName, Main__options.mainFileNamePrefix);
    String_appendCharArray(&ihxFileName, ".ihx");
    CodeOutput_create(ihxFileName, CodeOutput_writeIHXLine);
    String_destroy(&ihxFileName);
  }

  if (Main__options.sRecordFileIsUsed) {
    String_Type s19FileName = String_make();
    String_copy(&s19FileName, Main__options.mainFileNamePrefix);
    String_appendCharArray(&s19FileName, ".s19");
    CodeOutput_create(s19FileName, CodeOutput_writeS19Line);
    String_destroy(&s19FileName);
  }
}

/*--------------------*/

static void Main__setBaseAddresses (void)
  /** sets all base addresses of areas to values from
      <StringTable.baseAddressList> */
{
  Parser_setMappingFromList(StringTable_baseAddressList,
		    (Parser_KeyValueMappingProc) Area_setBaseAddresses);
}

/*--------------------*/

static void Main__initialize (void)
  /** initializes all modules */
{
  String_Type platformName = String_make();

  /* first initialize the basic modules */
  File_initialize();
  Error_initialize();
  List_initialize();
  Set_initialize();
  String_initialize();
  Map_initialize();
  Multimap_initialize();

  /* now initialize the linker specific modules */
  Area_initialize();
  Banking_initialize();
  CodeSequence_initialize();
  Library_initialize();
  ListingUpdater_initialize();
  MapFile_initialize();
  Module_initialize();
  NoICEMapFile_initialize();
  Parser_initialize();
  Scanner_initialize();
  StringTable_initialize();
  Target_initialize();

  String_copyCharArray(&platformName, "gbz80");
  Target_setInfo(platformName);
  Target_info.initialize();

  /* initialize all modules depending on platform settings */
  Symbol_initialize(Target_info.isCaseSensitive);
  CodeOutput_initialize(Target_info.isBigEndian);

  Main__options.linkFileList         = StringList_make();
  Main__options.mainFileNamePrefix   = String_make();
  Main__options.linkFilesAreEchoed   = true;
  Main__options.radix                = 10;
  Main__options.ihxFileIsUsed        = false;
  Main__options.sRecordFileIsUsed    = false;
  Main__options.listingsAreAugmented = false;

  String_destroy(&platformName);
}

/*--------------------*/

static void Main__finalize (void)
  /** finalizes all modules in reverse order of initialization */
{
  String_destroy(&Main__options.mainFileNamePrefix);
  List_destroy(&Main__options.linkFileList);

  /* first finalize the linker specific modules */
  Target_info.finalize();
  Target_finalize();
  Symbol_finalize();
  StringTable_finalize();
  Scanner_finalize();
  Parser_finalize();
  NoICEMapFile_finalize();
  Module_finalize();
  MapFile_finalize();
  ListingUpdater_finalize();
  Library_finalize();
  CodeSequence_finalize();
  CodeOutput_finalize();
  Banking_finalize();
  Area_finalize();

  /* finally finalize the basic modules */
  Multimap_finalize();
  Map_finalize();
  String_finalize();
  Set_finalize();
  List_finalize();
  Error_finalize();
  File_finalize();
}

/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

int main (int argc, char *argv[])
  /** evaluates the command line arguments to determine the linker
      parameters;
      the linking is done in two passes:

        - the first pass goes through each object file in the order
          presented to the linker to bind the symbols to concrete
          addresses and - when banking is used - resolves interbank
          calls by introducing trampoline symbols

        - the second pass relocates the code sequences and binds them
          into a load file
 */
{
  int argumentCount;
  StringList_Type argumentList = StringList_make();
  Boolean hasInterbankReferences;
  Boolean *optionIsHandledList;

  Main__initialize();

  /* process the command line options */
  Main__collectOptions(argc, argv, &argumentList);
  argumentCount = List_length(argumentList);

  optionIsHandledList = NEWARRAY(Boolean, argumentCount + 1);

  if (optionIsHandledList == NULL) {
    Error_raise(Error_Criticality_fatalError,
		"could not allocate argument use list");
  }

  Main__processOptions(argumentList, optionIsHandledList);

  /* do a two-pass processing of all object and library files */
  /* -- PASS 1 -- */
  MapFile_openAll(Main__options.mainFileNamePrefix);
  Parser_parseObjectFiles(true, Main__options.linkFileList);
  Library_resolveUndefinedSymbols();

  hasInterbankReferences = 
    Banking_resolveInterbankReferences(&Main__options.linkFileList);

  if (hasInterbankReferences) {
    /* add banking support object files */
    Library_resolveUndefinedSymbols();
  }

  Main__setBaseAddresses();
  Area_link();
  Main__processGlobalSymbolDefinitions();
  Symbol_checkForUndefinedSymbols(&File_stderr);
  MapFile_writeLinkingData();

  /* -- PASS 2 -- */
  Parser_parseObjectFiles(false, Main__options.linkFileList);
  Library_addCodeSequences();
  CodeOutput_closeStreams();
  MapFile_closeAll();

  if (Main__options.listingsAreAugmented) {
    ListingUpdater_update(Main__options.radix, Main__options.linkFileList);
  }

  Main__finalize();
}
