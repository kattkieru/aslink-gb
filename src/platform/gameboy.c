/** Gameboy target module --
    Implementation of module providing the target specific services
    for the Gameboy target within the generic SDCC linker.

    Original version by Thomas Tensi, 2007-02
    based on the module lkgb.c by Pascal Felber

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "gameboy.h"

/*========================================*/

#include "../area.h"
#include "../codeoutput.h"
#include "../codesequence.h"
#include "../error.h"
#include "../file.h"
#include "../globdefs.h"
#include "../list.h"
#include "../mapfile.h"
#include "../symbol.h"
#include "../string.h"
#include "../stringlist.h"
#include "../stringtable.h"

#include <ctype.h>
# define CType_isalpha isalpha
# define CType_isalnum isalnum
# define CType_isdigit isdigit
# define CType_toupper(ch) ((char) toupper(ch))
#include <stdio.h>
# define StdIO_sprintf sprintf
#include <stdlib.h>
# define StdLib_atoi    atoi
# define StdLib_strtol  strtol
# define StdLib_malloc  malloc
#include <string.h>
# define STRING_boundedCopy strncpy
# define STRING_memset      memset
# define STRING_strlen      strlen

/*========================================*/

#define undefined 0xFF

#define Gameboy__patchMagicNumber 0x132411


#define Gameboy__defaultCartridgeValue	0xFF
/** value used to fill the unused portions of the image; FFh puts less
    stress on a EPROM/Flash */

#define Gameboy__bankSize 0x4000UL
  /** size of a ROM bank */
#define Gameboy__bankStartAddress 0x4000UL
  /** address where overlayed ROM banks start */
#define Gameboy__maxRomAddress 0x7FFFUL
  /** last address in ROM */

#define Gameboy__maxBankCount 256
#define Gameboy__maxTitleLength 16

typedef struct {
  long magicNumber;
  UINT16 address;
  UINT8 value;
} Gameboy__PatchRecord;
  /** type defining the patching of specific bytes in cartridge
      (typically used for setting bytes in the header)  */  

typedef Gameboy__PatchRecord *Gameboy__Patch;


static UINT8 *Gameboy__data[Gameboy__maxBankCount];


/* the following values are configuration data from the command line
   options */
static char Gameboy__cartridgeTitle[Gameboy__maxTitleLength] = "";
static UINT8 Gameboy__romBankCount;
static UINT8 Gameboy__ramBankCount;
static UINT8 Gameboy__cartridgeType;

static UINT32 Gameboy__cartridgeSize;
  /** number of ROM bytes in a cartridge (depending on ROM bank
      count) */

static List_Type Gameboy__patchList;

static Banking_Configuration Gameboy__bankingConfiguration;
static String_Type Gameboy__codeAreaPrefix;
  /** prefix identifying all code areas which are targets for
      interbank calls */

/* ---- type descriptors ---- */ 
static Object Gameboy__makePatch (void);

static TypeDescriptor_Record Gameboy__patchRecordTDRecord =
  { /* .objectSize = */ sizeof(Gameboy__PatchRecord),
    /* .assignmentProc = */ NULL, /* .comparisonProc = */ NULL,
    /* .constructionProc = */ Gameboy__makePatch,
    /* .destructionProc = */ NULL, /* .hashCodeProc = */ NULL,
    /* .keyValidationProc = */ NULL };

static TypeDescriptor_Type Gameboy__patchRecordTypeDescriptor =
  &Gameboy__patchRecordTDRecord;
  /** variable used for describing the type properties when patch
      records occur in generic types like lists */


/* some constants for nogmb map files */
static String_Type Gameboy__codeAreaSymbolPrefix;
static String_Type Gameboy__lengthSymbolPrefix;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static UINT8 Gameboy__getCartridgeByte (UINT16 address);
static UINT8 Gameboy__ramCountCode (UINT8 value);
static UINT8 Gameboy__romCountCode (UINT8 value);
static void Gameboy__putAreaToMapFile (inout File_Type *file,
					in Area_Type area);
