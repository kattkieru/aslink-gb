/** ListingUpdater module --
    Implementation of module providing all services for augmenting
    listing files by relocated bytes in the generic SDCC linker.

    Original version by Thomas Tensi, 2008-03
    based on the module lklist.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "listingupdater.h"

/*========================================*/

#include "area.h"
#include "banking.h"
#include "globdefs.h"
#include "error.h"
#include "file.h"
#include "integermap.h"
#include "map.h"
#include "module.h"
#include "scanner.h"
#include "string.h"

#include <ctype.h>
# define CType_isHexDigit isxdigit

#include <stdio.h>
# define StdIO_sprintf sprintf

#include <stdlib.h>
# define StdLib_stringToLong strtol

#include <string.h>
# define STRING_copyNChars strncpy
/*========================================*/

static UINT8 ListingUpdater__base = 16;
  /** base used for bytes in listing file and revised listing file */

static IntegerMap_Type ListingUpdater__segmentToAddressMap;
  /** mapping from segment name to first address in some module
      file */


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static void ListingUpdater__checkForAreaDecl (inout String_Type *codeLine,
				      inout Target_Bank *segmentBank,
				      inout Target_Address *segmentAddress,
				      out Boolean *isOkay);

static void ListingUpdater__relocateData (inout String_Type *dataLine,
					  in Target_Bank segmentBank,
					  in Target_Address segmentAddress,
					  inout Target_Address *programCounter,
					  out Boolean *isOkay);

/*--------------------*/

static void ListingUpdater__adaptFile (inout File_Type *revisedListingFile,
				       in String_Type listingFileName,
				       inout File_Type *listingFile)
  /** reads all lines of assembler listing in <listingFile> with name
      <listingFileName> and adjusts the address and data bytes to the
      absolute values after linking and puts result into
      <revisedListingFile> */
{
  String_Type emptyPrefix = String_make();
  char formFeed = '\f';
  Boolean isAfterCodeLines = false;
  UINT32 lineNumber = 0;
  String_Type lineNumberString = String_make();
  SizeType lineNumberSize = 6;
  UINT8 prefixLength = 25;
  String_Type prefix = String_make();
  Target_Address programCounter; /** program counter value for current
				     line in listing file */
  Target_Address segmentAddress = 0; /** program counter value for current
					 area segment in listing file */
  Target_Bank segmentBank = 0;
  String_Type st = String_make();
  String_Type suffix = String_make();
  SizeType minimumLength = prefixLength + lineNumberSize + 1;

  String_copyCharArrayAligned(&emptyPrefix, prefixLength, "", ' ', true);
  programCounter = 0;

  for (;;) {
    SizeType stringLength;

    File_readLine(listingFile, &st);
    stringLength = String_length(st);

    if (stringLength == 0) {
      break;
    }

    if (String_getAt(st, 1) == formFeed) {
      isAfterCodeLines = true;
    }

    if (isAfterCodeLines) {
      File_writeString(revisedListingFile, st);
    } else {
      Boolean isContinuationLine = (stringLength < minimumLength);
      Boolean isOkay = true;

      if (isContinuationLine) {
	/* this is a continuation line for previous line */
	String_copy(&prefix, st);
	String_clear(&suffix);
      } else {
	lineNumber++;
	String_getSubstring(&prefix, st, 1, prefixLength);
	String_getSubstring(&lineNumberString, st, prefixLength + 1,
			    lineNumberSize);
	/* skip over prefix, line number and one blank character */
	String_getSubstring(&suffix, st, minimumLength + 1, SizeType_max);
      }

      /* process either a line with numbers or scan for an area
	 declaration */
      if (String_isEqual(prefix, emptyPrefix)) {
	ListingUpdater__checkForAreaDecl(&suffix, &segmentBank,
					 &segmentAddress, &isOkay);
      } else {
	/* some analysis of the numbers in this line should be done */
	ListingUpdater__relocateData(&prefix, segmentBank, segmentAddress,
				     &programCounter, &isOkay);
      }

      if (!isOkay) {
	Error_raise(Error_Criticality_warning,
		    "problems with listing file line %s (%d)",
		    String_asCharPointer(listingFileName), lineNumber);
      }

      File_writeString(revisedListingFile, prefix);

      if (!isContinuationLine) {
	File_writeString(revisedListingFile, lineNumberString);
	File_writeChar(revisedListingFile, ' ');
	File_writeString(revisedListingFile, suffix);
      }
    }
  }

  String_destroy(&suffix);
  String_destroy(&st);
  String_destroy(&prefix);
  String_destroy(&lineNumberString);
  String_destroy(&emptyPrefix);
}