static void Gameboy__setCartridgeByte (in UINT16 address, in UINT8 value);
static void Gameboy__writeCodeLine (inout File_Type *file,
				    in CodeOutput_State state,
				    in Boolean isBigEndian,
				    in CodeSequence_Type sequence);

/*--------------------*/

static Gameboy__Patch Gameboy__attemptConversionToPatch (in Object patch)
  /** verifies that <patch> is really a pointer to a Gameboy patch;
      if not, the program stops with an error message  */
{
  return (Gameboy__Patch) attemptConversion("Gameboy__Patch",
					    patch, Gameboy__patchMagicNumber);
}

/*--------------------*/

static long Gameboy__getBankFromName (in String_Type st)
  /* gets the bank from area symbol string <st> */
{
  String_Type numberString = String_make();
  SizeType pos;
  long result;

  pos = String_findCharacterFromEnd(st, '_');
  String_getSubstring(&numberString, st, pos + 1, 2);
  String_convertToLong(numberString, 10, &result);
  String_destroy(&numberString);

  return result;
}

/*--------------------*/

static void Gameboy__finalizeData (void)
{
  UINT16 cartridgeTitleAddress          = 0x0134;
  UINT16 cartridgeTypeAddress           = 0x0147;
  UINT16 cartridgeRomSizeAddress        = 0x0148;
  UINT16 cartridgeRamSizeAddress        = 0x0149;
  UINT16 cartridgeHeaderChecksumAddress = 0x014D;
  UINT16 cartridgeGlobalChecksumAddress = 0x014E;

  UINT16 checkSum;
  SizeType i;
  List_Cursor patchCursor;
  UINT16 pos;
  UINT8 value;
  char *st = Gameboy__cartridgeTitle;

  i = STRING_strlen(st);

  while (i > 0) {
    UINT8 ch = st[i-1];
    if (!CType_isalnum(ch) && ch != '.') {
      break;
    } else {
      i--;
    }
  }

  for (pos = 0;   pos < Gameboy__maxTitleLength;   pos++) {
    UINT8 ch = st[i];
    if (ch == '.') {
      break;
    } else {
      Gameboy__cartridgeTitle[pos] = CType_toupper(ch);
      i++;
    }
  }

  if (pos < Gameboy__maxTitleLength) {
    Gameboy__cartridgeTitle[pos] = String_terminator;
  }

  /* put cartridge name to cartridge */
  for (pos = cartridgeTitleAddress, i = 0;
       i < Gameboy__maxTitleLength;  pos++, i++) {
    UINT8 ch = Gameboy__cartridgeTitle[i];
    if (ch == String_terminator) {
      break;
    } else {
      Gameboy__setCartridgeByte(pos, ch);
    }
  }

  while  (pos < cartridgeTitleAddress + Gameboy__maxTitleLength) {
    Gameboy__setCartridgeByte(pos, 0);
    pos++;
  }

  Gameboy__setCartridgeByte(cartridgeTypeAddress, Gameboy__cartridgeType);

  /* put count of ROM banks to cartridge */
  value = Gameboy__romCountCode(Gameboy__romBankCount);
  Gameboy__setCartridgeByte(cartridgeRomSizeAddress, value);

  /* put count of RAM banks to cartridge */
  value = Gameboy__ramCountCode(Gameboy__ramBankCount);
  Gameboy__setCartridgeByte(cartridgeRamSizeAddress, value);

  /* apply all patches in patch list */
  patchCursor = List_resetCursor(Gameboy__patchList);

  while (patchCursor != NULL) {
    Object object = List_getElementAtCursor(patchCursor);
    Gameboy__Patch patch = Gameboy__attemptConversionToPatch(object);
    Gameboy__setCartridgeByte(patch->address, patch->value);
    List_advanceCursor(&patchCursor);
  }

  /* calculate checksum of header and store it in cartridge */
  checkSum = 0;

  for (pos = cartridgeTitleAddress;  pos < cartridgeHeaderChecksumAddress;
       pos++) {
    checkSum += Gameboy__getCartridgeByte(pos);
  }

  value = (0xE7 - (UINT8) (checkSum & 0xFF));
  Gameboy__setCartridgeByte(cartridgeHeaderChecksumAddress, value);

  /* calculate global checksum and store it in cartridge */
  checkSum = 0;
  Gameboy__setCartridgeByte(cartridgeGlobalChecksumAddress, 0);
  Gameboy__setCartridgeByte(cartridgeGlobalChecksumAddress + 1, 0);

  for (i = 0;  i < Gameboy__romBankCount;  i++) {
    for (pos = 0;  pos < Gameboy__bankSize;  pos++) {
      checkSum += Gameboy__data[i][pos];
    }
  }

  value = (UINT8) ((checkSum >> 8) & 0xFF);
  Gameboy__setCartridgeByte(cartridgeGlobalChecksumAddress, value);
  value = (UINT8) (checkSum & 0xFF);
  Gameboy__setCartridgeByte(cartridgeGlobalChecksumAddress + 1, value);
}

/*--------------------*/

static void Gameboy__generateNoGmbMapFile (inout File_Type *file)
  /** writes a map file in NoGMB format to <file> */
{
  List_Cursor areaCursor;
  Area_List areaList = List_make(Area_typeDescriptor);

  File_writeCharArray(file, "; no$gmb format .sym file\n"
		      "; Generated automagically by ASxxxx linker\n");
  Area_getList(&areaList);

  for (areaCursor = List_resetCursor(areaList);  areaCursor != NULL;
       List_advanceCursor(&areaCursor)) {
    Area_Type area = List_getElementAtCursor(areaCursor);
    Gameboy__putAreaToMapFile(file, area);
  }

  List_destroy(&areaList);
}

/*--------------------*/

static UINT8 Gameboy__getCartridgeByte (UINT16 address)
  /** returns the byte value in cartridge at <address> */
{
  UINT8 bank = (UINT8) (address / Gameboy__bankSize);
  UINT16 relativeAddress = address % Gameboy__bankSize;
  return Gameboy__data[bank][relativeAddress];
}

/*--------------------*/

static void Gameboy__initializeData (void)
  /** allocates all Gameboy segments and fills them with the default
      cartridge value */
{
  UINT8 i;

  for (i = 0;  i < Gameboy__romBankCount;  i++) {
    UINT8 *data = StdLib_malloc(Gameboy__bankSize);

    if (data == NULL) {
      Error_raise(Error_Criticality_fatalError,
		  "can't allocate data for bank %d", i);
    }

    STRING_memset(data, Gameboy__defaultCartridgeValue, Gameboy__bankSize);
    Gameboy__data[i] = data;
  }
}

/*--------------------*/

static char *Gameboy__makeAddressBytes (in UINT16 value)
  /** makes a character string containing the little-endian hex
      representation of <value> with an embedded blank */
{
  static char result[6];
  char *hexDigits = "0123456789ABCDEF";
  UINT8 partA = (UINT8) (value & 0xFF);
  UINT8 partB = (UINT8) (value >> 8);

  result[0] = hexDigits[partA >> 4];
  result[1] = hexDigits[partA & 0xF];
  result[2] = ' ';
  result[3] = hexDigits[partB >> 4];
  result[4] = hexDigits[partB & 0xF];
  result[5] = '\0';

  return result;
}

/*--------------------*/

static Object Gameboy__makePatch (void)
  /** private construction of patch used when a new entry is
      created in patch record list */
{
  Gameboy__Patch patch = NEW(Gameboy__PatchRecord);
  patch->magicNumber = Gameboy__patchMagicNumber;
  return patch;
}

/*--------------------*/