/*--------------------*/

static void ListingUpdater__checkForAreaDecl (inout String_Type *codeLine,
				      inout Target_Bank *segmentBank,
				      inout Target_Address *segmentAddress,
				      out Boolean *isOkay)
  /** checks whether <codeLine> contains an AREA declaration and sets
      <segmentAddress> to its address and <segmentBank> to its bank
      (by data from the segment lists); <isOkay> tells whether line
      could be parsed successfully; when area is relocated to another
      bank, its name is adapted accordingly */
{
  Scanner_TokenList tokenList;
  Scanner_Token *token;
  String_Type areaKeyword = String_makeFromCharArray(".area");

  Scanner_makeTokenList(&tokenList, *codeLine);
  token = List_getElement(tokenList, 1);

  if (token->kind == Scanner_TokenKind_identifier
      && String_isEqual(token->representation, areaKeyword)) {
    /* this is an area line ==> get area name and find associated
       segment address */
    token = List_getElement(tokenList, 2);

    if (token->kind != Scanner_TokenKind_identifier) {
      *isOkay = false;
    } else {
      long newSegmentAddress;
      String_Type segmentName = String_make();

      String_copy(&segmentName, token->representation);
      Banking_adaptAreaNameWhenBanked(Module_currentModule(), &segmentName);

      if (!String_isEqual(segmentName, token->representation)) {
	/* the original area is not used here because of banking
	   ==> adapt code line */
	String_copyCharArray(codeLine, "\t");
	String_append(codeLine, areaKeyword);
	String_appendCharArray(codeLine, "\t");
	String_append(codeLine, segmentName);
	String_appendCharArray(codeLine, "\n");
      }

      if (Target_info.getBankFromSegmentName == NULL) {
	*segmentBank = 0;
      } else {
	*segmentBank = Target_info.getBankFromSegmentName(segmentName);
      }

      newSegmentAddress =
	IntegerMap_lookup(ListingUpdater__segmentToAddressMap, segmentName);

      if (newSegmentAddress == IntegerMap_notFound) {
	*isOkay = false;
      } else {
	*segmentAddress = newSegmentAddress;
      }

      String_destroy(&segmentName);
    }
  }

  String_destroy(&areaKeyword);
  List_destroy(&tokenList);
}

/*--------------------*/

static void ListingUpdater__relocateData (inout String_Type *dataLine,
					  in Target_Bank segmentBank,
					  in Target_Address segmentAddress,
					  inout Target_Address *programCounter,
					  out Boolean *isOkay)
  /** relocate address information and code or data bytes in
      <dataLine> relative to <programCounter>; <isOkay> tells whether
      line could be parsed successfully; the program counter is
      synched accordingly for each data byte encountered */
{
#define maxNumberCount 10
  /** maximum count of numbers in a data line */
  struct Descriptor {
    char *start;
    UINT8 count;
    UINT16 value;
  } numberDescriptor[maxNumberCount];

  char newline = '\n';
  UINT8 numberCount = 0;
  *isOkay = true;

  {
    /* parse byte sequences in <dataLine>: the first is an address, the
       rest are code or data bytes */
    char *ptr = String_asCharPointer(*dataLine);

    for (;;) {
      while (*ptr == ' ') {
	ptr++;
      }

      if (*ptr == String_terminator || *ptr == newline) {
	break;
      } else {
	if (!CType_isHexDigit(*ptr) || numberCount >= maxNumberCount) {
	  *isOkay = false;
	  ptr++;
	} else {
	  struct Descriptor *descriptor = &numberDescriptor[numberCount];
	  UINT8 count = 0;
	  char *start = ptr;
	  char *junk;

	  while (CType_isHexDigit(*ptr)) {
	    count++;
	    ptr++;
	  }

	  descriptor->start = start;
	  descriptor->count = count;
	  descriptor->value = StdLib_stringToLong(start, &junk,
						  ListingUpdater__base);
	  numberCount++;
	}
      }
    }
  }

  {
    /* update the bytes from program counter */
    UINT8 i;
    char *format = (ListingUpdater__base == 16
		    ? "%0*X"
		    : (ListingUpdater__base == 8 ? "%0*o" : "%0*u"));

    for (i = 0;  i < numberCount;  i++) {
      struct Descriptor *descriptor = &numberDescriptor[i];
      UINT8 count = descriptor->count;
      UINT16 value = descriptor->value;
      char temp[20];

      if (count == 4) {
	value = segmentAddress + value;
	*programCounter = value;
      } else {
	value = Target_info.getCodeByte(segmentBank, *programCounter);
	(*programCounter)++;
      }

      StdIO_sprintf(temp, format, count, value);
      STRING_copyNChars(descriptor->start, temp, count);
    }
  }
}