static void Gameboy__putAreaToMapFile (inout File_Type *file,
				       in Area_Type area)
  /* write the symbols from <area> to map file */
{
  String_Type areaName = String_make();
  Symbol_List areaSymbolList = List_make(Symbol_typeDescriptor);
  long currentBank = 0;
  List_Cursor symbolCursor;

  Area_getName(area, &areaName);

  File_writeCharArray(file, "; Area: ");
  File_writeString(file, areaName);
  File_writeCharArray(file, "\n");
  
  if (!String_hasPrefix(areaName, Gameboy__codeAreaPrefix)) {
    currentBank = 0;
  } else {
    currentBank = Gameboy__getBankFromName(areaName);
  }

  MapFile_getSortedAreaSymbolList(area, &areaSymbolList);

  /* write all symbols to map file */
  for (symbolCursor = List_resetCursor(areaSymbolList);
       symbolCursor != NULL;
       List_advanceCursor(&symbolCursor)) {
    Symbol_Type symbol = List_getElementAtCursor(symbolCursor);
    String_Type symbolName = String_make();
    Target_Address address = Symbol_absoluteAddress(symbol);

    Symbol_getName(symbol, &symbolName);

    if (!String_hasPrefix(symbolName, Gameboy__lengthSymbolPrefix)) {
      if (!String_hasPrefix(symbolName, Gameboy__codeAreaSymbolPrefix)) {
	File_writeHex(file, currentBank, 2);
      } else {
	long bank = Gameboy__getBankFromName(symbolName);
	File_writeHex(file, bank, 2);
      }

      File_writeCharArray(file, ":");

      if (currentBank > 0) {
	address = (address & 0x7FFF);
      }

      File_writeHex(file, address, 4);
      File_writeCharArray(file, " ");
      File_writeString(file, symbolName);
      File_writeCharArray(file, "\n");
    }

    String_destroy(&symbolName);
  }

  List_destroy(&areaSymbolList);
  String_destroy(&areaName);
}

/*--------------------*/

static UINT8 Gameboy__ramCountCode (UINT8 value)
  /** returns the numerical code for the RAM bank count given by
      <value> within a Gameboy cartridge or <undefined>, when this ram
      bank count is bad */
{
  UINT8 result;

  switch (value) {
    case 0:
      result = 0;  break;
    case 1:
      result = 2;  break;
    case 4:
      result = 3;  break;
    case 16:
      result = 4;  break;
    default:
      value = undefined;  break;
  }

  return value;
}

/*--------------------*/

static UINT8 Gameboy__romCountCode (UINT8 value)
  /** returns the numerical code for the ROM bank count given by
      <value> within a Gameboy cartridge or <undefined>, when this rom
      bank count is bad */
{
  UINT8 result;

  switch (value) {
    case 2:
      result = 0;  break;
    case 4:
      result = 1;  break;
    case 8:
      result = 2;  break;
    case 16:
      result = 3;  break;
    case 32:
      result = 4;  break;
    case 64:
      result = 5;  break;
    case 128:
      result = 6;  break;
    case 256:
      result = 7;  break;
    case 512:
      result = 8;  break;
    default:
      result = undefined;  break;
  }

  return value;
}

/*--------------------*/

static void Gameboy__setCartridgeByte (in UINT16 address, in UINT8 value)
  /** sets byte in cartridge at <address> to <value> */
{
  UINT8 bank = (UINT8) (address / Gameboy__bankSize);
  UINT16 relativeAddress = address % Gameboy__bankSize;
  Gameboy__data[bank][relativeAddress] = value;
}

/*--------------------*/

static void Gameboy__processCodeSequence (in CodeSequence_Type sequence)
  /** adds code sequence <sequence> to output */
{
  if (sequence.length > 0) {
    UINT16 address       = sequence.offsetAddress;
    Target_Bank romBank = sequence.romBank;
    Boolean hasError = true;
    char errorMessage[255];

    /* perform some validity checks before writing to cartridge */
    if (address > Gameboy__maxRomAddress) {
      StdIO_sprintf(errorMessage, "address overflow (addr %lx > %lx)",
		    address, Gameboy__maxRomAddress);
    } else if (romBank >= Gameboy__romBankCount) {
      StdIO_sprintf(errorMessage, "bank overflow (bank %x > last bank %x)",
		    romBank, Gameboy__romBankCount);
    } else if (romBank > 0 
	       && address < Gameboy__bankStartAddress) {
      StdIO_sprintf(errorMessage, "address underflow (addr %lx < %lx)",
		    address, Gameboy__bankStartAddress);
    } else if (Gameboy__romBankCount == 2 && romBank > 0) {
      StdIO_sprintf(errorMessage,
	    "no bank switching possible when using only two ROM banks");
    } else {
      hasError = false;
    }

    if (hasError) {
      Error_raise(Error_Criticality_fatalError, errorMessage);
    } else {
      UINT8 i;

      if (romBank > 1) {
	/* adjust address */
	address += (romBank - 1) * Gameboy__bankSize;
      }

      for (i = 0;  i < sequence.length;  i++) {
	if (address >= Gameboy__cartridgeSize) {
	  Error_raise(Error_Criticality_fatalError,
		      "cartridge size overflow (addr %lx >= %lx)",
		      address, Gameboy__cartridgeSize);
	} else {
	  UINT8 oldValue = Gameboy__getCartridgeByte(address);
	  UINT8 newValue = sequence.byteList[i];

	  Gameboy__setCartridgeByte(address, newValue);

	  if (oldValue != Gameboy__defaultCartridgeValue) {
	    Error_raise(Error_Criticality_warning,
			"possibly wrote twice at addr %lx (%02X->%02X)",
			address, newValue, oldValue);
	  }
	}

	address++;
      }
    }
  }
}

/*--------------------*/

static void Gameboy__setBaseAddressTable (void)
  /** updates base address table for <romBankCount> and <ramBankCount> */
{
  char template[20];
  UINT8 i;

  for (i = 1;  i < Gameboy__romBankCount;  i++) {
    StdIO_sprintf(template, "_CODE_%d=0x4000", i);
    StringTable_addCharArray(&StringTable_baseAddressList, template);
  }

  for (i = 0; i < Gameboy__ramBankCount;  i++) {
    StdIO_sprintf(template, "_DATA_%d=0xA000", i);
    StringTable_addCharArray(&StringTable_baseAddressList, template);
  }
}

/*========================================*/
/*            CALLBACK ROUTINES           */
/*========================================*/

static Target_Bank Gameboy__getBankFromSegmentName (
					     in String_Type segmentName)
  /** scans <segmentName> whether it contains a ROM bank switch
      information; in the Gameboy those segments have a trailing
      underscore with a subsequent digit */
{
  INT32 position = String_findCharacterFromEnd(segmentName, '_');
  Target_Bank romBank = 0;

  if (position != String_notFound  
      && position + 1 <= (INT32) String_length(segmentName)) {
    UINT32 digitPosition = position + 1;
    char digitChar = String_getCharacter(segmentName, digitPosition);
    if (CType_isdigit(digitChar)) {
      romBank = digitChar - '0';
    }
  }

  return romBank;
}

/*--------------------*/

static UINT8 Gameboy__getCodeByte (in Target_Bank bank,
				   in Target_Address address)
{
  return Gameboy__data[bank][address];
}

/*--------------------*/

static Boolean Gameboy__ensureAsCallTarget (in String_Type moduleName,
					    in String_Type segmentName,
					    in String_Type symbolName)
  /** checks whether some symbol is a valid target for an interbank
      call (i.e. it lies in a code segment) */
{
  return String_hasPrefix(segmentName, Gameboy__codeAreaPrefix);
}

/*--------------------*/

static void Gameboy__giveUsageInfo (out String_Type *st)
  /** returns an indented line list with platform specific options as
      a usage info in <st> */
{
  char *result = 
    "Platform Gameboy:\n"
    "  -j   no$gmb symbol file generated as file[SYM]\n"
    "  -yo  Number of ROM banks (default: 2)\n"
    "  -ya  Number of RAM banks (default: 0)\n"
    "  -yt  MBC type (default: no MBC)\n"
    "  -yn  Name of program (default: name of output file)\n"
    "  -yp# Patch one byte in the output GB file (# is: addr=byte)\n"
    "  -z   Gameboy image as file[GB]\n";

  String_copyCharArray(st, result);
}

/*--------------------*/