/*--------------------*/

static void ListingUpdater__setupAreaMap (in String_Type linkFileName)
  /* prepares a map from area names to start addresses taken from the
     module associated with the object file given by <linkFileName> */
{
  Boolean isFound;

  Map_clear(&ListingUpdater__segmentToAddressMap);
  Module_setCurrentByFileName(linkFileName, &isFound);

  if (isFound) {
    List_Cursor segmentCursor;
    List_Type segmentList = List_make(Area_segmentTypeDescriptor);
    String_Type segmentName = String_make();

    Module_getSegmentList(Module_currentModule(), &segmentList);

    for (segmentCursor = List_resetCursor(segmentList);
	 segmentCursor != NULL;
	 List_advanceCursor(&segmentCursor)) {
      Area_Segment segment = List_getElementAtCursor(segmentCursor);
      Target_Address address = Area_getSegmentAddress(segment);

      Area_getSegmentName (segment, &segmentName);

      IntegerMap_set(&ListingUpdater__segmentToAddressMap,
		     segmentName, address);
    }
     
    String_destroy(&segmentName);
    List_destroy(&segmentList);
  }
}

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void ListingUpdater_initialize (void)
{
  ListingUpdater__segmentToAddressMap = Map_make(String_typeDescriptor);
}

/*--------------------*/

void ListingUpdater_finalize (void)
{
  Map_destroy(&ListingUpdater__segmentToAddressMap);
}


/*--------------------*/
/* TRANSFORMATION     */ 
/*--------------------*/

void ListingUpdater_update (in UINT8 base, in StringList_Type linkFileList)
{
  String_Type fileName = String_make();
  List_Cursor stringCursor;

  ListingUpdater__base = base;

  for (stringCursor = List_resetCursor(linkFileList);
       stringCursor != NULL;
       List_advanceCursor(&stringCursor)) {
    String_Type listingFileName = String_make();
    String_Type linkFileName = List_getElementAtCursor(stringCursor);
    SizeType dotPosition = String_findCharacterFromEnd(linkFileName, '.');
    SizeType slashPosition = String_findFromEnd(linkFileName,
						File_directorySeparator);
    if (dotPosition == String_notFound
	|| slashPosition != String_notFound && dotPosition < slashPosition) {
      /* no extension found */
      String_copy(&fileName, linkFileName);
    } else {
      String_getSubstring(&fileName, linkFileName, 1, dotPosition - 1);
    }

    String_copy(&listingFileName, fileName);
    String_appendCharArray(&listingFileName, ".lst");

    if (File_exists(listingFileName)) {
      File_Type listingFile;

      if (File_open(&listingFile, listingFileName, File_Mode_read)) {
	File_Type revisedListingFile;
	String_Type revisedListingFileName = String_make();

	String_copy(&revisedListingFileName, fileName);
	String_appendCharArray(&revisedListingFileName, ".rst");

	if (File_open(&revisedListingFile, revisedListingFileName,
		      File_Mode_write)) {
	  ListingUpdater__setupAreaMap(linkFileName);
	  ListingUpdater__adaptFile(&revisedListingFile, listingFileName,
				    &listingFile);
	  File_close(&revisedListingFile);
	}

	File_close(&listingFile);
	String_destroy(&revisedListingFileName);
      }

    }

    String_destroy(&listingFileName);
  }
}