static void Gameboy__handleCommandLine (in String_Type mainFileNamePrefix,
					in StringList_Type argumentList,
					inout Boolean optionIsHandledList[])
  /** parses command line options in <argumentList> for options
      relevant for this platform; all options where
      <optionIsHandledList> is true have already been processed; when
      an option is used by that routine, <optionIsHandledList> is also
      set to true; <mainFileNamePrefix> tells the name of the main
      file passed as parameter without extension */
{
  int i;
  int argumentListLength = List_length(argumentList);

  for (i = 1;  i <= argumentListLength;  i++) {
    Boolean *optionIsHandled = &optionIsHandledList[i];
    String_Type argument = List_getElement(argumentList, i);
    char *arg = String_asCharPointer(argument);
    char firstChar;

    firstChar = arg[0];

    if (Gameboy__cartridgeTitle[0] == String_terminator 
	&& (CType_isalnum(firstChar) || firstChar == '_')) {
      /* this is the first link file name and a default for the
	 cartridge title */
      UINT8 lastPos = Gameboy__maxTitleLength-1;
      STRING_boundedCopy(Gameboy__cartridgeTitle, arg, lastPos);
      Gameboy__cartridgeTitle[lastPos] = String_terminator;
    } else if (!*optionIsHandled) {
      if (firstChar == '-') {
	SizeType length = STRING_strlen(arg);
	char secondChar = CType_toupper(arg[1]);

	*optionIsHandled = true;

	if (secondChar == 'Z') {
	  String_Type outputFileName = String_make();
	  String_append(&outputFileName, mainFileNamePrefix);
	  String_appendCharArray(&outputFileName, ".gb");
	  CodeOutput_create(outputFileName, Gameboy__writeCodeLine);
	  String_destroy(&outputFileName);
	} else if (secondChar == 'J') {
	  MapFile_ProcDescriptor routines =
	    { NULL, Gameboy__generateNoGmbMapFile };
	  String_Type suffix = String_makeFromCharArray(".sym");
	  MapFile_registerForOutput(suffix, routines);
	  String_destroy(&suffix);
	} else if (secondChar != 'Y') {
	  *optionIsHandled = false;
	} else {
	  char *endp;
	  Boolean hasError;
	  Object *objectPtr;
	  char optionChar = CType_toupper(arg[2]);
	  Gameboy__Patch patch;
	  char *st;
	  UINT8 value = undefined;

	  if (length > 3) {
	   value = (UINT8) StdLib_atoi(&arg[3]);
	  }

	  switch (optionChar) {
	    case 'O':
	      if (Gameboy__romCountCode(value) == undefined) {
		Error_raise(Error_Criticality_warning,
			    "unsupported number of ROM banks [%d]", value);
	      }

	      Gameboy__romBankCount = value;
	      Gameboy__cartridgeSize = value * Gameboy__bankSize;
	      break;

	    case 'A':
	      if (Gameboy__ramCountCode(value) == undefined) {
		Error_raise(Error_Criticality_warning, 
			    "unsupported number of RAM banks [%d]", value);
	      }

	      Gameboy__ramBankCount = value;
	      break;

	    case 'T':
	      Gameboy__cartridgeType = value;
	      break;

	    case 'N':
	      if (arg[3] != '=' || arg[4] != '"') {
		Error_raise(Error_Criticality_fatalError,
			    "Syntax error in -YN=\"name\" option");
	      } else {
		UINT8 j = 0;
		st = &arg[5];

		while (*st != String_terminator && *st != '"'
		       && j < Gameboy__maxTitleLength) {
		  Gameboy__cartridgeTitle[j++] = *st++;
		}

		while (j < Gameboy__maxTitleLength) {
		  Gameboy__cartridgeTitle[j++] = String_terminator;
		}
	      }

	      break;

	    case 'P':
	      objectPtr = List_append(&Gameboy__patchList);
	      patch = Gameboy__attemptConversionToPatch(*objectPtr);
	      hasError = false;
	      st = &arg[3];

	      patch->address = StdLib_strtol(st, &endp, 0);;

	      if (endp == st || *endp != '=') {
		hasError = true;
	      } else {
		st = endp + 1;
		patch->value = (UINT8) StdLib_strtol(st, &endp, 0);
		hasError = (endp == st);
	      }

	      if (hasError) {
		Error_raise(Error_Criticality_fatalError,
			    "Syntax error in -YPaddress=value option");
	      }

	      break;

	    default:
	      *optionIsHandled = false;
	      Error_raise(Error_Criticality_fatalError, "invalid option %s",
			  arg);
	  }
	}
      }
    }
  }

  Gameboy__initializeData();
  Gameboy__setBaseAddressTable();
}

/*--------------------*/

static void Gameboy__makeBankedCodeAreaName (out String_Type *areaName,
					     in Target_Bank bank)
  /** constructs a banked code area name <areaName> from <bank>; when
      bank is undefined returns nonbanked code area name */
{
  if (bank == Target_undefinedBank) {
    String_copy(areaName, Gameboy__bankingConfiguration.nonbankedCodeAreaName);
  } else {
    String_copyCharArray(areaName, "_CODE_");
    String_appendInteger(areaName, bank, 16);
  }
}

/*--------------------*/

static void Gameboy__makeJumpLabelName (out String_Type *labelName,
					in Target_Bank bank)
    /** constructs the jump label <labelName> (for bank switching
	within a trampoline call) from <bank> */
{
  String_copyCharArray(labelName, "Banking__switchTo");
  String_appendInteger(labelName, bank, 16);
}

/*--------------------*/

static void Gameboy__makeSurrogateSymbolName (
				      out String_Type *surrogateSymbolName,
				      in String_Type symbolName)
  /** constructs a concrete trampoline surrogate symbol name in
      <surrogateSymbolName> from <symbolName> */
{
  String_copyCharArray(surrogateSymbolName, "_BC");
  String_append(surrogateSymbolName, symbolName);
}

/*--------------------*/

static void Gameboy__makeTrampolineCallCode (in UINT16 startAddress,
					     in UINT16 referencedAreaIndex,
					     in UINT16 targetSymbolIndex,
					     in UINT16 jumpLabelSymbolIndex,
					     out String_Type *codeSequence)
  /** constructs concrete trampoline call code in the nonbanked code
      area; the start address of the code within the segment is given
      as <startAddress >, the target symbol is given by
      <targetSymbolIndex> in area with index <referencedAreaIndex> and
      the jump label symbol as index <jumpLabelSymbolIndex> also in
      the same area; the routine returns several code lines in
      <codeSequence>: a T line with the call and a relocation line
      referencing the target symbol and the bank switch label */
{
  /* a call to routine XYZ in bank 23 is done by the following assembler
     code:
       BC_XYZ: LD   BC,#XYZ
               JMP  Banking_switchTo_23 */

  /* build a T line containing the load of the target address
     ("LD BC, targetAddress") and the jump to the bank switch label
     ("JMP Banking_switch_label"); additionally a corresponding R line
     is added for the relocation */

  /* -- T line */
  String_copyCharArray(codeSequence, "T ");
  String_appendCharArray(codeSequence,
			      Gameboy__makeAddressBytes(startAddress));
  String_appendCharArray(codeSequence, " 01 00 00 C3 00 00\n");

  /* -- R line */
  String_appendCharArray(codeSequence, "R ");
  String_appendCharArray(codeSequence, Gameboy__makeAddressBytes(0));
  String_appendCharArray(codeSequence, " ");
  String_appendCharArray(codeSequence, 
			      Gameboy__makeAddressBytes(referencedAreaIndex));
  /* first reloc: kind = sym (2), index = 3, value = targetSymbolIndex */
  String_appendCharArray(codeSequence, " 02 03 ");
  String_appendCharArray(codeSequence, 
			      Gameboy__makeAddressBytes(targetSymbolIndex));
  /* second reloc: kind = sym (2), index = 6, value = jumpLabelSymbolIndex */
  String_appendCharArray(codeSequence, " 02 06 ");
  String_appendCharArray(codeSequence, 
			      Gameboy__makeAddressBytes(jumpLabelSymbolIndex));
  String_appendCharArray(codeSequence, "\n");
}

/*--------------------*/

static void Gameboy__writeCodeLine (inout File_Type *file,
				    in CodeOutput_State state,
				    in Boolean isBigEndian,
				    in CodeSequence_Type sequence)
  /** code output routine producing Gameboy executable file format */
{
  UINT8 i;

  switch (state) {
    case CodeOutput_State_atBegin:
      /* initialization is done when the command line has been
	 parsed */
      break;

    case CodeOutput_State_inCode:
      Gameboy__processCodeSequence(sequence);
      break;

    case CodeOutput_State_atEnd:
      Gameboy__finalizeData();

      for (i = 0;  i < Gameboy__romBankCount;  i++) {
        File_writeBytes(file, Gameboy__data[i], Gameboy__bankSize);
      }
      break;
  }
}

/*--------------------*/

static void Gameboy__initialize (void)
  /** initializes internal data structures*/
{
  Banking_Configuration *conf = &Gameboy__bankingConfiguration;

  Gameboy__codeAreaPrefix = String_makeFromCharArray("_CODE");
  Gameboy__lengthSymbolPrefix = String_makeFromCharArray("l__");
  Gameboy__codeAreaSymbolPrefix = String_makeFromCharArray("s__CODE_");

  Gameboy__romBankCount  = 2;
  Gameboy__ramBankCount  = 0;
  Gameboy__cartridgeType = 0;
  Gameboy__cartridgeSize = Gameboy__romBankCount * Gameboy__bankSize;

  Gameboy__patchList = List_make(Gameboy__patchRecordTypeDescriptor);

  StringTable_addCharArray(&StringTable_baseAddressList, "_CODE=0x0200");
  StringTable_addCharArray(&StringTable_baseAddressList, "_DATA=0xC0A0");

  /* DMA transfer must start at multiples of 0x100 */
  StringTable_addCharArray(&StringTable_globalDefList, ".OAM=0xC000");
  StringTable_addCharArray(&StringTable_globalDefList, ".STACK=0xE000");
  StringTable_addCharArray(&StringTable_globalDefList, ".refresh_OAM=0xFF80");
  StringTable_addCharArray(&StringTable_globalDefList, ".init=0x0000");

  /* banking configuration */
  conf->genericBankedCodeAreaName = String_makeFromCharArray("_CODE_0");
  conf->nonbankedCodeAreaName     = String_makeFromCharArray("_CODE");
  conf->offsetPerTrampolineCall   = 6;

  conf->ensureAsCallTarget      = Gameboy__ensureAsCallTarget;
    /** checks target symbol for being target of an interbank call */
  conf->makeBankedCodeAreaName  = Gameboy__makeBankedCodeAreaName;
    /** constructs a banked code area name from a bank */
  conf->makeJumpLabelName       = Gameboy__makeJumpLabelName;
    /** constructs the jump label (for bank switching within a
	trampoline call) from a bank */
  conf->makeTrampolineCallCode  = Gameboy__makeTrampolineCallCode;
    /** constructs a concrete trampoline call code */
  conf->makeSurrogateSymbolName = Gameboy__makeSurrogateSymbolName;
    /** constructs a concrete trampoline surrogate symbol name from a
        symbol name */
}

/*--------------------*/

static void Gameboy__finalize (void)
  /** cleans up internal data structures */
{
  Banking_Configuration *conf = &Gameboy__bankingConfiguration;
  String_destroy(&conf->genericBankedCodeAreaName);
  String_destroy(&conf->nonbankedCodeAreaName);

  List_destroy(&Gameboy__patchList);

  String_destroy(&Gameboy__lengthSymbolPrefix);
  String_destroy(&Gameboy__codeAreaSymbolPrefix);
  String_destroy(&Gameboy__codeAreaPrefix);
}

/*========================================*/
/*            EXPORTED FEATURES           */
/*========================================*/

Target_Type Gameboy_targetInfo = {
  false,                             /* isBigEndian */
  true,                              /* isCaseSensitive */
  Gameboy__getBankFromSegmentName,   /* getBankFromSegmentName */
  Gameboy__getCodeByte,              /* getCodeByte */
  Gameboy__giveUsageInfo,            /* giveUsageInfo */
  Gameboy__handleCommandLine,        /* handleCommandLineOptions */
  Gameboy__initialize,               /* initialize */
  Gameboy__finalize,                 /* finalize */
  &Gameboy__bankingConfiguration     /* bankingConfiguration */
};
